import assert from "node:assert/strict";
import {
  cp,
  mkdir,
  mkdtemp,
  readFile,
  rm,
  writeFile,
} from "node:fs/promises";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";
import test from "node:test";

import { PalSchemaValidator } from "../src/validator.js";
import { SchemaRegistry } from "../src/schema-registry.js";

const repositoryRoot = resolve(import.meta.dirname, "../../..");
const schemaDirectory = resolve(repositoryRoot, "assets/schemas");

test("validates a checked-in JSON example with offline enum fallback", async () => {
  const validator = await PalSchemaValidator.create(schemaDirectory, {
    allowMissingGenerated: true,
  });
  const result = await validator.validateFile(
    resolve(
      repositoryRoot,
      "assets/examples/ExampleMod/items/example_items.json",
    ),
  );

  assert.equal(
    result.diagnostics.some((diagnostic) => diagnostic.severity === "error"),
    false,
  );
  assert.equal(
    result.diagnostics.some(
      (diagnostic) => diagnostic.code === "PS_SCHEMA_ENUMS_MISSING",
    ),
    true,
  );
});

test("requires runtime-generated enum schemas in strict mode", async () => {
  const validator = await PalSchemaValidator.create(schemaDirectory);
  const result = await validator.validateFile(
    resolve(
      repositoryRoot,
      "assets/examples/ExampleMod/items/example_items.json",
    ),
  );

  const diagnostic = result.diagnostics.find(
    (item) => item.code === "PS_SCHEMA_ENUMS_MISSING",
  );
  assert.ok(diagnostic);
  assert.equal(diagnostic.severity, "error");
});

test("reports schema errors at a stable source position", async () => {
  const temporaryRoot = await mkdtemp(join(tmpdir(), "palschema-tools-"));
  try {
    const itemsDirectory = join(temporaryRoot, "items");
    await mkdir(itemsDirectory);
    const file = join(itemsDirectory, "invalid.jsonc");
    await writeFile(file, '{\n  "Broken": { "Rarity": 99 }\n}\n');

    const validator = await PalSchemaValidator.create(schemaDirectory);
    const result = await validator.validateFile(file);
    const diagnostic = result.diagnostics.find(
      (item) => item.code === "PS_SCHEMA_MAXIMUM",
    );
    assert.ok(diagnostic);
    assert.equal(diagnostic.line, 2);
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
});

test("requires runtime-generated raw schemas explicitly", async () => {
  const validator = await PalSchemaValidator.create(schemaDirectory);
  const result = await validator.validateText(
    "{}",
    resolve(repositoryRoot, "fixture/raw/example.json"),
  );

  assert.equal(result.diagnostics[0]?.code, "PS_SCHEMA_GENERATED_MISSING");
  assert.equal(result.diagnostics[0]?.severity, "error");
});

test("can downgrade missing generated schemas for offline repository checks", async () => {
  const validator = await PalSchemaValidator.create(schemaDirectory, {
    allowMissingGenerated: true,
  });
  const result = await validator.validateText(
    "{}",
    resolve(repositoryRoot, "fixture/raw/example.jsonc"),
  );

  assert.equal(result.diagnostics[0]?.code, "PS_SCHEMA_GENERATED_MISSING");
  assert.equal(result.diagnostics[0]?.severity, "warning");
});

test("accepts an unpinned local runtime-generated schema", async () => {
  const temporaryRoot = await mkdtemp(join(tmpdir(), "palschema-schemas-"));
  try {
    const sourceIndex = JSON.parse(
      await readFile(resolve(schemaDirectory, "schema-index.json"), "utf8"),
    );
    await writeFile(
      join(temporaryRoot, "schema-index.json"),
      `${JSON.stringify(sourceIndex)}\n`,
    );
    await writeFile(
      join(temporaryRoot, "raw.schema.json"),
      '{"$schema":"http://json-schema.org/draft-07/schema#","type":"object"}\n',
    );

    const registry = await SchemaRegistry.load(temporaryRoot, {
      verifyStatic: false,
    });
    const verification = await registry.verify();
    assert.equal(
      verification.find((result) => result.entry.file === "raw.schema.json")
        ?.status,
      "present-generated",
    );
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
});

test("rejects a modified static schema before validation", async () => {
  const temporaryRoot = await mkdtemp(join(tmpdir(), "palschema-integrity-"));
  try {
    await cp(schemaDirectory, temporaryRoot, { recursive: true });
    await writeFile(
      join(temporaryRoot, "items.schema.json"),
      '{"type":"object"}\n',
    );
    await assert.rejects(
      PalSchemaValidator.create(temporaryRoot),
      /Static schema integrity check failed.*items\.schema\.json/,
    );
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
});

test("loads generated raw table support schemas by canonical ID", async () => {
  const temporaryRoot = await mkdtemp(join(tmpdir(), "palschema-raw-pack-"));
  try {
    await cp(schemaDirectory, temporaryRoot, { recursive: true });
    await mkdir(join(temporaryRoot, "raw"));
    await writeFile(
      join(temporaryRoot, "raw.schema.json"),
      JSON.stringify({
        $schema: "http://json-schema.org/draft-07/schema#",
        type: "object",
        properties: {
          ExampleTable: { $ref: "raw/ExampleTable.schema.json" },
        },
        additionalProperties: false,
      }),
    );
    await writeFile(
      join(temporaryRoot, "raw", "ExampleTable.schema.json"),
      JSON.stringify({
        type: "object",
        additionalProperties: {
          type: "object",
          required: ["Value"],
          properties: { Value: { type: "string" } },
        },
      }),
    );

    const validator = await PalSchemaValidator.create(temporaryRoot, {
      allowMissingGenerated: true,
    });
    const result = await validator.validateText(
      '{"ExampleTable":{"Row":{"Value":"ok"}}}',
      resolve(repositoryRoot, "fixture/raw/example.json"),
    );
    assert.equal(
      result.diagnostics.some((diagnostic) => diagnostic.severity === "error"),
      false,
    );
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
});
