#!/usr/bin/env python3

"""Verify and describe the stable PE contract of a PalSchema Win64 DLL."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path


EXPECTED_EXPORTS = {"start_mod", "uninstall_mod"}
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
    return parser.parse_args()


def inspect_artifact(artifact: Path, llvm_readobj: str) -> dict[str, object]:
    executable = shutil.which(llvm_readobj)
    if executable is None:
        raise RuntimeError(f"Unable to find {llvm_readobj!r} on PATH.")

    completed = subprocess.run(
        [
            executable,
            "--file-headers",
            "--coff-exports",
            "--coff-imports",
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
        contract = inspect_artifact(artifact, args.llvm_readobj)
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
