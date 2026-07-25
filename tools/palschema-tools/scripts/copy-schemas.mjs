import { cp, mkdir } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const packageRoot = resolve(scriptDirectory, "..");
const repositoryRoot = resolve(packageRoot, "../..");
const source = resolve(repositoryRoot, "assets/schemas");
const destination = resolve(packageRoot, "dist/schemas");

await mkdir(destination, { recursive: true });
await cp(source, destination, { recursive: true, force: true });
