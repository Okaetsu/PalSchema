import assert from "node:assert/strict";
import {
  cpSync,
  existsSync,
  mkdirSync,
  mkdtempSync,
  readFileSync,
  rmSync,
  symlinkSync,
  writeFileSync,
} from "node:fs";
import { tmpdir } from "node:os";
import { dirname, resolve } from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import test from "node:test";

const testDirectory = dirname(fileURLToPath(import.meta.url));
const packageRoot = resolve(testDirectory, "..");
const repositoryRoot = resolve(packageRoot, "../..");
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

test("validation resolves configuration from each explicit target", () => {
  const temporaryRoot = mkdtempSync(resolve(tmpdir(), "palschema-config-target-"));
  try {
    const caller = resolve(temporaryRoot, "caller");
    const target = resolve(temporaryRoot, "target");
    mkdirSync(caller);
    mkdirSync(resolve(target, "items"), { recursive: true });
    writeFileSync(
      resolve(caller, "palschema.config.json"),
      '{"schemaDirectory":"missing-schemas"}\n',
    );
    writeFileSync(
      resolve(target, "palschema.config.json"),
      '{"allowMissingGenerated":true}\n',
    );
    writeFileSync(
      resolve(target, "items", "example.json"),
      '{"Example":{"Type":"Generic","IconTexture":"/Game/Test/T.T","TypeA":"Any","TypeB":"Any","Rank":0,"Rarity":0,"MaxStackCount":1}}\n',
    );

    const validation = runCli(["validate", target], caller);
    assert.equal(validation.status, 0, validation.stderr);
    assert.match(validation.stdout, /PS_SCHEMA_ENUMS_MISSING.*warning/);
  } finally {
    rmSync(temporaryRoot, { recursive: true, force: true });
  }
});

test("recursive discovery terminates across a directory symlink cycle", () => {
  const workspace = mkdtempSync(resolve(tmpdir(), "palschema-cycle-"));
  try {
    mkdirSync(resolve(workspace, "items"));
    writeFileSync(resolve(workspace, "items", "data.json"), "{}\n");
    symlinkSync(workspace, resolve(workspace, "items", "cycle"), "dir");
    const validation = runCli(
      ["validate", "--allow-missing-generated", workspace],
      packageRoot,
    );
    assert.equal(validation.status, 0, validation.stderr);
    assert.match(validation.stdout, /Validated 1 file\(s\)/);
  } finally {
    rmSync(workspace, { recursive: true, force: true });
  }
});

test("init rejects schema-index traversal before copying", () => {
  const temporaryRoot = mkdtempSync(resolve(tmpdir(), "palschema-init-path-"));
  try {
    const schemas = resolve(temporaryRoot, "schemas");
    const workspace = resolve(temporaryRoot, "workspace");
    cpSync(resolve(repositoryRoot, "assets/schemas"), schemas, {
      recursive: true,
    });
    mkdirSync(workspace);
    const indexPath = resolve(schemas, "schema-index.json");
    const index = JSON.parse(readFileSync(indexPath, "utf8"));
    index.schemas[0].file = "../../escaped.json";
    writeFileSync(indexPath, `${JSON.stringify(index)}\n`);

    const initialization = runCli(
      ["init", "--schema-dir", schemas, workspace],
      packageRoot,
    );
    assert.equal(initialization.status, 2);
    assert.match(initialization.stderr, /Invalid schema file path/);
  } finally {
    rmSync(temporaryRoot, { recursive: true, force: true });
  }
});

test("init copies generated raw schema support files", () => {
  const temporaryRoot = mkdtempSync(resolve(tmpdir(), "palschema-init-raw-"));
  try {
    const schemas = resolve(temporaryRoot, "schemas");
    const workspace = resolve(temporaryRoot, "workspace");
    cpSync(resolve(repositoryRoot, "assets/schemas"), schemas, {
      recursive: true,
    });
    mkdirSync(resolve(schemas, "raw"));
    mkdirSync(workspace);
    writeFileSync(
      resolve(schemas, "raw.schema.json"),
      '{"type":"object","properties":{"Table":{"$ref":"raw/Table.schema.json"}}}\n',
    );
    writeFileSync(
      resolve(schemas, "raw", "Table.schema.json"),
      '{"type":"object"}\n',
    );

    const initialization = runCli(
      ["init", "--schema-dir", schemas, workspace],
      packageRoot,
    );
    assert.equal(initialization.status, 0, initialization.stderr);
    assert.equal(
      existsSync(
        resolve(
          workspace,
          ".palschema",
          "schemas",
          "raw",
          "Table.schema.json",
        ),
      ),
      true,
    );
  } finally {
    rmSync(temporaryRoot, { recursive: true, force: true });
  }
});

test("init refuses a symlinked managed parent", () => {
  const temporaryRoot = mkdtempSync(resolve(tmpdir(), "palschema-init-link-"));
  try {
    const workspace = resolve(temporaryRoot, "workspace");
    const outside = resolve(temporaryRoot, "outside");
    mkdirSync(workspace);
    mkdirSync(outside);
    symlinkSync(outside, resolve(workspace, ".palschema"), "dir");

    const initialization = runCli(["init", workspace], packageRoot);
    assert.equal(initialization.status, 2);
    assert.match(initialization.stderr, /symlink/);
  } finally {
    rmSync(temporaryRoot, { recursive: true, force: true });
  }
});

test("init --force refuses symlinked managed files", () => {
  for (const relativeTarget of [
    ".vscode/settings.json",
    "palschema.config.json",
  ]) {
    const temporaryRoot = mkdtempSync(
      resolve(tmpdir(), "palschema-init-file-link-"),
    );
    try {
      const workspace = resolve(temporaryRoot, "workspace");
      const outside = resolve(temporaryRoot, "outside.txt");
      mkdirSync(workspace);
      if (relativeTarget.startsWith(".vscode/")) {
        mkdirSync(resolve(workspace, ".vscode"));
      }
      writeFileSync(outside, "sentinel\n");
      symlinkSync(outside, resolve(workspace, relativeTarget), "file");

      const initialization = runCli(
        ["init", "--force", workspace],
        packageRoot,
      );
      assert.equal(initialization.status, 2);
      assert.match(initialization.stderr, /symlink/);
      assert.equal(readFileSync(outside, "utf8"), "sentinel\n");
    } finally {
      rmSync(temporaryRoot, { recursive: true, force: true });
    }
  }
});
