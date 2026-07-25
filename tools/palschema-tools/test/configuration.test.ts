import assert from "node:assert/strict";
import { mkdir, mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";

import { findProjectConfig } from "../src/configuration.js";

test("discovers project config and resolves its schema path", async () => {
  const temporaryRoot = await mkdtemp(join(tmpdir(), "palschema-config-"));
  try {
    const nested = join(temporaryRoot, "mods", "Example", "items");
    await mkdir(nested, { recursive: true });
    await writeFile(
      join(temporaryRoot, "palschema.config.json"),
      `{
        // JSONC is accepted for a friendlier checked-in config.
        "schemaDirectory": ".palschema/schemas",
        "allowMissingGenerated": true,
      }\n`,
    );

    const config = await findProjectConfig(nested);
    assert.equal(
      config?.schemaDirectory,
      join(temporaryRoot, ".palschema", "schemas"),
    );
    assert.equal(config?.allowMissingGenerated, true);
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
});
