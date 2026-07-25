import { readFile } from "node:fs/promises";
import { parse as parsePath, resolve } from "node:path";

import { parse, type ParseError, printParseErrorCode } from "jsonc-parser";

export interface PalSchemaProjectConfig {
  schemaDirectory?: string;
  allowMissingGenerated?: boolean;
}

export interface ResolvedProjectConfig extends PalSchemaProjectConfig {
  file: string;
}

function assertProjectConfig(
  value: unknown,
  file: string,
): asserts value is PalSchemaProjectConfig {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    throw new Error(`${file} must contain a JSON object.`);
  }
  if (
    "schemaDirectory" in value &&
    typeof value.schemaDirectory !== "string"
  ) {
    throw new Error(`${file}: schemaDirectory must be a string.`);
  }
  if (
    "allowMissingGenerated" in value &&
    typeof value.allowMissingGenerated !== "boolean"
  ) {
    throw new Error(`${file}: allowMissingGenerated must be a boolean.`);
  }
}

export async function findProjectConfig(
  startDirectory = process.cwd(),
): Promise<ResolvedProjectConfig | null> {
  let directory = resolve(startDirectory);
  for (;;) {
    const file = resolve(directory, "palschema.config.json");
    try {
      const text = await readFile(file, "utf8");
      const errors: ParseError[] = [];
      const value = parse(text, errors, {
        allowTrailingComma: true,
        disallowComments: false,
      }) as unknown;
      const firstError = errors[0];
      if (firstError) {
        throw new Error(
          `${file}: ${printParseErrorCode(firstError.error)}`,
        );
      }
      assertProjectConfig(value, file);
      const result: ResolvedProjectConfig = { file };
      if (value.schemaDirectory !== undefined) {
        result.schemaDirectory = resolve(directory, value.schemaDirectory);
      }
      if (value.allowMissingGenerated !== undefined) {
        result.allowMissingGenerated = value.allowMissingGenerated;
      }
      return result;
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code !== "ENOENT") {
        throw error;
      }
    }

    const parent = parsePath(directory).dir;
    if (parent === directory) {
      return null;
    }
    directory = parent;
  }
}
