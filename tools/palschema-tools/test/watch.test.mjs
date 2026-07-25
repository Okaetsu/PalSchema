import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { mkdir, mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { resolve } from "node:path";
import test from "node:test";

const packageRoot = resolve(import.meta.dirname, "..");
const cliSource = resolve(packageRoot, "src/cli.ts");
const tsxImport = import.meta.resolve("tsx");

test("JSON watch mode emits NDJSON and coalesces to the newest snapshot", async (context) => {
  const workspace = await mkdtemp(resolve(tmpdir(), "palschema-watch-"));
  const items = resolve(workspace, "items");
  const document = resolve(items, "item.json");
  await mkdir(items);
  await writeFile(
    resolve(workspace, "palschema.config.json"),
    '{"allowMissingGenerated":true}\n',
  );
  await writeFile(document, "{}\n");

  const child = spawn(
    process.execPath,
    [
      "--import",
      tsxImport,
      cliSource,
      "validate",
      "--watch",
      "--format",
      "json",
      workspace,
    ],
    { cwd: packageRoot, stdio: ["ignore", "pipe", "pipe"] },
  );
  context.after(async () => {
    if (!child.killed) {
      child.kill("SIGTERM");
    }
    await rm(workspace, { recursive: true, force: true });
  });

  const events = [];
  const waiters = [];
  let buffer = "";
  let stderr = "";

  const dispatch = (line) => {
    if (!line) {
      return;
    }
    const event = JSON.parse(line);
    events.push(event);
    for (const waiter of [...waiters]) {
      if (waiter.predicate(event)) {
        clearTimeout(waiter.timeout);
        waiters.splice(waiters.indexOf(waiter), 1);
        waiter.resolve(event);
      }
    }
  };
  child.stdout.on("data", (chunk) => {
    buffer += chunk.toString("utf8");
    for (;;) {
      const newline = buffer.indexOf("\n");
      if (newline < 0) {
        break;
      }
      dispatch(buffer.slice(0, newline));
      buffer = buffer.slice(newline + 1);
    }
  });
  child.stderr.on("data", (chunk) => {
    stderr += chunk.toString("utf8");
  });

  const waitFor = (predicate, label) => {
    const existing = events.find(predicate);
    if (existing) {
      return Promise.resolve(existing);
    }
    return new Promise((resolvePromise, rejectPromise) => {
      const timeout = setTimeout(
        () => rejectPromise(new Error(`Timed out waiting for ${label}: ${stderr}`)),
        10_000,
      );
      waiters.push({ predicate, resolve: resolvePromise, timeout });
    });
  };

  await waitFor((event) => event.event === "watch-started", "watch start");
  const validationCount = events.filter(
    (event) => event.event === "validation",
  ).length;
  await writeFile(document, '{"Broken":{"Rarity":99}}\n');
  await new Promise((resolvePromise) => setTimeout(resolvePromise, 10));
  await writeFile(document, "{}\n");
  const newest = await waitFor(
    (_event) =>
      events.filter((event) => event.event === "validation").length >
      validationCount,
    "coalesced validation",
  );
  assert.equal(newest.errors, 0);
  assert.equal(stderr, "");

  child.kill("SIGINT");
  await new Promise((resolvePromise) => child.once("exit", resolvePromise));
});
