#!/usr/bin/env node

import { existsSync } from "node:fs";
import { readFile } from "node:fs/promises";
import { fileURLToPath, pathToFileURL } from "node:url";

import {
  getLanguageService,
  type LanguageService,
} from "vscode-json-languageservice";
import {
  createConnection,
  DiagnosticSeverity,
  ProposedFeatures,
  TextDocumentSyncKind,
  type InitializeParams,
  type InitializeResult,
} from "vscode-languageserver/node";
import { TextDocument } from "vscode-languageserver-textdocument";
import { TextDocuments } from "vscode-languageserver/node";

import { SchemaRegistry } from "./schema-registry.js";
import type { PalSchemaDiagnostic } from "./types.js";
import { PalSchemaValidator } from "./validator.js";

interface PalSchemaInitializationOptions {
  schemaDirectory?: string;
  allowMissingGenerated?: boolean;
}

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);

let registry: SchemaRegistry;
let validator: PalSchemaValidator;
let jsonLanguageService: LanguageService;
let initializationOptions: PalSchemaInitializationOptions = {};

function fileForUri(uri: string): string {
  try {
    return fileURLToPath(uri);
  } catch {
    return uri;
  }
}

function toLspDiagnostic(
  document: TextDocument,
  diagnostic: PalSchemaDiagnostic,
) {
  const start = document.positionAt(diagnostic.offset);
  const end = document.positionAt(
    Math.min(document.getText().length, diagnostic.offset + diagnostic.length),
  );
  return {
    range: { start, end },
    severity:
      diagnostic.severity === "error"
        ? DiagnosticSeverity.Error
        : DiagnosticSeverity.Warning,
    code: diagnostic.code,
    source: diagnostic.source,
    message: diagnostic.message,
  };
}

async function configureServices(): Promise<void> {
  registry = await SchemaRegistry.load(initializationOptions.schemaDirectory);
  validator = await PalSchemaValidator.create(
    initializationOptions.schemaDirectory,
    {
      allowMissingGenerated:
        initializationOptions.allowMissingGenerated ?? true,
    },
  );
  jsonLanguageService = getLanguageService({
    schemaRequestService: async (uri) => {
      if (!uri.startsWith("file:")) {
        throw new Error(`Unsupported schema URI: ${uri}`);
      }
      return readFile(fileURLToPath(uri), "utf8");
    },
  });
  jsonLanguageService.configure({
    allowComments: true,
    schemas: registry.index.schemas
      .filter((entry) => existsSync(registry.pathFor(entry)))
      .map((entry) => ({
        uri: pathToFileURL(registry.pathFor(entry)).toString(),
        fileMatch: entry.patterns,
      })),
  });
}

async function validateDocument(document: TextDocument): Promise<void> {
  try {
    const result = await validator.validateText(
      document.getText(),
      fileForUri(document.uri),
    );
    connection.sendDiagnostics({
      uri: document.uri,
      diagnostics: result.diagnostics.map((diagnostic) =>
        toLspDiagnostic(document, diagnostic),
      ),
    });
  } catch (error) {
    connection.console.error(
      error instanceof Error ? error.stack ?? error.message : String(error),
    );
  }
}

connection.onInitialize(async (params: InitializeParams): Promise<InitializeResult> => {
  initializationOptions =
    (params.initializationOptions as PalSchemaInitializationOptions | null) ?? {};
  await configureServices();
  return {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Incremental,
      completionProvider: {
        resolveProvider: false,
        triggerCharacters: ['"', ":"],
      },
      hoverProvider: true,
      documentSymbolProvider: true,
      documentFormattingProvider: true,
      documentRangeFormattingProvider: true,
    },
    serverInfo: {
      name: "PalSchema Language Server",
      version: registry.index.palSchemaVersion,
    },
  };
});

connection.onCompletion((params) => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    return null;
  }
  return jsonLanguageService.doComplete(
    document,
    params.position,
    jsonLanguageService.parseJSONDocument(document),
  );
});

connection.onHover((params) => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    return null;
  }
  return jsonLanguageService.doHover(
    document,
    params.position,
    jsonLanguageService.parseJSONDocument(document),
  );
});

connection.onDocumentSymbol((params) => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    return [];
  }
  return jsonLanguageService.findDocumentSymbols2(
    document,
    jsonLanguageService.parseJSONDocument(document),
  );
});

connection.onDocumentFormatting((params) => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    return [];
  }
  return jsonLanguageService.format(
    document,
    undefined,
    params.options,
  );
});

connection.onDocumentRangeFormatting((params) => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    return [];
  }
  return jsonLanguageService.format(
    document,
    params.range,
    params.options,
  );
});

documents.onDidOpen(({ document }) => void validateDocument(document));
documents.onDidChangeContent(({ document }) => void validateDocument(document));
documents.onDidClose(({ document }) => {
  connection.sendDiagnostics({ uri: document.uri, diagnostics: [] });
});

documents.listen(connection);
connection.listen();
