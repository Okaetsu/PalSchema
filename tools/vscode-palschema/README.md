# PalSchema for VS Code

This extension adds PalSchema-aware diagnostics, JSON/JSONC completion, hover
documentation, document symbols, and formatting on Linux, Windows, and macOS.

Its language features attach only to JSON or JSONC files below a PalSchema
loader directory such as `items`, `pals`, `raw`, or `blueprints`. Static
schemas are bundled. The game-generated `enums.schema.json` and
`raw.schema.json` can be selected with the `palschema.schemaDirectory` setting
after copying them from a current PalSchema runtime.

UE4SS, game data, and generated proprietary schemas are not bundled.
