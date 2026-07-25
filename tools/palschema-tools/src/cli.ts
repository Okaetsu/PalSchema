#!/usr/bin/env node

import { existsSync } from "node:fs";
import {
  access,
  constants,
  copyFile,
  mkdir,
  readdir,
  stat,
  writeFile,
} from "node:fs/promises";
import { dirname, resolve, extname, join, relative } from "node:path";

import chokidar from "chokidar";

import { findProjectConfig } from "./configuration.js";
import { SchemaRegistry } from "./schema-registry.js";
import type { PalSchemaDiagnostic, ValidationResult } from "./types.js";
import { PalSchemaValidator } from "./validator.js";

const IGNORED_DIRECTORIES = new Set([
  ".git",
  ".palschema",
  ".vscode",
  "build",
  "dist",
  "node_modules",
]);
const IGNORED_FILES = new Set(["palschema.config.json"]);

interface CommonOptions {
  schemaDirectory?: string;
  format: "human" | "json";
}

interface ValidateOptions extends CommonOptions {
  paths: string[];
  watch: boolean;
  allowMissingGenerated: boolean | undefined;
}

interface InitOptions extends CommonOptions {
  destination: string;
  force: boolean;
}

function showUsage(): void {
  console.log(`PalSchema authoring tools

Usage:
  palschema validate [--watch] [--allow-missing-generated|--strict-generated]
                     [--format human|json]
                     [--schema-dir PATH] [PATH...]
  palschema schemas list [--format human|json] [--schema-dir PATH]
  palschema schemas verify [--format human|json] [--schema-dir PATH]
  palschema doctor [--format human|json] [--schema-dir PATH]
  palschema print-config [--schema-dir PATH]
  palschema init [--force] [--schema-dir PATH] [PATH]

Exit codes:
  0  validation passed (warnings are allowed)
  1  one or more validation errors
  2  invalid arguments, configuration, or internal failure`);
}

async function pathExists(path: string): Promise<boolean> {
  try {
    await access(path, constants.F_OK);
    return true;
  } catch (error) {
    if ((error as NodeJS.ErrnoException).code === "ENOENT") {
      return false;
    }
    throw error;
  }
}

function parseCommonOptions(arguments_: string[]): {
  options: CommonOptions;
  remaining: string[];
} {
  const options: CommonOptions = { format: "human" };
  const remaining: string[] = [];

  for (let index = 0; index < arguments_.length; index += 1) {
    const argument = arguments_[index];
    if (argument === "--schema-dir") {
      const value = arguments_[index + 1];
      if (!value) {
        throw new Error("--schema-dir requires a path.");
      }
      options.schemaDirectory = value;
      index += 1;
    } else if (argument === "--format") {
      const value = arguments_[index + 1];
      if (value !== "human" && value !== "json") {
        throw new Error("--format must be human or json.");
      }
      options.format = value;
      index += 1;
    } else {
      remaining.push(argument ?? "");
    }
  }

  return { options, remaining };
}

function parseValidateOptions(arguments_: string[]): ValidateOptions {
  const watch = arguments_.includes("--watch");
  const allowsMissing = arguments_.includes("--allow-missing-generated");
  const requiresGenerated = arguments_.includes("--strict-generated");
  if (allowsMissing && requiresGenerated) {
    throw new Error(
      "--allow-missing-generated and --strict-generated are mutually exclusive.",
    );
  }
  const allowMissingGenerated = allowsMissing
    ? true
    : requiresGenerated
      ? false
      : undefined;
  const filtered = arguments_.filter(
    (argument) =>
      argument !== "--watch" &&
      argument !== "--allow-missing-generated" &&
      argument !== "--strict-generated",
  );
  const { options, remaining } = parseCommonOptions(filtered);
  return {
    ...options,
    paths: remaining.length > 0 ? remaining : ["."],
    watch,
    allowMissingGenerated,
  };
}

function parseInitOptions(arguments_: string[]): InitOptions {
  const force = arguments_.includes("--force");
  const filtered = arguments_.filter((argument) => argument !== "--force");
  const { options, remaining } = parseCommonOptions(filtered);
  if (remaining.length > 1) {
    throw new Error("init accepts at most one destination path.");
  }
  return {
    ...options,
    destination: resolve(remaining[0] ?? "."),
    force,
  };
}

function isJsonDocument(path: string): boolean {
  const extension = extname(path).toLowerCase();
  return extension === ".json" || extension === ".jsonc";
}

async function discoverFiles(paths: string[]): Promise<string[]> {
  const discovered = new Set<string>();

  async function visit(path: string): Promise<void> {
    const absolutePath = resolve(path);
    const metadata = await stat(absolutePath);
    if (metadata.isFile()) {
      if (
        !IGNORED_FILES.has(absolutePath.split(/[\\/]/).at(-1) ?? "") &&
        isJsonDocument(absolutePath)
      ) {
        discovered.add(absolutePath);
      }
      return;
    }
    if (!metadata.isDirectory()) {
      return;
    }

    const entries = await readdir(absolutePath, { withFileTypes: true });
    await Promise.all(
      entries.map(async (entry) => {
        if (entry.isDirectory() && IGNORED_DIRECTORIES.has(entry.name)) {
          return;
        }
        await visit(join(absolutePath, entry.name));
      }),
    );
  }

  await Promise.all(paths.map(visit));
  return [...discovered].sort();
}

function displayFile(file: string): string {
  const relativePath = relative(process.cwd(), file);
  return relativePath && !relativePath.startsWith("..") ? relativePath : file;
}

function printDiagnostic(diagnostic: PalSchemaDiagnostic): void {
  console.log(
    `${displayFile(diagnostic.file)}:${diagnostic.line}:${diagnostic.column} ` +
      `[${diagnostic.code}] ${diagnostic.severity}: ${diagnostic.message}`,
  );
}

function printValidationResults(
  results: ValidationResult[],
  format: CommonOptions["format"],
): number {
  const diagnostics = results.flatMap((result) => result.diagnostics);
  if (format === "json") {
    console.log(
      JSON.stringify(
        {
          files: results.length,
          errors: diagnostics.filter((item) => item.severity === "error").length,
          warnings: diagnostics.filter((item) => item.severity === "warning")
            .length,
          diagnostics,
        },
        null,
        2,
      ),
    );
  } else {
    diagnostics.forEach(printDiagnostic);
    const errors = diagnostics.filter((item) => item.severity === "error").length;
    const warnings = diagnostics.length - errors;
    console.log(
      `Validated ${results.length} file(s): ${errors} error(s), ${warnings} warning(s).`,
    );
  }
  return diagnostics.some((item) => item.severity === "error") ? 1 : 0;
}

async function validateOnce(
  validator: PalSchemaValidator,
  paths: string[],
  format: CommonOptions["format"],
): Promise<number> {
  const files = await discoverFiles(paths);
  const results = await Promise.all(files.map((file) => validator.validateFile(file)));
  return printValidationResults(results, format);
}

async function validationProjectConfig(paths: string[]) {
  const currentProjectConfig = await findProjectConfig();
  if (currentProjectConfig || paths.length !== 1) {
    return currentProjectConfig;
  }

  const target = resolve(paths[0] ?? ".");
  const metadata = await stat(target);
  return findProjectConfig(metadata.isDirectory() ? target : dirname(target));
}

async function validateCommand(arguments_: string[]): Promise<number> {
  const options = parseValidateOptions(arguments_);
  const projectConfig = await validationProjectConfig(options.paths);
  const schemaDirectory =
    options.schemaDirectory ??
    process.env.PALSCHEMA_SCHEMA_DIR ??
    projectConfig?.schemaDirectory;
  const validator = await PalSchemaValidator.create(schemaDirectory, {
    allowMissingGenerated:
      options.allowMissingGenerated ??
      projectConfig?.allowMissingGenerated ??
      false,
  });
  let exitCode = await validateOnce(validator, options.paths, options.format);
  if (!options.watch) {
    return exitCode;
  }

  const watcher = chokidar.watch(options.paths, {
    ignored: (path, metadata) =>
      metadata?.isDirectory() === true &&
      IGNORED_DIRECTORIES.has(path.split(/[\\/]/).at(-1) ?? ""),
    ignoreInitial: true,
  });
  let pending: NodeJS.Timeout | undefined;
  const rerun = (): void => {
    if (pending) {
      clearTimeout(pending);
    }
    pending = setTimeout(async () => {
      try {
        exitCode = await validateOnce(
          validator,
          options.paths,
          options.format,
        );
        process.exitCode = exitCode;
      } catch (error) {
        console.error(error);
        process.exitCode = 2;
      }
    }, 100);
  };
  watcher.on("add", rerun).on("change", rerun).on("unlink", rerun);
  console.log("Watching for PalSchema JSON/JSONC changes. Press Ctrl+C to stop.");
  await new Promise<void>((resolvePromise) => {
    process.once("SIGINT", () => resolvePromise());
    process.once("SIGTERM", () => resolvePromise());
  });
  await watcher.close();
  return exitCode;
}

async function schemasCommand(arguments_: string[]): Promise<number> {
  const action = arguments_[0];
  const { options, remaining } = parseCommonOptions(arguments_.slice(1));
  if (remaining.length > 0) {
    throw new Error(`Unknown schemas option: ${remaining[0]}`);
  }
  const projectConfig = await findProjectConfig();
  const registry = await SchemaRegistry.load(
    options.schemaDirectory ??
      process.env.PALSCHEMA_SCHEMA_DIR ??
      projectConfig?.schemaDirectory,
  );

  if (action === "list") {
    if (options.format === "json") {
      console.log(JSON.stringify(registry.index, null, 2));
    } else {
      for (const entry of registry.index.schemas) {
        console.log(
          `${entry.file}\t${entry.generated ? "generated" : "static"}\t` +
            `${entry.folder ?? "dependency-only"}`,
        );
      }
    }
    return 0;
  }

  if (action === "verify") {
    const results = await registry.verify();
    if (options.format === "json") {
      console.log(JSON.stringify(results, null, 2));
    } else {
      for (const result of results) {
        console.log(`${result.status}\t${result.entry.file}`);
      }
    }
    return results.some(
      (result) =>
        result.status === "missing" || result.status === "checksum-mismatch",
    )
      ? 1
      : 0;
  }

  throw new Error("schemas requires either list or verify.");
}

async function doctorCommand(arguments_: string[]): Promise<number> {
  const { options, remaining } = parseCommonOptions(arguments_);
  if (remaining.length > 0) {
    throw new Error(`Unknown doctor option: ${remaining[0]}`);
  }
  const projectConfig = await findProjectConfig();
  const registry = await SchemaRegistry.load(
    options.schemaDirectory ??
      process.env.PALSCHEMA_SCHEMA_DIR ??
      projectConfig?.schemaDirectory,
  );
  const verification = await registry.verify();
  const report = {
    node: process.version,
    platform: process.platform,
    architecture: process.arch,
    schemaDirectory: registry.schemaDirectory,
    palSchemaVersion: registry.index.palSchemaVersion,
    schemas: verification.map((result) => ({
      file: result.entry.file,
      generated: result.entry.generated,
      status: result.status,
    })),
  };
  if (options.format === "json") {
    console.log(JSON.stringify(report, null, 2));
  } else {
    console.log(`Node: ${report.node} (${report.platform}/${report.architecture})`);
    console.log(`Schema directory: ${report.schemaDirectory}`);
    report.schemas.forEach((schema) =>
      console.log(`${schema.status}\t${schema.file}`),
    );
  }
  return verification.some(
    (result) =>
      result.status === "missing" || result.status === "checksum-mismatch",
  )
    ? 1
    : 0;
}

async function printConfigCommand(arguments_: string[]): Promise<number> {
  const { options, remaining } = parseCommonOptions(arguments_);
  if (remaining.length > 0) {
    throw new Error(`Unknown print-config option: ${remaining[0]}`);
  }
  const projectConfig = await findProjectConfig();
  const registry = await SchemaRegistry.load(
    options.schemaDirectory ??
      process.env.PALSCHEMA_SCHEMA_DIR ??
      projectConfig?.schemaDirectory,
  );
  console.log(
    JSON.stringify(
      {
        schemaDirectory: registry.schemaDirectory,
        palSchemaVersion: registry.index.palSchemaVersion,
        formatVersion: registry.index.formatVersion,
        projectConfig: projectConfig?.file ?? null,
      },
      null,
      2,
    ),
  );
  return 0;
}

function editorSchemaMappings(registry: SchemaRegistry): Array<{
  fileMatch: string[];
  url: string;
}> {
  return registry.index.schemas
    .filter(
      (entry) =>
        entry.folder !== null && existsSync(registry.pathFor(entry)),
    )
    .map((entry) => ({
      fileMatch: entry.patterns,
      url: `./.palschema/schemas/${entry.file}`,
    }));
}

async function initCommand(arguments_: string[]): Promise<number> {
  const options = parseInitOptions(arguments_);
  const registry = await SchemaRegistry.load(options.schemaDirectory);
  const schemaDestination = join(
    options.destination,
    ".palschema",
    "schemas",
  );
  const settingsPath = join(options.destination, ".vscode", "settings.json");
  const configPath = join(options.destination, "palschema.config.json");
  const managedTargets = [settingsPath, configPath];

  if (!options.force) {
    const conflicts = [];
    for (const target of managedTargets) {
      if (await pathExists(target)) {
        conflicts.push(target);
      }
    }
    if (conflicts.length > 0) {
      throw new Error(
        `Refusing to overwrite existing workspace files:\n${conflicts.join("\n")}\n` +
          "Re-run with --force after reviewing them.",
      );
    }
  }

  await mkdir(schemaDestination, { recursive: true });
  await mkdir(join(options.destination, ".vscode"), { recursive: true });

  await copyFile(
    join(registry.schemaDirectory, "schema-index.json"),
    join(schemaDestination, "schema-index.json"),
  );
  const copiedSchemas: string[] = ["schema-index.json"];
  const missingGenerated: string[] = [];
  for (const entry of registry.index.schemas) {
    const source = registry.pathFor(entry);
    const destination = join(schemaDestination, entry.file);
    if (!(await pathExists(source))) {
      if (entry.generated) {
        missingGenerated.push(entry.file);
        continue;
      }
      throw new Error(`Required schema is missing: ${source}`);
    }
    await copyFile(source, destination);
    copiedSchemas.push(entry.file);
  }

  const settings = {
    "json.schemas": editorSchemaMappings(registry),
    "palschema.schemaDirectory": ".palschema/schemas",
    "palschema.allowMissingGenerated": true,
  };
  const config = {
    schemaDirectory: ".palschema/schemas",
    allowMissingGenerated: true,
  };
  await writeFile(settingsPath, `${JSON.stringify(settings, null, 2)}\n`, "utf8");
  await writeFile(configPath, `${JSON.stringify(config, null, 2)}\n`, "utf8");

  const report = {
    destination: options.destination,
    schemaDirectory: schemaDestination,
    copiedSchemas,
    missingGenerated,
    settings: settingsPath,
    config: configPath,
  };
  if (options.format === "json") {
    console.log(JSON.stringify(report, null, 2));
  } else {
    console.log(`Initialized PalSchema authoring in ${options.destination}`);
    console.log(`Copied ${copiedSchemas.length} schema file(s).`);
    if (missingGenerated.length > 0) {
      console.log(
        `Runtime-generated schemas not present yet: ${missingGenerated.join(", ")}`,
      );
    }
  }
  return 0;
}

async function main(): Promise<number> {
  const [command, ...arguments_] = process.argv.slice(2);
  if (!command || command === "-h" || command === "--help" || command === "help") {
    showUsage();
    return 0;
  }
  if (command === "validate") {
    return validateCommand(arguments_);
  }
  if (command === "schemas") {
    return schemasCommand(arguments_);
  }
  if (command === "doctor") {
    return doctorCommand(arguments_);
  }
  if (command === "print-config") {
    return printConfigCommand(arguments_);
  }
  if (command === "init") {
    return initCommand(arguments_);
  }
  throw new Error(`Unknown command: ${command}`);
}

try {
  process.exitCode = await main();
} catch (error) {
  console.error(error instanceof Error ? error.message : error);
  process.exitCode = 2;
}
