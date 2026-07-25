import { createHash } from "node:crypto";
import { readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const schemaDirectory = resolve(scriptDirectory, "../../../assets/schemas");
const indexPath = resolve(schemaDirectory, "schema-index.json");
const index = JSON.parse(await readFile(indexPath, "utf8"));

for (const entry of index.schemas) {
  try {
    const content = await readFile(resolve(schemaDirectory, entry.file));
    entry.sha256 = createHash("sha256").update(content).digest("hex");
  } catch (error) {
    if (error.code !== "ENOENT" || !entry.generated) {
      throw error;
    }
    entry.sha256 = null;
  }
}

await writeFile(indexPath, `${JSON.stringify(index, null, 2)}\n`, "utf8");
