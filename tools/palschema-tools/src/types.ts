import type { ErrorObject } from "ajv";

export type DiagnosticSeverity = "error" | "warning";

export interface SourcePosition {
  line: number;
  column: number;
  offset: number;
  length: number;
}

export interface PalSchemaDiagnostic extends SourcePosition {
  code: string;
  severity: DiagnosticSeverity;
  message: string;
  file: string;
  source: "palschema";
  schemaPath?: string;
  instancePath?: string;
}

export interface SchemaIndexEntry {
  id: string;
  file: string;
  folder: string | null;
  patterns: string[];
  generated: boolean;
  dependencies: string[];
  sha256: string | null;
}

export interface SchemaIndex {
  formatVersion: number;
  palSchemaVersion: string;
  baseId: string;
  schemas: SchemaIndexEntry[];
}

export interface SchemaVerification {
  entry: SchemaIndexEntry;
  status:
    | "verified"
    | "present-generated"
    | "missing-generated"
    | "missing"
    | "checksum-mismatch";
  actualSha256?: string;
}

export interface ParsedDocument {
  data: unknown;
  root: import("jsonc-parser").Node | undefined;
  diagnostics: PalSchemaDiagnostic[];
}

export interface ValidationResult {
  file: string;
  schema: SchemaIndexEntry | null;
  diagnostics: PalSchemaDiagnostic[];
}

export interface AjvDiagnosticContext {
  error: ErrorObject;
  text: string;
  root: import("jsonc-parser").Node | undefined;
  file: string;
}
