import { readFile } from "node:fs/promises";
import { basename } from "node:path";

import {
  Ajv,
  type ErrorObject,
  type ValidateFunction,
} from "ajv";
import { findNodeAtLocation, type Node as JsonNode } from "jsonc-parser";

import { parseJsoncDocument } from "./jsonc-document.js";
import { SchemaRegistry } from "./schema-registry.js";
import { positionAt } from "./text-position.js";
import type {
  AjvDiagnosticContext,
  PalSchemaDiagnostic,
  SchemaIndexEntry,
  ValidationResult,
} from "./types.js";

function decodeJsonPointer(pointer: string): Array<string | number> {
  if (!pointer) {
    return [];
  }
  return pointer
    .slice(1)
    .split("/")
    .map((segment) => segment.replaceAll("~1", "/").replaceAll("~0", "~"))
    .map((segment) => (/^(0|[1-9][0-9]*)$/.test(segment) ? Number(segment) : segment));
}

function nodeForError(
  root: JsonNode | undefined,
  error: ErrorObject,
): JsonNode | undefined {
  if (!root) {
    return undefined;
  }
  const path = decodeJsonPointer(error.instancePath);
  const node = findNodeAtLocation(root, path);
  if (error.keyword === "required" && node) {
    return node;
  }
  return node ?? root;
}

function ajvDiagnostic(context: AjvDiagnosticContext): PalSchemaDiagnostic {
  const { error, text, root, file } = context;
  const node = nodeForError(root, error);
  const location = positionAt(text, node?.offset ?? 0, node?.length ?? 1);
  let message = error.message ?? "Schema validation failed.";
  if (error.keyword === "required") {
    const missingProperty = String(error.params.missingProperty);
    message = `Missing required property ${JSON.stringify(missingProperty)}.`;
  }

  return {
    ...location,
    code: `PS_SCHEMA_${error.keyword.toUpperCase()}`,
    severity: "error",
    message,
    file,
    source: "palschema",
    schemaPath: error.schemaPath,
    instancePath: error.instancePath,
  };
}

function referencedEnumDefinitions(schema: unknown): string[] {
  const serialized = JSON.stringify(schema);
  const definitions = new Set<string>();
  const expression = /enums\.schema\.json#\/definitions\/([^"\\]+)/g;
  for (const match of serialized.matchAll(expression)) {
    const definition = match[1];
    if (definition) {
      definitions.add(definition);
    }
  }
  return [...definitions].sort();
}

export class PalSchemaValidator {
  readonly registry: SchemaRegistry;
  private readonly validators = new Map<string, ValidateFunction>();
  private readonly fallbackEnumEntries = new Set<string>();
  private readonly allowMissingGenerated: boolean;

  private constructor(registry: SchemaRegistry, allowMissingGenerated: boolean) {
    this.registry = registry;
    this.allowMissingGenerated = allowMissingGenerated;
  }

  static async create(
    schemaDirectory?: string,
    options: { allowMissingGenerated?: boolean } = {},
  ): Promise<PalSchemaValidator> {
    return new PalSchemaValidator(
      await SchemaRegistry.load(schemaDirectory),
      options.allowMissingGenerated ?? false,
    );
  }

  private async compile(entry: SchemaIndexEntry): Promise<ValidateFunction> {
    const existing = this.validators.get(entry.id);
    if (existing) {
      return existing;
    }

    const schema = await this.registry.readSchema(entry);
    const ajv = new Ajv({
      allErrors: true,
      strict: false,
      validateFormats: false,
    });

    if (entry.dependencies.includes("enums.schema.json")) {
      const enumsEntry = this.registry.entryForFile("enums.schema.json");
      if (!enumsEntry) {
        throw new Error("schema-index.json does not declare enums.schema.json.");
      }

      try {
        ajv.addSchema(await this.registry.readSchema(enumsEntry), enumsEntry.id);
      } catch (error) {
        if ((error as NodeJS.ErrnoException).code !== "ENOENT") {
          throw error;
        }
        this.fallbackEnumEntries.add(entry.id);
        const definitions = Object.fromEntries(
          referencedEnumDefinitions(schema).map((name) => [name, {}]),
        );
        ajv.addSchema(
          {
            $schema: "http://json-schema.org/draft-07/schema#",
            $id: enumsEntry.id,
            definitions,
          },
          enumsEntry.id,
        );
      }
    }

    const validate = ajv.compile(schema);
    this.validators.set(entry.id, validate);
    return validate;
  }

  async validateText(text: string, file: string): Promise<ValidationResult> {
    const parsed = parseJsoncDocument(text, file);
    const entry = this.registry.findForDocument(file);
    if (parsed.diagnostics.length > 0) {
      return { file, schema: entry, diagnostics: parsed.diagnostics };
    }
    if (!entry) {
      return {
        file,
        schema: null,
        diagnostics: [
          {
            ...positionAt(text, 0),
            code: "PS_SCHEMA_UNASSOCIATED",
            severity: "warning",
            message:
              "No PalSchema schema is associated with this folder; JSON/JSONC syntax was checked.",
            file,
            source: "palschema",
          },
        ],
      };
    }

    if (entry.generated) {
      try {
        await this.registry.readSchema(entry);
      } catch (error) {
        if ((error as NodeJS.ErrnoException).code === "ENOENT") {
          return {
            file,
            schema: entry,
            diagnostics: [
              {
                ...positionAt(text, 0),
                code: "PS_SCHEMA_GENERATED_MISSING",
                severity: this.allowMissingGenerated ? "warning" : "error",
                message:
                  `${entry.file} is generated by PalSchema at runtime. ` +
                  "Generate or copy the current schema pack, then set PALSCHEMA_SCHEMA_DIR.",
                file,
                source: "palschema",
              },
            ],
          };
        }
        throw error;
      }
    }

    const validate = await this.compile(entry);
    const valid = validate(parsed.data);
    const diagnostics = (validate.errors ?? []).map((error) =>
      ajvDiagnostic({ error, text, root: parsed.root, file }),
    );
    if (this.fallbackEnumEntries.has(entry.id)) {
      diagnostics.push({
        ...positionAt(text, 0),
        code: "PS_SCHEMA_ENUMS_MISSING",
        severity: this.allowMissingGenerated ? "warning" : "error",
        message:
          "enums.schema.json is missing; enum values were not constrained during this structural validation.",
        file,
        source: "palschema",
      });
    }
    if (!valid && diagnostics.length === 0) {
      diagnostics.push({
        ...positionAt(text, 0),
        code: "PS_SCHEMA_INVALID",
        severity: "error",
        message: "Schema validation failed without a detailed Ajv error.",
        file,
        source: "palschema",
      });
    }

    return { file, schema: entry, diagnostics };
  }

  async validateFile(file: string): Promise<ValidationResult> {
    return this.validateText(await readFile(file, "utf8"), file);
  }

  describeSchema(result: ValidationResult): string {
    return result.schema?.file ?? `syntax-only (${basename(result.file)})`;
  }
}
