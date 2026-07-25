import assert from "node:assert/strict";
import test from "node:test";

import { startClientsIndependently } from "../src/start-clients-independently.js";

test("keeps starting healthy workspace clients after one fails", async () => {
  const started: string[] = [];
  const failures = await startClientsIndependently(
    ["healthy-a", "broken", "healthy-b"],
    async (candidate) => {
      if (candidate === "broken") {
        return new Error("invalid schema directory");
      }
      started.push(candidate);
      return null;
    },
  );

  assert.deepEqual(started, ["healthy-a", "healthy-b"]);
  assert.equal(failures.length, 1);
  assert.equal(failures[0]?.candidate, "broken");
  assert.match(failures[0]?.error.message ?? "", /invalid schema directory/);
});
