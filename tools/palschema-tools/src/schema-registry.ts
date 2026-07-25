import { createHash } from "node:crypto";
import { existsSync } from "node:fs";
import { readFile } from "node:fs/promises";
import { dirname, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

import type {
  SchemaIndex,
  SchemaIndexEntry,
  SchemaVerification,
} from "./types.js";

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
}

export class SchemaRegistry {
  readonly schemaDirectory: string;
  readonly index: SchemaIndex;

  private constructor(schemaDirectory: string, index: SchemaIndex) {
    this.schemaDirectory = schemaDirectory;
    this.index = index;
  }

  static async load(schemaDirectory = defaultSchemaDirectory()): Promise<SchemaRegistry> {
    const absoluteDirectory = resolve(schemaDirectory);
    const indexPath = resolve(absoluteDirectory, "schema-index.json");
    const index = JSON.parse(await readFile(indexPath, "utf8")) as unknown;
    assertSchemaIndex(index);
    return new SchemaRegistry(absoluteDirectory, index);
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

  pathFor(entry: SchemaIndexEntry): string {
    return resolve(this.schemaDirectory, entry.file);
  }

  async readSchema(entry: SchemaIndexEntry): Promise<Record<string, unknown>> {
    return JSON.parse(await readFile(this.pathFor(entry), "utf8")) as Record<
      string,
      unknown
    >;
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
