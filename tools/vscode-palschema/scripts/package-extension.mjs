import { mkdir } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { spawn } from "node:child_process";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const extensionRoot = resolve(scriptDirectory, "..");
const repositoryRoot = resolve(extensionRoot, "../..");
const outputDirectory = resolve(repositoryRoot, "dist");
const packageDefinition = await import(
  new URL("../package.json", import.meta.url),
  { with: { type: "json" } }
);
const output = resolve(
  outputDirectory,
  `palschema-vscode-${packageDefinition.default.version}.vsix`,
);

await mkdir(outputDirectory, { recursive: true });

const executable =
  process.platform === "win32"
    ? resolve(repositoryRoot, "node_modules/.bin/vsce.cmd")
    : resolve(repositoryRoot, "node_modules/.bin/vsce");
const child = spawn(
  executable,
  ["package", "--no-dependencies", "--out", output],
  {
    cwd: extensionRoot,
    stdio: "inherit",
  },
);
const exitCode = await new Promise((resolvePromise, rejectPromise) => {
  child.once("error", rejectPromise);
  child.once("exit", resolvePromise);
});
if (exitCode !== 0) {
  throw new Error(`vsce exited with code ${String(exitCode)}.`);
}

console.log(output);
