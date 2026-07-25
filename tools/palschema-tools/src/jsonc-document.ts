import {
  parse,
  parseTree,
  printParseErrorCode,
  type ParseError,
} from "jsonc-parser";

import { positionAt } from "./text-position.js";
import type { PalSchemaDiagnostic, ParsedDocument } from "./types.js";

export function parseJsoncDocument(text: string, file: string): ParsedDocument {
  const errors: ParseError[] = [];
  const options = {
    allowEmptyContent: false,
    allowTrailingComma: true,
    disallowComments: false,
  };
  const data = parse(text, errors, options) as unknown;
  const root = parseTree(text, [], options);
  const diagnostics: PalSchemaDiagnostic[] = errors.map((error) => ({
    ...positionAt(text, error.offset, error.length),
    code: `PS_JSON_${printParseErrorCode(error.error).toUpperCase()}`,
    severity: "error",
    message: printParseErrorCode(error.error),
    file,
    source: "palschema",
  }));

  return { data, root, diagnostics };
}
