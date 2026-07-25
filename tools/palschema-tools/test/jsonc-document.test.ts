import assert from "node:assert/strict";
import test from "node:test";

import { parseJsoncDocument } from "../src/jsonc-document.js";

test("accepts comments and trailing commas", () => {
  const parsed = parseJsoncDocument(
    `{
      // PalSchema supports JSONC.
      "Example": {
        "Value": 1,
      },
    }`,
    "example.jsonc",
  );

  assert.deepEqual(parsed.diagnostics, []);
  assert.deepEqual(parsed.data, { Example: { Value: 1 } });
});

test("reports one-based source positions for syntax errors", () => {
  const parsed = parseJsoncDocument('{\n  "Example":,\n}', "broken.jsonc");

  assert.equal(parsed.diagnostics.length > 0, true);
  assert.equal(parsed.diagnostics[0]?.line, 2);
  assert.equal(parsed.diagnostics[0]?.source, "palschema");
});
