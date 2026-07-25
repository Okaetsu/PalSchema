from __future__ import annotations

import subprocess
import struct
import sys
import tempfile
import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
VERIFIER = REPOSITORY_ROOT / "scripts" / "verify-win64-artifact.py"


def align4(content: bytes) -> bytes:
    return content + b"\0" * (-len(content) % 4)


def version_node(
    key: str,
    *,
    value: bytes | str = b"",
    value_type: int = 0,
    children: tuple[bytes, ...] = (),
) -> bytes:
    encoded_key = f"{key}\0".encode("utf-16-le")
    if isinstance(value, str):
        encoded_value = f"{value}\0".encode("utf-16-le")
        value_length = len(encoded_value) // 2
    else:
        encoded_value = value
        value_length = len(encoded_value)
    content = align4(b"\0" * 6 + encoded_key)
    content += encoded_value
    content = align4(content)
    content += b"".join(align4(child) for child in children)
    return struct.pack("<HHH", len(content), value_length, value_type) + content[6:]


def version_resource(
    product_name: str,
    product_version: str,
    *,
    fixed_version: str | None = None,
    comments: str | None = None,
) -> bytes:
    components = [
        int(value)
        for value in (fixed_version or product_version).split(".")
    ]
    product_ms = (components[0] << 16) | components[1]
    product_ls = (components[2] << 16) | components[3]
    fixed = struct.pack(
        "<13I",
        0xFEEF04BD,
        0x00010000,
        product_ms,
        product_ls,
        product_ms,
        product_ls,
        0x3F,
        0,
        0x40004,
        1,
        0,
        0,
        0,
    )
    strings = [
        version_node("ProductName", value=product_name, value_type=1),
        version_node("ProductVersion", value=product_version, value_type=1),
    ]
    if comments is not None:
        strings.append(version_node("Comments", value=comments, value_type=1))
    table = version_node("040904B0", children=tuple(strings))
    string_file_info = version_node("StringFileInfo", children=(table,))
    return version_node(
        "VS_VERSION_INFO",
        value=fixed,
        children=(string_file_info,),
    )


def resource_dump(
    product_name: str,
    product_version: str,
    *,
    fixed_version: str | None = None,
    comments: str | None = None,
) -> str:
    content = version_resource(
        product_name,
        product_version,
        fixed_version=fixed_version,
        comments=comments,
    )
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
    *,
    fixed_version: str | None = None,
    comments: str | None = None,
) -> str:
    return f"""\
File: fixture.dll
Format: COFF-x86-64
ImageFileHeader {{
  Characteristics [ (0x2022)
    IMAGE_FILE_DLL
  ]
}}
ImageOptionalHeader {{
  Characteristics [ (0x8160)
    IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE
    IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA
    IMAGE_DLL_CHARACTERISTICS_NX_COMPAT
  ]
}}
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
{resource_dump(product_name, product_version, fixed_version=fixed_version, comments=comments)}
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
        completed = self.run_verifier(
            readobj_output(product_name="OtherMod!", comments="PalSchema"),
        )
        self.assertEqual(completed.returncode, 1)
        self.assertIn("product identity", completed.stderr)

    def test_rejects_wrong_fixed_product_version_despite_decoy_string(self) -> None:
        completed = self.run_verifier(
            readobj_output(fixed_version="9.9.9.9"),
        )
        self.assertEqual(completed.returncode, 1)
        self.assertIn("VS_FIXEDFILEINFO product version differs", completed.stderr)

    def test_rejects_each_broken_pe_contract_field(self) -> None:
        valid = readobj_output()
        mutations = {
            "architecture": (
                valid.replace("Format: COFF-x86-64", "Format: COFF-i386"),
                "not COFF x86-64",
            ),
            "dll flag": (
                valid.replace("    IMAGE_FILE_DLL\n", "")
                + "Import {\n  Name: IMAGE_FILE_DLL\n}\n",
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
                valid.replace(f"    {characteristic}\n", "")
                + f"Import {{\n  Name: {characteristic}\n}}\n",
                "missing DLL security characteristics",
            )

        for name, (output, expected_error) in mutations.items():
            with self.subTest(name=name):
                completed = self.run_verifier(output)
                self.assertEqual(completed.returncode, 1)
                self.assertIn(expected_error, completed.stderr)

    def test_json_output_cannot_replace_artifact_or_follow_symlink(self) -> None:
        with tempfile.TemporaryDirectory(prefix="palschema-pe-output-") as directory:
            root = Path(directory)
            artifact = root / "fixture.dll"
            artifact.write_bytes(b"fixture")
            protected = root / "protected.json"
            protected.write_text("sentinel\n", encoding="utf-8")
            linked_output = root / "contract.json"
            linked_output.symlink_to(protected)
            readobj = root / "llvm-readobj"
            readobj.write_text(
                "#!/usr/bin/env sh\ncat <<'EOF'\n"
                f"{readobj_output()}"
                "EOF\n",
                encoding="utf-8",
            )
            readobj.chmod(0o755)

            for output in (artifact, linked_output):
                with self.subTest(output=output.name):
                    completed = subprocess.run(
                        [
                            sys.executable,
                            str(VERIFIER),
                            str(artifact),
                            "--llvm-readobj",
                            str(readobj),
                            "--json-output",
                            str(output),
                        ],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    self.assertEqual(completed.returncode, 1)
            self.assertEqual(artifact.read_bytes(), b"fixture")
            self.assertEqual(protected.read_text(encoding="utf-8"), "sentinel\n")


if __name__ == "__main__":
    unittest.main()
