export interface PalSchemaInitializationOptions {
  schemaDirectory?: string;
  allowMissingGenerated?: boolean;
}

export function parseInitializationOptions(
  value: unknown,
): PalSchemaInitializationOptions {
  if (value === undefined || value === null) {
    return {};
  }
  if (typeof value !== "object" || Array.isArray(value)) {
    throw new Error("PalSchema initializationOptions must be an object.");
  }
  const candidate = value as Record<string, unknown>;
  if (
    candidate.schemaDirectory !== undefined &&
    typeof candidate.schemaDirectory !== "string"
  ) {
    throw new Error("initializationOptions.schemaDirectory must be a string.");
  }
  if (
    candidate.allowMissingGenerated !== undefined &&
    typeof candidate.allowMissingGenerated !== "boolean"
  ) {
    throw new Error(
      "initializationOptions.allowMissingGenerated must be a boolean.",
    );
  }
  return {
    ...(candidate.schemaDirectory !== undefined
      ? { schemaDirectory: candidate.schemaDirectory }
      : {}),
    ...(candidate.allowMissingGenerated !== undefined
      ? { allowMissingGenerated: candidate.allowMissingGenerated }
      : {}),
  };
}
