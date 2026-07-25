from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
VERIFIER = REPOSITORY_ROOT / "scripts" / "verify-win64-artifact.py"


def resource_dump(product_name: str, product_version: str) -> str:
    content = (
        f"ProductName\0{product_name}\0"
        f"ProductVersion\0{product_version}\0"
    ).encode("utf-16-le")
    content += b"\0" * (-len(content) % 4)
    groups = [
        content[index : index + 4].hex().upper()
        for index in range(0, len(content), 4)
    ]
    rows = []
    for index in range(0, len(groups), 4):
        rows.append(
            f"    {index * 4:04X}: {' '.join(groups[index:index + 4])} |data|"
        )
    return "\n".join(rows)


def readobj_output(
    product_name: str = "PalSchema",
    product_version: str = "0.6.1.0",
) -> str:
    return f"""\
File: fixture.dll
Format: COFF-x86-64
Characteristics [ (0x2022)
  IMAGE_FILE_DLL
]
DLLCharacteristics [ (0x8160)
  IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE
  IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA
  IMAGE_DLL_CHARACTERISTICS_NX_COMPAT
]
Export {{
  Name: start_mod
}}
Export {{
  Name: uninstall_mod
}}
Import {{
  Name: UE4SS.dll
}}
Resources [
  Type: VERSIONINFO (ID 16) [
    Data (
{resource_dump(product_name, product_version)}
    )
  ]
]
"""


class VerifyWin64ArtifactTest(unittest.TestCase):
    def run_verifier(
        self,
        output: str | None = None,
    ) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory(prefix="palschema-pe-test-") as directory:
            root = Path(directory)
            artifact = root / "fixture.dll"
            artifact.write_bytes(b"fixture")
            readobj = root / "llvm-readobj"
            readobj.write_text(
                "#!/usr/bin/env sh\n"
                "cat <<'EOF'\n"
                f"{output or readobj_output()}"
                "EOF\n",
                encoding="utf-8",
            )
            readobj.chmod(0o755)
            return subprocess.run(
                [
                    sys.executable,
                    str(VERIFIER),
                    str(artifact),
                    "--llvm-readobj",
                    str(readobj),
                ],
                capture_output=True,
                text=True,
                check=False,
            )

    def test_accepts_project_identity_and_version(self) -> None:
        completed = self.run_verifier()
        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn('"product_name": "PalSchema"', completed.stdout)

    def test_rejects_an_unrelated_generic_ue4ss_mod(self) -> None:
        completed = self.run_verifier(readobj_output(product_name="OtherMod!"))
        self.assertEqual(completed.returncode, 1)
        self.assertIn("product identity", completed.stderr)

    def test_rejects_each_broken_pe_contract_field(self) -> None:
        valid = readobj_output()
        mutations = {
            "architecture": (
                valid.replace("Format: COFF-x86-64", "Format: COFF-i386"),
                "not COFF x86-64",
            ),
            "dll flag": (
                valid.replace("  IMAGE_FILE_DLL\n", ""),
                "not marked as a DLL",
            ),
            "export": (
                valid.replace("Export {\n  Name: uninstall_mod\n}\n", ""),
                "exports differ",
            ),
            "UE4SS import": (
                valid.replace("Import {\n  Name: UE4SS.dll\n}\n", ""),
                "UE4SS.dll import is missing",
            ),
            "VERSIONINFO": (
                valid.replace("Type: VERSIONINFO", "Type: OTHER"),
                "VERSIONINFO resource is missing",
            ),
            "product version": (
                readobj_output(product_version="9.9.9.9"),
                "product version",
            ),
        }
        for characteristic in (
            "IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE",
            "IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA",
            "IMAGE_DLL_CHARACTERISTICS_NX_COMPAT",
        ):
            mutations[characteristic] = (
                valid.replace(f"  {characteristic}\n", ""),
                "missing DLL security characteristics",
            )

        for name, (output, expected_error) in mutations.items():
            with self.subTest(name=name):
                completed = self.run_verifier(output)
                self.assertEqual(completed.returncode, 1)
                self.assertIn(expected_error, completed.stderr)


if __name__ == "__main__":
    unittest.main()
