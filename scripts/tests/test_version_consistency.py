from __future__ import annotations

import json
import re
import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]


def palschema_version() -> str:
    header = (REPOSITORY_ROOT / "version.h").read_text(encoding="utf-8")
    components = []
    for name in ("MAJOR", "MINOR", "REVISION"):
        match = re.search(
            rf"^\s*#define\s+VERSION_{name}\s+([0-9]+)\s*$",
            header,
            re.MULTILINE,
        )
        if match is None:
            raise AssertionError(f"VERSION_{name} is missing from version.h")
        components.append(match.group(1))
    return ".".join(components)


class VersionConsistencyTests(unittest.TestCase):
    def test_packages_and_schema_ids_match_version_header(self) -> None:
        version = palschema_version()
        for manifest in (
            "tools/palschema-tools/package.json",
            "tools/vscode-palschema/package.json",
        ):
            data = json.loads(
                (REPOSITORY_ROOT / manifest).read_text(encoding="utf-8")
            )
            self.assertEqual(data["version"], version, manifest)

        schema_root = REPOSITORY_ROOT / "assets" / "schemas"
        index = json.loads(
            (schema_root / "schema-index.json").read_text(encoding="utf-8")
        )
        self.assertEqual(index["palSchemaVersion"], version)
        for entry in index["schemas"]:
            expected_id = (
                "https://okaetsu.github.io/PalSchema/"
                f"schemas/{version}/{entry['file']}"
            )
            self.assertEqual(entry["id"], expected_id, entry["file"])
            schema_file = schema_root / entry["file"]
            if schema_file.is_file():
                schema = json.loads(schema_file.read_text(encoding="utf-8"))
                self.assertEqual(schema.get("$id"), expected_id, entry["file"])


if __name__ == "__main__":
    unittest.main()
