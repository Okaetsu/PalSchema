import assert from "node:assert/strict";
import test from "node:test";

import { parseInitializationOptions } from "../src/lsp-options.js";

test("validates LSP initialization options at runtime", () => {
  assert.deepEqual(parseInitializationOptions(null), {});
  assert.deepEqual(
    parseInitializationOptions({
      schemaDirectory: "/tmp/schemas",
      allowMissingGenerated: false,
    }),
    {
      schemaDirectory: "/tmp/schemas",
      allowMissingGenerated: false,
    },
  );
  for (const invalid of [
    [],
    1,
    "options",
    { schemaDirectory: 7 },
    { allowMissingGenerated: "false" },
  ]) {
    assert.throws(() => parseInitializationOptions(invalid));
  }
});
