import { mkdir, rm } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import { build } from "esbuild";
import { copyPublicSchemas } from "../../../scripts/copy-public-schemas.mjs";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const extensionRoot = resolve(scriptDirectory, "..");
const repositoryRoot = resolve(extensionRoot, "../..");
const outputDirectory = resolve(extensionRoot, "dist");

await rm(outputDirectory, { recursive: true, force: true });
await mkdir(outputDirectory, { recursive: true });
await Promise.all([
  build({
    entryPoints: [resolve(extensionRoot, "src/extension.ts")],
    outfile: resolve(outputDirectory, "extension.js"),
    bundle: true,
    platform: "node",
    format: "cjs",
    external: ["vscode"],
    target: "node20",
  }),
  build({
    entryPoints: [
      resolve(repositoryRoot, "tools/palschema-tools/src/lsp-server.ts"),
    ],
    outfile: resolve(outputDirectory, "server.mjs"),
    bundle: true,
    platform: "node",
    format: "esm",
    target: "node20",
  }),
]);

await copyPublicSchemas(
  resolve(repositoryRoot, "assets/schemas"),
  resolve(outputDirectory, "schemas"),
);
