# PalSchema authoring tools

Cross-platform JSON/JSONC validation and Language Server Protocol support for
PalSchema mod projects.

Install a repository-built tarball:

```bash
npm install --global ./palschema-tools-0.6.1.tgz
```

Initialize a workspace and validate it:

```bash
palschema init /path/to/MyPalSchemaMod
palschema validate /path/to/MyPalSchemaMod
```

Start the language server for a generic editor:

```bash
palschema-lsp --stdio
```

Run `palschema --help` for all commands. Full build, configuration, generated
schema, and editor instructions are in
`docs/linux/authoring-tools.md` in the PalSchema repository.

The package contains redistributable static schemas only. It does not bundle
Palworld data, UE4SS, UEPseudo, or runtime-generated enum/raw schemas.
