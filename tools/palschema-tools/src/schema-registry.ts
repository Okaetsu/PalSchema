import { createHash } from "node:crypto";
import { existsSync } from "node:fs";
import { lstat, readFile, readdir, realpath } from "node:fs/promises";
import {
  basename,
  dirname,
  isAbsolute,
  relative,
  resolve,
  sep,
} from "node:path";
import { fileURLToPath } from "node:url";

import type {
  SchemaIndex,
  SchemaIndexEntry,
  SchemaVerification,
} from "./types.js";
import {
  isPathContained,
  resolveContainedPath,
} from "./path-security.js";

const moduleDirectory = dirname(fileURLToPath(import.meta.url));
const packageDirectory = resolve(moduleDirectory, "..");
const bundledSchemaDirectory = resolve(moduleDirectory, "schemas");
const repositorySchemaDirectory = resolve(
  packageDirectory,
  "../../assets/schemas",
);

function defaultSchemaDirectory(): string {
  const configured = process.env.PALSCHEMA_SCHEMA_DIR;
  if (configured) {
    return resolve(configured);
  }
  if (existsSync(resolve(bundledSchemaDirectory, "schema-index.json"))) {
    return bundledSchemaDirectory;
  }
  return repositorySchemaDirectory;
}

function assertSchemaIndex(value: unknown): asserts value is SchemaIndex {
  if (
    typeof value !== "object" ||
    value === null ||
    !("formatVersion" in value) ||
    value.formatVersion !== 1 ||
    !("schemas" in value) ||
    !Array.isArray(value.schemas)
  ) {
    throw new Error("Unsupported or malformed schema-index.json.");
  }

  const files = new Set<string>();
  const ids = new Set<string>();
  for (const candidate of value.schemas as unknown[]) {
    const entry = candidate as Record<string, unknown>;
    if (
      typeof candidate !== "object" ||
      candidate === null ||
      !("id" in candidate) ||
      typeof candidate.id !== "string" ||
      !("file" in candidate) ||
      typeof candidate.file !== "string" ||
      !("folder" in candidate) ||
      (candidate.folder !== null && typeof candidate.folder !== "string") ||
      !("patterns" in candidate) ||
      !Array.isArray(candidate.patterns) ||
      !candidate.patterns.every((pattern) => typeof pattern === "string") ||
      !("generated" in candidate) ||
      typeof candidate.generated !== "boolean" ||
      !("dependencies" in candidate) ||
      !Array.isArray(candidate.dependencies) ||
      !candidate.dependencies.every(
        (dependency) => typeof dependency === "string",
      ) ||
      !("sha256" in candidate) ||
      (candidate.sha256 !== null && typeof candidate.sha256 !== "string")
    ) {
      throw new Error("schema-index.json contains a malformed schema entry.");
    }
    if (!("supportPatterns" in entry)) {
      entry.supportPatterns = [];
    }
    if (
      !Array.isArray(entry.supportPatterns) ||
      !entry.supportPatterns.every(
        (pattern: unknown) => typeof pattern === "string",
      )
    ) {
      throw new Error("schema-index.json contains invalid supportPatterns.");
    }
    assertSingleFileName(candidate.file, "schema file");
    for (const dependency of candidate.dependencies) {
      assertSingleFileName(dependency, "schema dependency");
    }
    for (const pattern of entry.supportPatterns as string[]) {
      assertSupportPattern(pattern);
    }
    if (files.has(candidate.file) || ids.has(candidate.id)) {
      throw new Error("schema-index.json contains duplicate files or IDs.");
    }
    files.add(candidate.file);
    ids.add(candidate.id);
  }

  for (const entry of value.schemas) {
    for (const dependency of entry.dependencies) {
      if (!files.has(dependency)) {
        throw new Error(
          `schema-index.json dependency is not declared: ${dependency}`,
        );
      }
    }
  }
}

function assertSingleFileName(value: string, label: string): void {
  if (
    !value ||
    value === "." ||
    value === ".." ||
    isAbsolute(value) ||
    basename(value) !== value ||
    value.includes("/") ||
    value.includes("\\")
  ) {
    throw new Error(`Invalid ${label} path in schema-index.json: ${value}`);
  }
}

function assertSupportPattern(pattern: string): void {
  const segments = pattern.split("/");
  if (
    segments.length !== 2 ||
    !segments[0] ||
    segments[0] === "." ||
    segments[0] === ".." ||
    segments[0].includes("\\") ||
    segments[1] !== "*.schema.json"
  ) {
    throw new Error(
      `Unsupported schema support pattern in schema-index.json: ${pattern}`,
    );
  }
}

export class SchemaRegistry {
  readonly schemaDirectory: string;
  readonly index: SchemaIndex;
  private readonly supportFileCache = new Map<string, Promise<string[]>>();
  private supportPathIndex: Promise<Map<string, string>> | undefined;

  private constructor(schemaDirectory: string, index: SchemaIndex) {
    this.schemaDirectory = schemaDirectory;
    this.index = index;
  }

  static async load(
    schemaDirectory = defaultSchemaDirectory(),
    options: { verifyStatic?: boolean } = {},
  ): Promise<SchemaRegistry> {
    const absoluteDirectory = resolve(schemaDirectory);
    const indexPath = resolve(absoluteDirectory, "schema-index.json");
    const index = JSON.parse(await readFile(indexPath, "utf8")) as unknown;
    assertSchemaIndex(index);
    const registry = new SchemaRegistry(absoluteDirectory, index);
    if (options.verifyStatic ?? true) {
      await registry.assertStaticIntegrity();
    }
    return registry;
  }

  findForDocument(file: string): SchemaIndexEntry | null {
    const segments = resolve(file).split(sep);
    for (let index = segments.length - 2; index >= 0; index -= 1) {
      const segment = segments[index];
      const entry = this.index.schemas.find(
        (candidate) => candidate.folder === segment,
      );
      if (entry) {
        return entry;
      }
    }
    return null;
  }

  entryForFile(file: string): SchemaIndexEntry | null {
    return this.index.schemas.find((entry) => entry.file === file) ?? null;
  }

  entryForId(id: string): SchemaIndexEntry | null {
    return this.index.schemas.find((entry) => entry.id === id) ?? null;
  }

  pathForRelative(file: string): string {
    return resolveContainedPath(
      this.schemaDirectory,
      file,
      "Schema path escapes its pack",
    );
  }

  pathFor(entry: SchemaIndexEntry): string {
    assertSingleFileName(entry.file, "schema file");
    return this.pathForRelative(entry.file);
  }

  async readSchema(entry: SchemaIndexEntry): Promise<Record<string, unknown>> {
    return JSON.parse(await readFile(this.pathFor(entry), "utf8")) as Record<
      string,
      unknown
    >;
  }

  async supportFilesFor(entry: SchemaIndexEntry): Promise<string[]> {
    const cached = this.supportFileCache.get(entry.id);
    if (cached) {
      return cached;
    }
    const pending = this.discoverSupportFiles(entry);
    this.supportFileCache.set(entry.id, pending);
    try {
      return await pending;
    } catch (error) {
      this.supportFileCache.delete(entry.id);
      throw error;
    }
  }

  private async discoverSupportFiles(
    entry: SchemaIndexEntry,
  ): Promise<string[]> {
    const files: string[] = [];
    for (const pattern of entry.supportPatterns) {
      assertSupportPattern(pattern);
      const directoryName = pattern.split("/")[0] as string;
      const directory = this.pathForRelative(directoryName);
      let children;
      try {
        children = await readdir(directory, { withFileTypes: true });
      } catch (error) {
        if ((error as NodeJS.ErrnoException).code === "ENOENT") {
          continue;
        }
        throw error;
      }
      for (const child of children) {
        if (!child.name.endsWith(".schema.json")) {
          continue;
        }
        const relativeFile = `${directoryName}/${child.name}`;
        const path = this.pathForRelative(relativeFile);
        const metadata = await lstat(path);
        if (!metadata.isFile() || metadata.isSymbolicLink()) {
          throw new Error(`Schema support file must be a regular file: ${path}`);
        }
        const canonical = await realpath(path);
        if (!isPathContained(this.schemaDirectory, canonical)) {
          throw new Error(`Schema support file escapes its pack: ${path}`);
        }
        files.push(relativeFile);
      }
    }
    return [...new Set(files)].sort();
  }

  canonicalIdForSupport(entry: SchemaIndexEntry, file: string): string {
    return new URL(file, entry.id).toString();
  }

  async pathForId(id: string): Promise<string | null> {
    const normalized = new URL(id);
    normalized.hash = "";
    const entry = this.entryForId(normalized.toString());
    if (entry) {
      return this.pathFor(entry);
    }
    this.supportPathIndex ??= this.buildSupportPathIndex();
    const supportPath = (await this.supportPathIndex).get(
      normalized.toString(),
    );
    return supportPath ?? null;
  }

  private async buildSupportPathIndex(): Promise<Map<string, string>> {
    const paths = new Map<string, string>();
    for (const candidate of this.index.schemas) {
      for (const supportFile of await this.supportFilesFor(candidate)) {
        paths.set(
          this.canonicalIdForSupport(candidate, supportFile),
          this.pathForRelative(supportFile),
        );
      }
    }
    return paths;
  }

  async assertStaticIntegrity(): Promise<void> {
    const failures = (await this.verify()).filter(
      (result) =>
        !result.entry.generated &&
        (result.status === "missing" ||
          result.status === "checksum-mismatch"),
    );
    if (failures.length > 0) {
      throw new Error(
        `Static schema integrity check failed: ${failures
          .map((failure) => `${failure.entry.file} (${failure.status})`)
          .join(", ")}`,
      );
    }
  }

  async verify(): Promise<SchemaVerification[]> {
    return Promise.all(
      this.index.schemas.map(async (entry): Promise<SchemaVerification> => {
        const path = this.pathFor(entry);
        if (!existsSync(path)) {
          return {
            entry,
            status: entry.generated ? "missing-generated" : "missing",
          };
        }

        const content = await readFile(path);
        const actualSha256 = createHash("sha256").update(content).digest("hex");
        if (entry.generated && entry.sha256 === null) {
          return { entry, status: "present-generated", actualSha256 };
        }
        if (entry.sha256 !== actualSha256) {
          return { entry, status: "checksum-mismatch", actualSha256 };
        }
        return { entry, status: "verified", actualSha256 };
      }),
    );
  }

  displayPath(path: string): string {
    const relativePath = relative(process.cwd(), path);
    return relativePath && !relativePath.startsWith("..") ? relativePath : path;
  }
}
