# Building the Win64 PalSchema DLL on Linux

Palworld's Steam client is a Windows executable, including when Steam launches
it through Proton. The PalSchema runtime loaded by that client must therefore
remain a Win64 DLL. These instructions build that DLL entirely from Linux.

UE4SS is a separate runtime installation. PalSchema does not download, install,
or bundle UE4SS.

## Supported build path

The canonical toolchain is:

- CMake 3.22 or newer;
- Ninja;
- LLVM/Clang with `clang-cl`, `lld-link`, `llvm-lib`, `llvm-rc`, and
  `llvm-mt`;
- Rust and Cargo;
- [xwin](https://github.com/Jake-Shadle/xwin);
- the repository's pinned RE-UE4SS and UEPseudo submodules.

The generated DLL is portable across Linux distributions because it targets
the Windows ABI. The build host may use Arch/CachyOS, Debian/Ubuntu, Fedora,
openSUSE, or another distribution that provides the listed tools.

Typical host packages are:

| Distribution family | Packages |
| --- | --- |
| Arch / CachyOS | `clang cmake curl git lld llvm ninja` |
| Debian | `clang clang-tools cmake curl git lld llvm ninja-build util-linux` |
| Ubuntu | `clang clang-tools cmake curl git lld llvm ninja-build util-linux` |
| Fedora / RHEL family | `clang clang-tools-extra cmake curl git lld llvm ninja-build util-linux` |
| openSUSE | `clang cmake curl git lld llvm ninja util-linux` |

Package names can change between distribution releases. Run
`scripts/bootstrap-linux.sh` after installation; it checks the exact commands
the build uses, including `clang-cl`, `lld-link`, `llvm-lib`, `llvm-mt`,
`llvm-rc`, `llvm-ranlib`, `flock`, and Ninja. The check accepts the
distribution's version-suffixed LLVM tools, such as Ubuntu's `clang-cl-18`,
without creating system symlinks. Rust can remain project-isolated as
described below.

On 2026-07-25 the host-tool gate and shell scripts were exercised in clean
Ubuntu 24.04, Debian 13, Fedora 42, Arch Linux, and openSUSE Tumbleweed
containers. The complete SDK preparation, Dev/Shipping cross-build, PE
verification, packaging, and Proton/Wine runtime tests were run on CachyOS.

## Epic and UEPseudo access

The recursive RE-UE4SS dependency includes the private UEPseudo repository.
Your GitHub account must be linked to Epic Games and must have accepted the
EpicGames organization invitation.

Initialize the exact commits pinned by PalSchema:

```bash
git submodule update --init --recursive
```

Do not use `--remote`; it would move dependencies away from the reviewed
commits.

## Bootstrap

Run the read-only prerequisite check:

```bash
scripts/bootstrap-linux.sh
```

If xwin is missing, it can be installed explicitly:

```bash
scripts/bootstrap-linux.sh --install-xwin
```

RE-UE4SS also builds a Rust dependency for Windows. Some distributions ship
Rust without cross-target standard libraries. Install a project-isolated
toolchain and the required target without replacing the system Rust package:

```bash
scripts/bootstrap-linux.sh --install-rust-toolchain
```

The installer is downloaded from `static.rust-lang.org` and verified against
its official SHA-256 before execution. It uses
`$XDG_CACHE_HOME/palschema/{rustup,cargo}` or
`$HOME/.cache/palschema/{rustup,cargo}` and does not modify shell profiles.

xwin downloads Microsoft CRT and Windows SDK files. The project never accepts
that license silently. Review the applicable Microsoft terms and then prepare
the SDK with the explicit gate:

```bash
scripts/bootstrap-linux.sh \
  --prepare-sdk \
  --accept-microsoft-license
```

The default SDK cache is:

- `$XWIN_DIR` when set;
- otherwise `$XDG_CACHE_HOME/palschema/xwin` when set;
- otherwise `$HOME/.cache/palschema/xwin`.

## Build

Development build:

```bash
scripts/build-linux.sh dev
```

Shipping build:

```bash
scripts/build-linux.sh shipping
```

Verify the stable PE contract and emit machine-readable metadata:

```bash
python scripts/verify-win64-artifact.py \
  build/win64-xwin-shipping/PalSchema.dll \
  --json-output build/win64-xwin-shipping/pe-contract.json
```

The scripts use `CMakePresets.json`. To configure once through the same
environment setup and then invoke CMake directly:

```bash
scripts/build-linux.sh dev --configure-only
cmake --build --preset win64-xwin-dev --target PalSchema
```

Building the explicit target avoids compiling RE-UE4SS's unrelated example
programs and proxy DLL. The configure helper also selects the project-isolated
Rust toolchain when it is installed, so a distro's system Rust remains
untouched. Newer Cargo releases may normalize RE-UE4SS's pinned lock file for
the enabled feature subset. The helper routes Cargo through a serialized guard
that restores the exact pre-build lock file after every invocation, including
failed or interrupted builds. Build products remain under `build/<preset>/`;
the development DLL is written to `build/win64-xwin-dev/PalSchema.dll` and the
shipping DLL to `build/win64-xwin-shipping/PalSchema.dll`.

## Windows CI baseline

Public pull requests run source-format checks without cloning UEPseudo. The
Windows/MSVC baseline is deliberately opt-in because recursive checkout
requires authorized access to Epic-licensed UEPseudo:

- repository variable `PRIVATE_SUBMODULES_AVAILABLE=true`;
- repository secret `UEPSEUDO_TOKEN` containing a read-capable GitHub token.

The private job never uploads source trees. Its artifact contains only
`PalSchema.dll` and the JSON PE contract. Pull requests from forks cannot run
the secret-bearing job.

## What this does not provide

This target produces a Win64 DLL for the Windows Palworld client under Proton
and the Win64 Dedicated Server under Wine or Proton. It is not a native ELF
plugin for `PalServer-Linux-Shipping`. Native server support requires a stable
native UE4SS C++ mod ABI plus Linux-specific signatures, layouts, hooks, and
integration testing. See
[Dedicated server development on Linux](dedicated-server.md) for the currently
verified server path and native-runtime status.
