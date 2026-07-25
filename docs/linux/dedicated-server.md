# Dedicated server development on Linux

PalSchema's supported dedicated-server path on a Linux host is the **Win64
Palworld Dedicated Server running through Wine or Proton**. It uses the same
PalSchema DLL and UE4SS C++ mod ABI as the Windows client.

The native `PalServer-Linux-Shipping` binary is not currently a verified 1:1
PalSchema target. PalSchema is presently built as a Win64 UE4SS C++ mod, while
the upstream native UE4SS port remains under development. The upstream
headless Linux draft explicitly leaves C++ mods outside its initial scope:
[UE4SS pull request 1347](https://github.com/UE4SS-RE/RE-UE4SS/pull/1347).
Downstream Linux experiments may accept a native `CppUserModBase`/`main.so`
mod at the loader boundary; that is narrower than proving PalSchema's
signatures, layouts, hooks, threading, teardown, and generated schemas against
the native Palworld server. No such downstream and PalSchema combination is
pinned or verified by this repository yet, so it must not be treated as a
drop-in replacement for this workflow.

## Ownership boundary

This repository builds and deploys only PalSchema.

You must install and maintain UE4SS yourself. PalSchema does not download,
redistribute, package, or modify UE4SS. The same boundary applies to Palworld
and its Dedicated Server files.

## Requirements

- a recent 64-bit Wine or Proton installation;
- SteamCMD;
- a separately acquired Win64 Palworld Dedicated Server;
- a compatible UE4SS installation placed in that server;
- a completed PalSchema Shipping build.

The workflow uses portable shell, CMake, Ninja, clang-cl, Node.js, and Wine
interfaces. It is not tied to CachyOS or Arch. Package names differ, but the
same repository commands apply on current Arch/CachyOS, Debian/Ubuntu,
Fedora/RHEL-family, openSUSE, and other major distributions.

## Acquire an isolated Win64 server

SteamCMD can download the Windows depot even when SteamCMD itself runs on
Linux. Choose a dedicated directory that does not overlap an existing native
server:

```bash
steamcmd \
  +@sSteamCmdForcePlatformType windows \
  +force_install_dir /srv/palworld-win64 \
  +login anonymous \
  +app_update 2394010 validate \
  +quit
```

Valve documents `@sSteamCmdForcePlatformType` in the
[SteamCMD reference](https://developer.valvesoftware.com/wiki/SteamCMD).
Back up any existing server configuration and save data before updating.

Install UE4SS into the resulting Win64 server according to the UE4SS and
Palworld-specific instructions you have chosen. Before continuing, the layout
must include:

```text
/srv/palworld-win64/
└── Pal/Binaries/Win64/
    ├── PalServer-Win64-Shipping-Cmd.exe
    ├── dwmapi.dll
    └── ue4ss/
        ├── UE4SS.dll
        ├── UE4SS-settings.ini
        ├── MemberVariableLayout.ini
        └── Mods/
```

These UE4SS files are examples of the required external installation. They
are never copied from or included in a PalSchema package.

## Build and deploy PalSchema

```bash
scripts/build-linux.sh shipping
scripts/deploy-proton.sh shipping \
  --target server \
  --game-dir /srv/palworld-win64 \
  --dry-run
```

Review the paths, remove `--dry-run`, and keep the printed rollback command.
Deployment uses a locked, journaled replacement transaction, creates a
timestamped backup, and refuses to proceed while the selected Win64 server is
active or process visibility is incomplete. A signal restores the previous
installation; after host or power interruption, the next deploy/rollback
recovers the recorded transaction before making another change.

## Start under Wine

Use a dedicated Wine prefix so server dependencies and settings remain
isolated:

```bash
cd /srv/palworld-win64/Pal/Binaries/Win64

WINEPREFIX=/srv/palworld-win64/.compat/pfx \
WINEDLLOVERRIDES='dwmapi=n,b' \
WINEDEBUG=-all \
SteamAppId=2394010 \
wine ./PalServer-Win64-Shipping-Cmd.exe Pal \
  -port=8211 \
  -queryport=27015 \
  -players=32 \
  -useperfthreads \
  -NoAsyncLoadingThread \
  -UseMultithreadForDS
```

Adjust ports and normal Palworld server arguments for your environment. Do not
reuse ports already owned by another server.

PalSchema detects Wine at runtime and selects UE4SS's synchronous signature
scanner path during startup. Native Windows keeps the normal parallel scanner.
This avoids a Wine `std::async` startup deadlock without changing signatures,
loader behavior, or the produced Win64 DLL ABI.

When PalSchema auto-reload is enabled, Wine uses efsw's polling backend because
Wine does not reliably deliver the Win32 directory-change notifications used
by efsw's native Windows backend. Native Windows keeps the event-driven
backend. The selected fallback is reported in `UE4SS.log`.

## Verification

Check `Pal/Binaries/Win64/ue4ss/UE4SS.log` for:

- `PalSchema v... loaded`;
- every expected `Found ...` signature line;
- `Event loop start`;
- initialization of `enums`, `raw`, `blueprints`, `resources`, `pals`, `npcs`,
  `items`, `skins`, `appearance`, `buildings`, `helpguide`, `spawns`, and
  `translations`.

Also verify that the configured game and query ports are open. A process that
exists but never opens its ports is not a successful smoke test.

The repeatable smoke harness performs those checks, stops only the selected
Wine process group and prefix between cycles, and writes one evidence directory
per run:

```bash
scripts/test-win64-server.sh \
  --game-dir /srv/palworld-win64 \
  --cycles 3
```

Auto-reload can be included by pointing at a harmless fixture that already
exists inside one PalSchema mod. The harness only updates that file's
modification time:

```bash
scripts/test-win64-server.sh \
  --game-dir /srv/palworld-win64 \
  --cycles 3 \
  --hot-reload-file \
    /srv/palworld-win64/Pal/Binaries/Win64/ue4ss/Mods/PalSchema/mods/Smoke/translations/en/smoke.json
```

Automatic discovery is intentionally disabled: the server root must be
explicit so the harness cannot select another Palworld installation.

The Linux-hosted workflow was exercised on 2026-07-25 against Win64 Dedicated
Server Steam build `24181105` under Wine 11.13. Five consecutive startup and
shutdown cycles found all 22 PalSchema signatures, initialized all 13 loaders,
reached UE4SS's event loop, opened both isolated UDP ports, and released both
ports after shutdown. Three additional cycles triggered and observed
PalSchema auto-reload through the Wine polling fallback. The same DLL was then
verified against the installed Palworld client under Proton. This is a
reproducibility record, not a promise that future Palworld or UE4SS updates
cannot change signatures or compatibility.

## Updating and rollback

Stop the Win64 server before every update or rollback. Rebuild, deploy, and
repeat the verification above after Palworld, UE4SS, Wine/Proton, or PalSchema
changes.

Use the exact rollback command printed by `deploy-proton.sh`. It restores only
the previous `Mods/PalSchema` state and leaves UE4SS, other mods, the Wine
prefix, configuration, and saves untouched.
