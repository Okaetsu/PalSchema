import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { cp, mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import test from "node:test";

const testDirectory = dirname(fileURLToPath(import.meta.url));
const packageRoot = resolve(testDirectory, "..");
const repositoryRoot = resolve(packageRoot, "../..");

function encodeMessage(message) {
  const payload = JSON.stringify(message);
  return `Content-Length: ${Buffer.byteLength(payload)}\r\n\r\n${payload}`;
}

test("serves diagnostics and schema features over LSP", async (context) => {
  const temporarySchemaDirectory = await mkdtemp(
    resolve(tmpdir(), "palschema-lsp-schemas-"),
  );
  await cp(
    resolve(repositoryRoot, "assets/schemas"),
    temporarySchemaDirectory,
    { recursive: true },
  );
  await writeFile(
    resolve(temporarySchemaDirectory, "enums.schema.json"),
    JSON.stringify({
      $schema: "http://json-schema.org/draft-07/schema#",
      $id: "https://okaetsu.github.io/PalSchema/schemas/0.6.1/enums.schema.json",
      definitions: {
        EPalItemTypeA: { type: "string", enum: ["CanaryTypeA"] },
        EPalItemTypeB: { type: "string", enum: ["CanaryTypeB"] },
      },
    }),
  );
  context.after(async () => {
    await rm(temporarySchemaDirectory, { recursive: true, force: true });
  });
  const child = spawn(
    process.execPath,
    ["--import", "tsx", "src/lsp-server.ts", "--stdio"],
    {
      cwd: packageRoot,
      stdio: ["pipe", "pipe", "pipe"],
    },
  );
  context.after(() => {
    if (!child.killed) {
      child.kill();
    }
  });

  let buffer = Buffer.alloc(0);
  let stderr = "";
  const received = [];
  const waiters = [];

  function dispatch(message) {
    received.push(message);
    for (const waiter of [...waiters]) {
      if (waiter.predicate(message)) {
        clearTimeout(waiter.timeout);
        waiters.splice(waiters.indexOf(waiter), 1);
        waiter.resolve(message);
      }
    }
  }

  function consume() {
    for (;;) {
      const separator = buffer.indexOf("\r\n\r\n");
      if (separator < 0) {
        return;
      }
      const header = buffer.subarray(0, separator).toString("ascii");
      const lengthMatch = /^Content-Length: (\d+)$/im.exec(header);
      assert.ok(lengthMatch, `Missing Content-Length header in ${header}`);
      const length = Number(lengthMatch[1]);
      const bodyStart = separator + 4;
      if (buffer.length < bodyStart + length) {
        return;
      }
      const body = buffer
        .subarray(bodyStart, bodyStart + length)
        .toString("utf8");
      buffer = buffer.subarray(bodyStart + length);
      dispatch(JSON.parse(body));
    }
  }

  child.stdout.on("data", (chunk) => {
    buffer = Buffer.concat([buffer, chunk]);
    consume();
  });
  child.stderr.on("data", (chunk) => {
    stderr += chunk.toString("utf8");
  });

  function send(message) {
    child.stdin.write(encodeMessage(message));
  }

  function waitFor(predicate, label) {
    const existing = received.find(predicate);
    if (existing) {
      return Promise.resolve(existing);
    }
    return new Promise((resolvePromise, rejectPromise) => {
      const timeout = setTimeout(() => {
        rejectPromise(
          new Error(`Timed out waiting for ${label}. Server stderr:\n${stderr}`),
        );
      }, 10_000);
      waiters.push({ predicate, resolve: resolvePromise, timeout });
    });
  }

  send({
    jsonrpc: "2.0",
    id: 1,
    method: "initialize",
    params: {
      processId: process.pid,
      rootUri: pathToFileURL(repositoryRoot).toString(),
      capabilities: {},
      initializationOptions: {
        schemaDirectory: temporarySchemaDirectory,
        allowMissingGenerated: true,
      },
    },
  });
  const initialize = await waitFor(
    (message) => message.id === 1,
    "initialize response",
  );
  assert.equal(initialize.result.serverInfo.name, "PalSchema Language Server");
  assert.equal(initialize.result.capabilities.hoverProvider, true);
  send({ jsonrpc: "2.0", method: "initialized", params: {} });

  const documentUri = pathToFileURL(
    resolve(repositoryRoot, "fixture/items/invalid.jsonc"),
  ).toString();
  send({
    jsonrpc: "2.0",
    method: "textDocument/didOpen",
    params: {
      textDocument: {
        uri: documentUri,
        languageId: "jsonc",
        version: 1,
        text: '{\n  "Broken": { "Rarity": 99 }\n}\n',
      },
    },
  });
  const diagnostics = await waitFor(
    (message) =>
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === documentUri,
    "published diagnostics",
  );
  assert.equal(
    diagnostics.params.diagnostics.some(
      (diagnostic) => diagnostic.code === "PS_SCHEMA_MAXIMUM",
    ),
    true,
  );

  const completionUri = pathToFileURL(
    resolve(repositoryRoot, "fixture/items/completion.jsonc"),
  ).toString();
  send({
    jsonrpc: "2.0",
    method: "textDocument/didOpen",
    params: {
      textDocument: {
        uri: completionUri,
        languageId: "jsonc",
        version: 1,
        text: '{\n  "Example": {\n    \n  }\n}\n',
      },
    },
  });
  await waitFor(
    (message) =>
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === completionUri,
    "completion document diagnostics",
  );
  send({
    jsonrpc: "2.0",
    id: 2,
    method: "textDocument/completion",
    params: {
      textDocument: { uri: completionUri },
      position: { line: 2, character: 4 },
    },
  });
  const completion = await waitFor(
    (message) => message.id === 2,
    "completion response",
  );
  const completionItems = Array.isArray(completion.result)
    ? completion.result
    : completion.result?.items ?? [];
  assert.equal(
    completionItems.some((item) => item.label === "Type"),
    true,
  );

  const enumUri = pathToFileURL(
    resolve(repositoryRoot, "fixture/items/enum-completion.jsonc"),
  ).toString();
  send({
    jsonrpc: "2.0",
    method: "textDocument/didOpen",
    params: {
      textDocument: {
        uri: enumUri,
        languageId: "jsonc",
        version: 1,
        text: '{\n  "Example": {\n    "TypeA": ""\n  }\n}\n',
      },
    },
  });
  await waitFor(
    (message) =>
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === enumUri,
    "enum completion document diagnostics",
  );
  send({
    jsonrpc: "2.0",
    id: 20,
    method: "textDocument/completion",
    params: {
      textDocument: { uri: enumUri },
      position: { line: 2, character: 14 },
    },
  });
  const enumCompletion = await waitFor(
    (message) => message.id === 20,
    "offline enum completion response",
  );
  const enumItems = Array.isArray(enumCompletion.result)
    ? enumCompletion.result
    : enumCompletion.result?.items ?? [];
  assert.equal(
    enumItems.some(
      (item) =>
        item.label === '"CanaryTypeA"' &&
        item.textEdit?.newText === '"CanaryTypeA"',
    ),
    true,
  );

  const hoverText = '{\n  "Example": {\n    "Rarity": 1\n  }\n}\n';
  send({
    jsonrpc: "2.0",
    method: "textDocument/didChange",
    params: {
      textDocument: { uri: completionUri, version: 2 },
      contentChanges: [{ text: hoverText }],
    },
  });
  await waitFor(
    (message) =>
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === completionUri,
    "hover document diagnostics",
  );
  send({
    jsonrpc: "2.0",
    id: 3,
    method: "textDocument/hover",
    params: {
      textDocument: { uri: completionUri },
      position: { line: 2, character: 7 },
    },
  });
  const hover = await waitFor(
    (message) => message.id === 3,
    "hover response",
  );
  assert.match(JSON.stringify(hover.result), /background color/i);

  const closedUri = pathToFileURL(
    resolve(repositoryRoot, "fixture/items/closed.jsonc"),
  ).toString();
  send({
    jsonrpc: "2.0",
    method: "textDocument/didOpen",
    params: {
      textDocument: {
        uri: closedUri,
        languageId: "jsonc",
        version: 1,
        text: '{\n  "Broken": { "Rarity": 99 }\n}\n',
      },
    },
  });
  send({
    jsonrpc: "2.0",
    method: "textDocument/didClose",
    params: { textDocument: { uri: closedUri } },
  });
  const cleared = await waitFor(
    (message) =>
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === closedUri &&
      message.params?.diagnostics?.length === 0,
    "closed document diagnostics clear",
  );
  const clearIndex = received.indexOf(cleared);
  await new Promise((resolvePromise) => setTimeout(resolvePromise, 200));
  assert.equal(
    received
      .slice(clearIndex + 1)
      .some(
        (message) =>
          message.method === "textDocument/publishDiagnostics" &&
          message.params?.uri === closedUri &&
          message.params?.diagnostics?.length > 0,
      ),
    false,
  );

  const largeUri = pathToFileURL(
    resolve(repositoryRoot, "fixture/items/large-invalid.json"),
  ).toString();
  const largeDocument = Object.fromEntries(
    Array.from({ length: 700 }, (_, index) => [
      `Broken${index}`,
      { Rarity: 99 },
    ]),
  );
  send({
    jsonrpc: "2.0",
    method: "textDocument/didOpen",
    params: {
      textDocument: {
        uri: largeUri,
        languageId: "json",
        version: 1,
        text: JSON.stringify(largeDocument),
      },
    },
  });
  const boundedDiagnostics = await waitFor(
    (message) =>
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === largeUri,
    "bounded diagnostics",
  );
  assert.equal(boundedDiagnostics.params.diagnostics.length, 500);

  const rawUri = pathToFileURL(
    resolve(repositoryRoot, "fixture/raw/stale.json"),
  ).toString();
  send({
    jsonrpc: "2.0",
    method: "textDocument/didOpen",
    params: {
      textDocument: {
        uri: rawUri,
        languageId: "json",
        version: 1,
        text: "{}\n",
      },
    },
  });
  const missingRaw = await waitFor(
    (message) =>
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === rawUri &&
      message.params?.diagnostics?.some(
        (diagnostic) => diagnostic.code === "PS_SCHEMA_GENERATED_MISSING",
      ),
    "missing raw schema diagnostic",
  );
  const malformedStart = received.indexOf(missingRaw) + 1;
  await writeFile(
    resolve(temporarySchemaDirectory, "raw.schema.json"),
    "{malformed\n",
  );
  send({
    jsonrpc: "2.0",
    method: "textDocument/didChange",
    params: {
      textDocument: { uri: rawUri, version: 2 },
      contentChanges: [{ text: '{"changed":true}\n' }],
    },
  });
  await waitFor(
    (message) =>
      received.indexOf(message) >= malformedStart &&
      message.method === "textDocument/publishDiagnostics" &&
      message.params?.uri === rawUri &&
      message.params?.diagnostics?.length === 0,
    "stale diagnostics clear after validator failure",
  );

  send({ jsonrpc: "2.0", id: 4, method: "shutdown", params: null });
  await waitFor((message) => message.id === 4, "shutdown response");
  send({ jsonrpc: "2.0", method: "exit", params: null });
});
