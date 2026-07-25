import { isAbsolute, resolve } from "node:path";

import {
  commands,
  type ExtensionContext,
  workspace,
} from "vscode";
import {
  LanguageClient,
  TransportKind,
  type LanguageClientOptions,
  type ServerOptions,
} from "vscode-languageclient/node";

const PALSCHEMA_FOLDERS = [
  "appearance",
  "blueprints",
  "buildings",
  "enums",
  "helpguide",
  "items",
  "npcs",
  "pals",
  "raw",
  "resources",
  "skins",
  "spawns",
  "translations",
];

let client: LanguageClient | undefined;

function configuredSchemaDirectory(context: ExtensionContext): string {
  const configured = workspace
    .getConfiguration("palschema")
    .get<string>("schemaDirectory", "")
    .trim();
  if (!configured) {
    return context.asAbsolutePath("dist/schemas");
  }
  if (isAbsolute(configured)) {
    return configured;
  }
  const workspaceRoot = workspace.workspaceFolders?.[0]?.uri.fsPath;
  return resolve(workspaceRoot ?? process.cwd(), configured);
}

function documentSelector(): NonNullable<
  LanguageClientOptions["documentSelector"]
> {
  return PALSCHEMA_FOLDERS.flatMap((folder) => [
    { scheme: "file", language: "json", pattern: `**/${folder}/**/*.json` },
    { scheme: "file", language: "jsonc", pattern: `**/${folder}/**/*.jsonc` },
  ]);
}

async function startClient(context: ExtensionContext): Promise<void> {
  const serverModule = context.asAbsolutePath("dist/server.mjs");
  const serverOptions: ServerOptions = {
    run: { module: serverModule, transport: TransportKind.ipc },
    debug: { module: serverModule, transport: TransportKind.ipc },
  };
  const clientOptions: LanguageClientOptions = {
    documentSelector: documentSelector(),
    initializationOptions: {
      schemaDirectory: configuredSchemaDirectory(context),
      allowMissingGenerated: workspace
        .getConfiguration("palschema")
        .get<boolean>("allowMissingGenerated", true),
    },
  };
  client = new LanguageClient(
    "palschema",
    "PalSchema Language Server",
    serverOptions,
    clientOptions,
  );
  await client.start();
}

async function restartClient(context: ExtensionContext): Promise<void> {
  if (client) {
    await client.stop();
    client = undefined;
  }
  await startClient(context);
}

export async function activate(context: ExtensionContext): Promise<void> {
  context.subscriptions.push(
    commands.registerCommand("palschema.restartLanguageServer", () =>
      restartClient(context),
    ),
    workspace.onDidChangeConfiguration((event) => {
      if (event.affectsConfiguration("palschema")) {
        void restartClient(context);
      }
    }),
  );
  await startClient(context);
}

export async function deactivate(): Promise<void> {
  if (client) {
    await client.stop();
    client = undefined;
  }
}
