import assert from "node:assert/strict";
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { dirname, resolve } from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import test from "node:test";

const testDirectory = dirname(fileURLToPath(import.meta.url));
const packageRoot = resolve(testDirectory, "..");
const cliSource = resolve(packageRoot, "src/cli.ts");
const tsxImport = import.meta.resolve("tsx");

function runCli(arguments_, cwd) {
  return spawnSync(
    process.execPath,
    ["--import", tsxImport, cliSource, ...arguments_],
    {
      cwd,
      encoding: "utf8",
    },
  );
}

test("init creates a workspace that validate can check without support-file noise", () => {
  const workspace = mkdtempSync(resolve(tmpdir(), "palschema-cli-"));
  try {
    const initialization = runCli(["init", "."], workspace);
    assert.equal(initialization.status, 0, initialization.stderr);

    const itemsDirectory = resolve(workspace, "items");
    mkdirSync(itemsDirectory);
    writeFileSync(
      resolve(itemsDirectory, "example.jsonc"),
      '{\n  "Example": {\n    "Type": "Generic",\n    "IconTexture": "/Game/Test/T_Test.T_Test",\n    "TypeA": "Any",\n    "TypeB": "Any",\n    "Rank": 0,\n    "Rarity": 0,\n    "MaxStackCount": 1\n  }\n}\n',
    );

    const validation = runCli(["validate", "."], workspace);
    assert.equal(validation.status, 0, validation.stderr);
    assert.match(validation.stdout, /PS_SCHEMA_ENUMS_MISSING.*warning/);
    assert.match(validation.stdout, /Validated 1 file\(s\): 0 error\(s\), 1 warning\(s\)\./);

    const validationFromOutside = runCli(
      ["validate", workspace],
      packageRoot,
    );
    assert.equal(validationFromOutside.status, 0, validationFromOutside.stderr);
    assert.match(
      validationFromOutside.stdout,
      /Validated 1 file\(s\): 0 error\(s\), 1 warning\(s\)\./,
    );
  } finally {
    rmSync(workspace, { recursive: true, force: true });
  }
});
