import assert from "node:assert/strict";
import test from "node:test";

import { SerialOperationQueue } from "../src/serial-operation-queue.js";

test("serializes overlapping client lifecycle operations", async () => {
  const queue = new SerialOperationQueue();
  const events: string[] = [];
  let releaseFirst: (() => void) | undefined;
  const firstGate = new Promise<void>((resolve) => {
    releaseFirst = resolve;
  });

  const first = queue.run(async () => {
    events.push("first-start");
    await firstGate;
    events.push("first-end");
  });
  const second = queue.run(async () => {
    events.push("second-start");
    events.push("second-end");
  });

  await new Promise<void>((resolve) => setImmediate(resolve));
  assert.deepEqual(events, ["first-start"]);
  releaseFirst?.();
  await Promise.all([first, second]);
  assert.deepEqual(events, [
    "first-start",
    "first-end",
    "second-start",
    "second-end",
  ]);
});

test("continues after a failed lifecycle operation", async () => {
  const queue = new SerialOperationQueue();
  await assert.rejects(
    queue.run(async () => {
      throw new Error("start failed");
    }),
    /start failed/,
  );
  assert.equal(await queue.run(async () => "recovered"), "recovered");
});
