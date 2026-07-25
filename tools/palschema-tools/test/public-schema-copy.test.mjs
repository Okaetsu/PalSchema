import assert from "node:assert/strict";
import {
  cp,
  mkdtemp,
  readFile,
  rm,
  writeFile,
} from "node:fs/promises";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";
import test from "node:test";

import { copyPublicSchemas } from "../../../scripts/copy-public-schemas.mjs";

const repositoryRoot = resolve(import.meta.dirname, "../../..");

test("public schema packaging rejects generated Palworld data", async () => {
  const temporaryRoot = await mkdtemp(join(tmpdir(), "palschema-public-pack-"));
  try {
    const source = join(temporaryRoot, "source");
    const destination = join(temporaryRoot, "destination");
    await cp(resolve(repositoryRoot, "assets/schemas"), source, {
      recursive: true,
    });
    await writeFile(
      join(source, "enums.schema.json"),
      '{"definitions":{"CanaryPrivateGameValue":{"enum":["secret"]}}}\n',
    );
    await assert.rejects(
      copyPublicSchemas(source, destination),
      /runtime-generated schema/,
    );
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
});

test("public schema packaging rejects an ancestor destination without deleting it", async () => {
  const temporaryRoot = await mkdtemp(join(tmpdir(), "palschema-public-overlap-"));
  try {
    const source = join(temporaryRoot, "source");
    const sentinel = join(temporaryRoot, "sentinel.txt");
    await cp(resolve(repositoryRoot, "assets/schemas"), source, {
      recursive: true,
    });
    await writeFile(sentinel, "preserve\n");
    await assert.rejects(
      copyPublicSchemas(source, temporaryRoot),
      /must not overlap/,
    );
    assert.equal(await readFile(sentinel, "utf8"), "preserve\n");
  } finally {
    await rm(temporaryRoot, { recursive: true, force: true });
  }
});
