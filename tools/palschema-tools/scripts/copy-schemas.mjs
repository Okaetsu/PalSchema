import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import { copyPublicSchemas } from "../../../scripts/copy-public-schemas.mjs";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const packageRoot = resolve(scriptDirectory, "..");
const repositoryRoot = resolve(packageRoot, "../..");
const source = resolve(repositoryRoot, "assets/schemas");
const destination = resolve(packageRoot, "dist/schemas");

await copyPublicSchemas(source, destination);
