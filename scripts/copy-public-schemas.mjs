#!/usr/bin/env node

import { createHash } from "node:crypto";
import { existsSync } from "node:fs";
import {
  copyFile,
  lstat,
  mkdir,
  readFile,
  readdir,
  realpath,
  rm,
} from "node:fs/promises";
import {
  basename,
  dirname,
  isAbsolute,
  relative,
  resolve,
  sep,
} from "node:path";
import { fileURLToPath } from "node:url";

function assertSingleFileName(value, label) {
  if (
    typeof value !== "string" ||
    !value ||
    value === "." ||
    value === ".." ||
    isAbsolute(value) ||
    basename(value) !== value ||
    value.includes("/") ||
    value.includes("\\")
  ) {
    throw new Error(`Invalid ${label}: ${String(value)}`);
  }
}

function containedPath(root, relativeFile) {
  const candidate = resolve(root, relativeFile);
  const fromRoot = relative(root, candidate);
  if (
    !fromRoot ||
    fromRoot === ".." ||
    fromRoot.startsWith(`..${sep}`) ||
    isAbsolute(fromRoot)
  ) {
    throw new Error(`Schema path escapes its pack: ${relativeFile}`);
  }
  return candidate;
}

function pathsOverlap(left, right) {
  const fromLeft = relative(left, right);
  const fromRight = relative(right, left);
  return (
    fromLeft === "" ||
    (!fromLeft.startsWith(`..${sep}`) && fromLeft !== ".." && !isAbsolute(fromLeft)) ||
    (!fromRight.startsWith(`..${sep}`) && fromRight !== ".." && !isAbsolute(fromRight))
  );
}

async function canonicalPotentialPath(path) {
  const missingSegments = [];
  let candidate = path;
  for (;;) {
    try {
      return resolve(
        await realpath(candidate),
        ...missingSegments.reverse(),
      );
    } catch (error) {
      if (error?.code !== "ENOENT") {
        throw error;
      }
      const parent = dirname(candidate);
      if (parent === candidate) {
        throw error;
      }
      missingSegments.push(basename(candidate));
      candidate = parent;
    }
  }
}

async function rejectGeneratedContent(source, entry) {
  const generatedPath = containedPath(source, entry.file);
  if (existsSync(generatedPath)) {
    throw new Error(
      `Refusing to package runtime-generated schema: ${entry.file}`,
    );
  }
  for (const pattern of entry.supportPatterns ?? []) {
    const match = /^([^/]+)\/\*\.schema\.json$/.exec(pattern);
    if (!match) {
      throw new Error(`Unsupported schema support pattern: ${pattern}`);
    }
    const directory = containedPath(source, match[1]);
    if (!existsSync(directory)) {
      continue;
    }
    const generatedFiles = (await readdir(directory)).filter((file) =>
      file.endsWith(".schema.json"),
    );
    if (generatedFiles.length > 0) {
      throw new Error(
        `Refusing to package runtime-generated schema support files below ${match[1]}/`,
      );
    }
  }
}

export async function copyPublicSchemas(sourceDirectory, destinationDirectory) {
  const source = resolve(sourceDirectory);
  const destination = resolve(destinationDirectory);
  if (
    pathsOverlap(source, destination) ||
    pathsOverlap(
      await realpath(source),
      await canonicalPotentialPath(destination),
    )
  ) {
    throw new Error("Schema source and destination must not overlap.");
  }

  const indexPath = resolve(source, "schema-index.json");
  const index = JSON.parse(await readFile(indexPath, "utf8"));
  if (
    index?.formatVersion !== 1 ||
    !Array.isArray(index.schemas)
  ) {
    throw new Error("Unsupported or malformed schema-index.json.");
  }

  const staticEntries = [];
  for (const entry of index.schemas) {
    assertSingleFileName(entry.file, "schema file");
    if (entry.generated) {
      await rejectGeneratedContent(source, entry);
      continue;
    }
    if (
      typeof entry.sha256 !== "string" ||
      !/^[0-9a-f]{64}$/.test(entry.sha256)
    ) {
      throw new Error(`Static schema has no valid checksum: ${entry.file}`);
    }
    const schemaPath = containedPath(source, entry.file);
    const metadata = await lstat(schemaPath);
    if (!metadata.isFile() || metadata.isSymbolicLink()) {
      throw new Error(`Static schema must be a regular file: ${entry.file}`);
    }
    const content = await readFile(schemaPath);
    const digest = createHash("sha256").update(content).digest("hex");
    if (digest !== entry.sha256) {
      throw new Error(`Static schema checksum mismatch: ${entry.file}`);
    }
    staticEntries.push({ entry, schemaPath });
  }

  await rm(destination, { recursive: true, force: true });
  await mkdir(destination, { recursive: true });
  await copyFile(indexPath, resolve(destination, "schema-index.json"));
  for (const { entry, schemaPath } of staticEntries) {
    const output = containedPath(destination, entry.file);
    await mkdir(dirname(output), { recursive: true });
    await copyFile(schemaPath, output);
  }
}

const invokedPath = process.argv[1] ? resolve(process.argv[1]) : "";
if (invokedPath === fileURLToPath(import.meta.url)) {
  const [source, destination, ...extra] = process.argv.slice(2);
  if (!source || !destination || extra.length > 0) {
    console.error(
      "Usage: scripts/copy-public-schemas.mjs SOURCE_DIR DESTINATION_DIR",
    );
    process.exitCode = 2;
  } else {
    try {
      await copyPublicSchemas(source, destination);
    } catch (error) {
      console.error(error instanceof Error ? error.message : error);
      process.exitCode = 1;
    }
  }
}
