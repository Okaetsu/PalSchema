import {
  parse,
  parseTree,
  printParseErrorCode,
  type ParseError,
} from "jsonc-parser";
import { extname } from "node:path";

import { createPositionMapper } from "./text-position.js";
import type { PalSchemaDiagnostic, ParsedDocument } from "./types.js";

export function parseJsoncDocument(text: string, file: string): ParsedDocument {
  const errors: ParseError[] = [];
  const isJsonc = extname(file).toLowerCase() === ".jsonc";
  const options = {
    allowEmptyContent: false,
    allowTrailingComma: false,
    disallowComments: !isJsonc,
  };
  const data = parse(text, errors, options) as unknown;
  const root = parseTree(text, [], options);
  const positionAt = createPositionMapper(text);
  const diagnostics: PalSchemaDiagnostic[] = errors.map((error) => ({
    ...positionAt(error.offset, error.length),
    code: `PS_JSON_${printParseErrorCode(error.error).toUpperCase()}`,
    severity: "error",
    message: printParseErrorCode(error.error),
    file,
    source: "palschema",
  }));

  return { data, root, diagnostics };
}
