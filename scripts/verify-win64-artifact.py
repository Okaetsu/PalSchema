#!/usr/bin/env python3

"""Verify and describe the stable PE contract of a PalSchema Win64 DLL."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
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


def has_version_string(resource: bytes, key: str, value: str) -> bool:
    key_marker = f"{key}\0".encode("utf-16-le")
    value_marker = f"{value}\0".encode("utf-16-le")
    key_offset = resource.find(key_marker)
    if key_offset < 0:
        return False
    value_offset = resource.find(
        value_marker,
        key_offset + len(key_marker),
        key_offset + len(key_marker) + 512,
    )
    return value_offset >= 0


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

    missing_characteristics = sorted(
        characteristic
        for characteristic in REQUIRED_DLL_CHARACTERISTICS
        if characteristic not in output
    )
    errors: list[str] = []
    if "Format: COFF-x86-64" not in output:
        errors.append("artifact is not COFF x86-64")
    if "IMAGE_FILE_DLL" not in output:
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
    if not has_version_string(
        resource_bytes,
        "ProductName",
        EXPECTED_PRODUCT_NAME,
    ):
        errors.append(
            f"VERSIONINFO product identity {EXPECTED_PRODUCT_NAME!r} is missing"
        )
    if not has_version_string(
        resource_bytes,
        "ProductVersion",
        expected_version,
    ):
        errors.append(
            f"VERSIONINFO product version {expected_version!r} is missing"
        )
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
        output_path = args.json_output.resolve()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(serialized, encoding="utf-8")
        print(f"Verified {artifact}; wrote {output_path}")
    else:
        print(serialized, end="")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
