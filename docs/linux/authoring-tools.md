# PalSchema authoring tools

PalSchema includes a cross-platform authoring toolchain for Linux, Windows,
and macOS:

- a JSON and JSONC validator with stable diagnostics and exit codes;
- a schema registry with checksums and explicit generated-schema status;
- a Language Server Protocol (LSP) server;
- an installable VS Code-compatible extension;
- portable workspace initialization;
- watch mode for editors and command-line workflows.

The tools do not contain Palworld data, UE4SS, UEPseudo, or the
runtime-generated `enums.schema.json` and `raw.schema.json`. Those generated
schemas belong to the user's own game/runtime installation and can be copied
into a workspace when available.

## Requirements

- Node.js 20 or newer.
- npm 10 or newer is recommended.

There are no distro-specific runtime dependencies. The same npm package and
VSIX work on Arch/CachyOS, Debian/Ubuntu, Fedora/RHEL-family distributions,
openSUSE, and other current Linux distributions for which Node.js and a
VS Code-compatible editor are available.

## Build from this repository

```bash
npm ci
npm run tools:typecheck
npm run tools:test
npm run tools:build
npm run editor:build
npm run editor:package
```

The resulting packages are written to `dist/`:

- `palschema-tools-0.6.1.tgz` after running the npm pack command below;
- `palschema-vscode-0.6.1.vsix`.

Build the npm tarball with:

```bash
npm pack --workspace @palschema/tools --pack-destination dist
```

## Initialize a mod workspace

After installing the package globally or invoking its built CLI, run:

```bash
palschema init /path/to/MyPalSchemaMod
```

This creates:

- `.palschema/schemas/` with the redistributable schema pack;
- `.vscode/settings.json` with JSON and JSONC mappings;
- `palschema.config.json` with a portable relative schema path.

Existing `.vscode/settings.json` or `palschema.config.json` files are never
overwritten unless `--force` is explicitly supplied. Review existing editor
settings before using that option.

The CLI searches parent directories for `palschema.config.json`, so commands
work from nested mod folders. `--schema-dir` overrides project configuration,
and `PALSCHEMA_SCHEMA_DIR` provides an environment-level override.

## Validate and watch

```bash
palschema validate .
palschema validate --format json .
palschema validate --watch .
palschema validate --strict-generated .
```

JSONC comments and trailing commas are supported. Exit codes are:

- `0`: no errors; warnings may be present;
- `1`: at least one validation error;
- `2`: invalid arguments, configuration, or an internal failure.

By default, a missing runtime-generated schema is an error. An initialized
workspace opts into warnings with `allowMissingGenerated: true`, which keeps
offline structural checks useful while clearly reporting that enum or raw
table constraints are incomplete. Use `--strict-generated` to override that
setting for release checks.

Inspect the active schema pack with:

```bash
palschema schemas list
palschema schemas verify
palschema doctor
palschema print-config
```

`schemas verify` checks every redistributable schema against the SHA-256 value
in `schema-index.json`. Missing generated schemas are reported separately and
do not make the command fail.

## Add game-generated schemas

PalSchema writes `enums.schema.json` and `raw.schema.json` from the current
game/runtime data. Copy those two files from your own PalSchema schema output
into:

```text
.palschema/schemas/
```

Then update their `sha256` values in a private/local copy of
`schema-index.json` if you want the local pack pinned to exact generated
content. Generated entries with a `null` checksum are accepted and reported as
`present-generated`; redistributable static schemas always require an exact
checksum match. Do not commit generated Palworld data to this repository.

## VS Code and compatible editors

Install the locally built extension:

```bash
code --install-extension dist/palschema-vscode-0.6.1.vsix
```

VSCodium and many VS Code-derived editors accept the same VSIX through their
extension installation UI or equivalent CLI.

The extension's language client attaches only to JSON/JSONC documents inside
known PalSchema loader folders. It provides:

- JSON/JSONC diagnostics from the same validator as the CLI;
- property and value completion from the selected schema;
- schema hover documentation;
- document symbols;
- whole-document and range formatting.

Settings:

- `palschema.schemaDirectory`: an absolute path or a path relative to the first
  workspace folder; empty uses the bundled redistributable schemas.
- `palschema.allowMissingGenerated`: downgrade absent runtime-generated
  schemas to warnings.

Run **PalSchema: Restart Language Server** after manually replacing a schema
pack. Configuration changes restart it automatically.

## Generic LSP clients

The npm package exposes `palschema-lsp`. Editors can start it with standard I/O:

```bash
palschema-lsp --stdio
```

Pass these LSP initialization options when the client supports them:

```json
{
  "schemaDirectory": "/absolute/path/to/schemas",
  "allowMissingGenerated": true
}
```

The server implements incremental document synchronization, diagnostics,
completion, hover, symbols, and formatting. It uses only the selected local
schema directory and does not contact the game, Steam, UE4SS, or a network
service.
