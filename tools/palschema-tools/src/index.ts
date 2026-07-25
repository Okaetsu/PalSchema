export { findProjectConfig } from "./configuration.js";
export { parseJsoncDocument } from "./jsonc-document.js";
export { SchemaRegistry } from "./schema-registry.js";
export { PalSchemaValidator } from "./validator.js";
export type {
  DiagnosticSeverity,
  PalSchemaDiagnostic,
  SchemaIndex,
  SchemaIndexEntry,
  SchemaVerification,
  ValidationResult,
} from "./types.js";
export type {
  PalSchemaProjectConfig,
  ResolvedProjectConfig,
} from "./configuration.js";
