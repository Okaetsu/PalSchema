import assert from "node:assert/strict";
import test from "node:test";

import { parseJsoncDocument } from "../src/jsonc-document.js";

test("accepts comments in JSONC but rejects trailing commas", () => {
  const parsed = parseJsoncDocument(
    `{
      // PalSchema supports JSONC.
      "Example": {
        "Value": 1
      }
    }`,
    "example.jsonc",
  );

  assert.deepEqual(parsed.diagnostics, []);
  assert.deepEqual(parsed.data, { Example: { Value: 1 } });

  const trailing = parseJsoncDocument('{"Value": 1,}', "example.jsonc");
  assert.equal(trailing.diagnostics.length > 0, true);
});

test("rejects comments in strict JSON files", () => {
  const parsed = parseJsoncDocument(
    '{\n  // JSON comments are runtime-incompatible.\n  "Value": 1\n}',
    "example.json",
  );
  assert.equal(parsed.diagnostics.length > 0, true);
});

test("reports one-based source positions for syntax errors", () => {
  const parsed = parseJsoncDocument('{\n  "Example":,\n}', "broken.jsonc");

  assert.equal(parsed.diagnostics.length > 0, true);
  assert.equal(parsed.diagnostics[0]?.line, 2);
  assert.equal(parsed.diagnostics[0]?.source, "palschema");
});
