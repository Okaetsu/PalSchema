import { isAbsolute, resolve } from "node:path";

import {
  commands,
  RelativePattern,
  type ExtensionContext,
  type WorkspaceFolder,
  Uri,
  workspace,
} from "vscode";
import {
  LanguageClient,
  TransportKind,
  type LanguageClientOptions,
  type ServerOptions,
} from "vscode-languageclient/node";

import { SerialOperationQueue } from "./serial-operation-queue.js";

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

const clients = new Map<string, LanguageClient>();
const clientOperations = new SerialOperationQueue();

function configuredSchemaDirectory(
  context: ExtensionContext,
  folder: WorkspaceFolder,
): string {
  const configured = workspace
    .getConfiguration("palschema", folder.uri)
    .get<string>("schemaDirectory", "")
    .trim();
  if (!configured) {
    return context.asAbsolutePath("dist/schemas");
  }
  if (isAbsolute(configured)) {
    return configured;
  }
  return resolve(folder.uri.fsPath, configured);
}

function documentSelector(folder: WorkspaceFolder): NonNullable<
  LanguageClientOptions["documentSelector"]
> {
  return PALSCHEMA_FOLDERS.flatMap((loaderFolder) => [
    {
      scheme: "file",
      language: "json",
      pattern: new RelativePattern(
        folder,
        `**/${loaderFolder}/**/*.json`,
      ) as unknown as string,
    },
    {
      scheme: "file",
      language: "jsonc",
      pattern: new RelativePattern(
        folder,
        `**/${loaderFolder}/**/*.jsonc`,
      ) as unknown as string,
    },
  ]);
}

async function resourceExists(uri: Uri): Promise<boolean> {
  try {
    await workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

async function isPalSchemaWorkspace(folder: WorkspaceFolder): Promise<boolean> {
  const root = folder.uri;
  if (
    await resourceExists(Uri.joinPath(root, "palschema.config.json"))
  ) {
    return true;
  }
  if (
    (await resourceExists(Uri.joinPath(root, "CMakeLists.txt"))) &&
    (await resourceExists(
      Uri.joinPath(root, "assets", "schemas", "schema-index.json"),
    ))
  ) {
    return true;
  }
  if (
    (await resourceExists(Uri.joinPath(root, "enabled.txt"))) &&
    (await resourceExists(Uri.joinPath(root, "mods")))
  ) {
    return true;
  }
  return resourceExists(
    Uri.joinPath(root, "Mods", "PalSchema", "enabled.txt"),
  );
}

async function startClient(
  context: ExtensionContext,
  folder: WorkspaceFolder,
): Promise<void> {
  const serverModule = context.asAbsolutePath("dist/server.mjs");
  const serverOptions: ServerOptions = {
    run: { module: serverModule, transport: TransportKind.ipc },
    debug: { module: serverModule, transport: TransportKind.ipc },
  };
  const clientOptions: LanguageClientOptions = {
    documentSelector: documentSelector(folder),
    workspaceFolder: folder,
    initializationOptions: {
      schemaDirectory: configuredSchemaDirectory(context, folder),
      allowMissingGenerated: workspace
        .getConfiguration("palschema", folder.uri)
        .get<boolean>("allowMissingGenerated", true),
    },
  };
  const client = new LanguageClient(
    `palschema-${folder.index}`,
    `PalSchema Language Server (${folder.name})`,
    serverOptions,
    clientOptions,
  );
  await client.start();
  clients.set(folder.uri.toString(), client);
}

async function stopClients(): Promise<void> {
  const running = [...clients.values()];
  clients.clear();
  await Promise.all(running.map((client) => client.stop()));
}

async function restartClients(context: ExtensionContext): Promise<void> {
  await stopClients();
  try {
    for (const folder of workspace.workspaceFolders ?? []) {
      if (await isPalSchemaWorkspace(folder)) {
        await startClient(context, folder);
      }
    }
  } catch (error) {
    await stopClients();
    throw error;
  }
}

function scheduleRestart(context: ExtensionContext): Promise<void> {
  return clientOperations.run(() => restartClients(context));
}

function requestRestart(context: ExtensionContext): void {
  void scheduleRestart(context).catch((error: unknown) => {
    console.error("Unable to restart PalSchema language servers.", error);
  });
}

export async function activate(context: ExtensionContext): Promise<void> {
  context.subscriptions.push(
    commands.registerCommand("palschema.restartLanguageServer", () =>
      scheduleRestart(context),
    ),
    workspace.onDidChangeConfiguration((event) => {
      if (event.affectsConfiguration("palschema")) {
        requestRestart(context);
      }
    }),
    workspace.onDidChangeWorkspaceFolders(() => {
      requestRestart(context);
    }),
  );
  await scheduleRestart(context);
}

export async function deactivate(): Promise<void> {
  await clientOperations.run(stopClients);
}
