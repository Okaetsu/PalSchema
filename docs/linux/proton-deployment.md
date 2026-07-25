# Safe deployment to Palworld under Proton or Wine

PalSchema is a UE4SS C++ mod. Install UE4SS separately before using these
commands; this repository neither downloads nor bundles it.

Build and inspect the Shipping DLL first:

```bash
scripts/build-linux.sh shipping
scripts/deploy-proton.sh shipping --target client --dry-run
```

The deploy helper discovers the default Steam library at
`$HOME/.local/share/Steam/steamapps/common/Palworld`. For another Steam library,
pass its Palworld directory explicitly:

```bash
scripts/deploy-proton.sh shipping \
  --target client \
  --game-dir /mnt/games/SteamLibrary/steamapps/common/Palworld \
  --dry-run
```

Remove `--dry-run` to deploy. The helper:

- verifies the Palworld Win64 executable and a separate `UE4SS.dll`;
- refuses to run while the Windows client is active;
- stages `Mods/PalSchema` before switching it into place;
- preserves the previous installation under
  `Pal/Binaries/Win64/ue4ss/.palschema-backups/`;
- prints the exact rollback command.

It does not modify UE4SS, other mods, the Proton prefix, Palworld saves, or the
native Dedicated Server.

For a Windows dedicated-server installation managed on a Linux host, select
the server target explicitly:

```bash
scripts/deploy-proton.sh shipping \
  --target server \
  --game-dir /srv/palworld-win64
```

Auto-detection prefers the client when both executable layouts exist. Use an
explicit target in automation. The server target recognizes both
`PalServer-Win64-Shipping.exe` and
`PalServer-Win64-Shipping-Cmd.exe`, refuses to deploy while either is active,
and records the target in its rollback metadata.

See [Dedicated server development on Linux](dedicated-server.md) for acquiring
and starting the Win64 server. The native Linux PalServer executable is a
different runtime and cannot load PalSchema's Win64 UE4SS C++ mod.

Use the Dev flavor when schemas, examples, VS Code settings, and PDB symbols
are needed:

```bash
scripts/deploy-proton.sh dev
```

Standalone release-compatible archives can be created with:

```bash
scripts/package-linux.sh shipping
scripts/package-linux.sh dev
```

They are written under `dist/` and preserve the upstream
`PalSchema/dlls/main.dll` layout.
