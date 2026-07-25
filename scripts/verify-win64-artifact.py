#!/usr/bin/env python3

"""Verify and describe the stable PE contract of a PalSchema Win64 DLL."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


EXPECTED_EXPORTS = {"start_mod", "uninstall_mod"}
EXPECTED_PRODUCT_NAME = "PalSchema"
REQUIRED_DLL_CHARACTERISTICS = {
    "IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE",
    "IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA",
    "IMAGE_DLL_CHARACTERISTICS_NX_COMPAT",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact", type=Path, help="PalSchema.dll or main.dll")
    parser.add_argument(
        "--json-output",
        type=Path,
        help="Write the verified contract to this JSON file.",
    )
    parser.add_argument(
        "--llvm-readobj",
        default="llvm-readobj",
        help="llvm-readobj executable name or path.",
    )
    parser.add_argument(
        "--expected-version",
        help="Expected four-component PalSchema product version (defaults to version.h).",
    )
    return parser.parse_args()


def version_from_header() -> str:
    header = Path(__file__).resolve().parent.parent / "version.h"
    text = header.read_text(encoding="utf-8")
    components = []
    for name in ("MAJOR", "MINOR", "REVISION", "BUILD"):
        match = re.search(
            rf"^\s*#define\s+VERSION_{name}\s+([0-9]+)\s*$",
            text,
            re.MULTILINE,
        )
        if match is None:
            raise RuntimeError(f"Unable to resolve VERSION_{name} from {header}.")
        components.append(match.group(1))
    return ".".join(components)


def version_resource_bytes(output: str) -> bytes:
    in_version_resource = False
    in_data = False
    chunks: list[bytes] = []
    for raw_line in output.splitlines():
        line = raw_line.strip()
        if line.startswith("Type: VERSIONINFO"):
            in_version_resource = True
            continue
        if in_version_resource and line == "Data (":
            in_data = True
            continue
        if in_data and line == ")":
            break
        if not in_data:
            continue
        match = re.match(
            r"^[0-9A-Fa-f]+:\s+((?:[0-9A-Fa-f]{8}\s+)+)\|",
            line,
        )
        if match is not None:
            chunks.extend(
                bytes.fromhex(group)
                for group in match.group(1).split()
            )
    return b"".join(chunks)


def align4(offset: int) -> int:
    return (offset + 3) & ~3


def read_utf16_z(data: bytes, offset: int, end: int) -> tuple[str, int]:
    cursor = offset
    while cursor + 2 <= end:
        if data[cursor : cursor + 2] == b"\0\0":
            return data[offset:cursor].decode("utf-16-le"), cursor + 2
        cursor += 2
    raise RuntimeError("VERSIONINFO contains an unterminated UTF-16 key.")


def parse_version_node(
    data: bytes,
    offset: int,
    limit: int,
) -> tuple[dict[str, object], int]:
    if offset + 6 > limit:
        raise RuntimeError("VERSIONINFO node header is truncated.")
    length, value_length, value_type = struct.unpack_from("<HHH", data, offset)
    if length < 6 or offset + length > limit:
        raise RuntimeError("VERSIONINFO node length is invalid.")
    end = offset + length
    key, key_end = read_utf16_z(data, offset + 6, end)
    value_offset = align4(key_end)
    value_size = value_length * 2 if value_type == 1 else value_length
    value_end = value_offset + value_size
    if value_end > end:
        raise RuntimeError(f"VERSIONINFO value for {key!r} is truncated.")
    if value_type == 1:
        value: bytes | str = data[value_offset:value_end].decode(
            "utf-16-le",
        ).rstrip("\0")
    else:
        value = data[value_offset:value_end]

    children: list[dict[str, object]] = []
    cursor = align4(value_end)
    while cursor + 2 <= end:
        if data[cursor:end].strip(b"\0") == b"":
            break
        child, child_end = parse_version_node(data, cursor, end)
        children.append(child)
        cursor = align4(child_end)
    return {
        "key": key,
        "value": value,
        "children": children,
    }, end


def decode_version_info(
    resource: bytes,
) -> tuple[tuple[int, int, int, int], dict[str, list[str]]]:
    root, _ = parse_version_node(resource, 0, len(resource))
    if root["key"] != "VS_VERSION_INFO":
        raise RuntimeError("VERSIONINFO root key is invalid.")
    fixed = root["value"]
    if not isinstance(fixed, bytes) or len(fixed) < 52:
        raise RuntimeError("VS_FIXEDFILEINFO is missing or truncated.")
    fields = struct.unpack_from("<13I", fixed)
    if fields[0] != 0xFEEF04BD:
        raise RuntimeError("VS_FIXEDFILEINFO signature is invalid.")
    product_version = (
        fields[4] >> 16,
        fields[4] & 0xFFFF,
        fields[5] >> 16,
        fields[5] & 0xFFFF,
    )

    strings: dict[str, list[str]] = {}

    def collect(node: dict[str, object], under_string_table: bool = False) -> None:
        key = node["key"]
        children = node["children"]
        if not isinstance(key, str) or not isinstance(children, list):
            raise RuntimeError("VERSIONINFO contains an invalid node.")
        is_string_table = under_string_table or key == "StringFileInfo"
        value = node["value"]
        if is_string_table and isinstance(value, str) and key not in {
            "StringFileInfo",
        }:
            strings.setdefault(key, []).append(value)
        for child in children:
            if not isinstance(child, dict):
                raise RuntimeError("VERSIONINFO contains an invalid child.")
            collect(child, is_string_table)

    collect(root)
    return product_version, strings


def parse_flag_block(output: str, parent: str, heading: str) -> set[str]:
    flags: set[str] = set()
    in_parent = False
    in_block = False
    for raw_line in output.splitlines():
        line = raw_line.strip()
        if line == f"{parent} {{":
            in_parent = True
            continue
        if in_parent and line.startswith(f"{heading} ["):
            in_block = True
            continue
        if in_block and line == "]":
            break
        if in_block:
            match = re.match(r"^(IMAGE_[A-Z0-9_]+)(?:\s|\(|$)", line)
            if match:
                flags.add(match.group(1))
    return flags


def inspect_artifact(
    artifact: Path,
    llvm_readobj: str,
    expected_version: str,
) -> dict[str, object]:
    executable = shutil.which(llvm_readobj)
    if executable is None:
        raise RuntimeError(f"Unable to find {llvm_readobj!r} on PATH.")

    completed = subprocess.run(
        [
            executable,
            "--file-headers",
            "--coff-exports",
            "--coff-imports",
            "--coff-resources",
            str(artifact),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    output = completed.stdout

    exports: set[str] = set()
    imports: set[str] = set()
    block: str | None = None
    for raw_line in output.splitlines():
        line = raw_line.strip()
        if line == "Export {":
            block = "export"
            continue
        if line == "Import {":
            block = "import"
            continue
        if line == "}":
            block = None
            continue
        if not line.startswith("Name: "):
            continue

        name = line.removeprefix("Name: ").strip()
        if block == "export":
            exports.add(name)
        elif block == "import":
            imports.add(name)

    file_characteristics = parse_flag_block(
        output,
        "ImageFileHeader",
        "Characteristics",
    )
    dll_characteristics = parse_flag_block(
        output,
        "ImageOptionalHeader",
        "Characteristics",
    )
    missing_characteristics = sorted(
        characteristic
        for characteristic in REQUIRED_DLL_CHARACTERISTICS
        if characteristic not in dll_characteristics
    )
    errors: list[str] = []
    if "Format: COFF-x86-64" not in output:
        errors.append("artifact is not COFF x86-64")
    if "IMAGE_FILE_DLL" not in file_characteristics:
        errors.append("artifact is not marked as a DLL")
    if exports != EXPECTED_EXPORTS:
        errors.append(
            "exports differ: "
            f"expected {sorted(EXPECTED_EXPORTS)}, found {sorted(exports)}"
        )
    if "UE4SS.dll" not in imports:
        errors.append("dynamic UE4SS.dll import is missing")
    resource_bytes = version_resource_bytes(output)
    if "Type: VERSIONINFO" not in output:
        errors.append("VERSIONINFO resource is missing")
    elif not resource_bytes:
        errors.append("VERSIONINFO resource data is missing")
    if resource_bytes:
        try:
            fixed_version, version_strings = decode_version_info(resource_bytes)
            expected_components = tuple(int(value) for value in expected_version.split("."))
            if fixed_version != expected_components:
                errors.append(
                    "VS_FIXEDFILEINFO product version differs: "
                    f"expected {expected_components}, found {fixed_version}"
                )
            if version_strings.get("ProductName") != [EXPECTED_PRODUCT_NAME]:
                errors.append(
                    f"VERSIONINFO product identity {EXPECTED_PRODUCT_NAME!r} is missing"
                )
            if version_strings.get("ProductVersion") != [expected_version]:
                errors.append(
                    f"VERSIONINFO product version {expected_version!r} is missing"
                )
        except RuntimeError as error:
            errors.append(str(error))
    if missing_characteristics:
        errors.append(
            "missing DLL security characteristics: "
            + ", ".join(missing_characteristics)
        )
    if errors:
        raise RuntimeError("; ".join(errors))

    digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
    return {
        "schema_version": 1,
        "artifact": artifact.name,
        "sha256": digest,
        "format": "COFF-x86-64",
        "product_name": EXPECTED_PRODUCT_NAME,
        "product_version": expected_version,
        "exports": sorted(exports),
        "imported_dlls": sorted(imports, key=str.casefold),
        "dll_characteristics": sorted(REQUIRED_DLL_CHARACTERISTICS),
    }


def main() -> int:
    args = parse_args()
    artifact = args.artifact.resolve()
    if not artifact.is_file():
        print(f"Artifact does not exist: {artifact}", file=sys.stderr)
        return 1

    try:
        expected_version = args.expected_version or version_from_header()
        if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+", expected_version):
            raise RuntimeError(
                f"Expected version must have four numeric components: {expected_version}"
            )
        contract = inspect_artifact(
            artifact,
            args.llvm_readobj,
            expected_version,
        )
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Win64 artifact verification failed: {error}", file=sys.stderr)
        return 1

    serialized = json.dumps(contract, indent=2) + "\n"
    if args.json_output:
        output_path = args.json_output.absolute()
        try:
            if output_path.is_symlink():
                raise RuntimeError(
                    f"JSON output must not be a symlink: {output_path}",
                )
            if output_path.exists() and os.path.samefile(output_path, artifact):
                raise RuntimeError(
                    "JSON output must not replace the verified artifact.",
                )
            if output_path.resolve(strict=False) == artifact:
                raise RuntimeError(
                    "JSON output must not replace the verified artifact.",
                )
            output_path.parent.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(
                mode="w",
                encoding="utf-8",
                dir=output_path.parent,
                prefix=f".{output_path.name}.",
                delete=False,
            ) as temporary:
                temporary.write(serialized)
                temporary.flush()
                os.fsync(temporary.fileno())
                temporary_path = Path(temporary.name)
            os.replace(temporary_path, output_path)
        except (OSError, RuntimeError) as error:
            print(f"Unable to write verifier JSON: {error}", file=sys.stderr)
            return 1
        print(f"Verified {artifact}; wrote {output_path}")
    else:
        print(serialized, end="")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
