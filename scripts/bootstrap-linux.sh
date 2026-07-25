#!/usr/bin/env bash

set -euo pipefail

show_usage() {
    printf '%s\n' \
        "Usage: scripts/bootstrap-linux.sh [options]" \
        "" \
        "Checks the Linux-to-Win64 build prerequisites without changing the system." \
        "" \
        "Options:" \
        "  --install-xwin              Install xwin with cargo when it is missing." \
        "  --install-rust-toolchain     Install an isolated Rust toolchain in the PalSchema cache." \
        "  --prepare-sdk               Download and unpack the Microsoft CRT/SDK." \
        "  --accept-microsoft-license  Explicitly accept the xwin Microsoft license gate." \
        "  -h, --help                  Show this help."
}

install_xwin=false
install_rust_toolchain=false
prepare_sdk=false
accept_microsoft_license=false
readonly PALSCHEMA_XWIN_VERSION="0.9.0"

while (($# > 0)); do
    case "$1" in
        --install-xwin)
            install_xwin=true
            ;;
        --install-rust-toolchain)
            install_rust_toolchain=true
            ;;
        --prepare-sdk)
            prepare_sdk=true
            ;;
        --accept-microsoft-license)
            accept_microsoft_license=true
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            printf 'Unknown option: %s\n\n' "$1" >&2
            show_usage >&2
            exit 2
            ;;
    esac
    shift
done

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$script_dir/lib/build-env.sh"
palschema_configure_build_environment

if [[ "$(uname -s)" != "Linux" ]]; then
    printf '%s\n' "This bootstrap targets Linux hosts." >&2
    exit 1
fi

required_commands=(
    cmake
    curl
    flock
    git
    ninja
    python3
    sha256sum
    sync
)
missing_commands=()

for required_command in "${required_commands[@]}"; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        missing_commands+=("$required_command")
    fi
done

for llvm_tool in clang-cl lld-link llvm-mt llvm-rc llvm-ranlib; do
    if ! palschema_llvm_tool_available "$llvm_tool"; then
        missing_commands+=("$llvm_tool")
    fi
done
if ! palschema_llvm_tool_available llvm-lib &&
    ! palschema_llvm_tool_available llvm-ar; then
    missing_commands+=("llvm-lib/llvm-ar")
fi

if ((${#missing_commands[@]} > 0)); then
    printf 'Missing required commands: %s\n' "${missing_commands[*]}" >&2
    printf '%s\n' \
        "Install the equivalent packages for your distribution, then rerun this script." >&2
    exit 1
fi

install_isolated_rust_toolchain() {
    case "$(uname -m)" in
        x86_64)
            rustup_host="x86_64-unknown-linux-gnu"
            ;;
        aarch64|arm64)
            rustup_host="aarch64-unknown-linux-gnu"
            ;;
        *)
            printf 'Unsupported rustup host architecture: %s\n' "$(uname -m)" >&2
            exit 1
            ;;
    esac

    rustup_base_url="https://static.rust-lang.org/rustup/dist/$rustup_host/rustup-init"
    rustup_download_dir="$(mktemp -d "${TMPDIR:-/tmp}/palschema-rustup.XXXXXX")"
    trap 'rm -rf -- "$rustup_download_dir"' EXIT

    curl --fail --location --silent --show-error \
        --output "$rustup_download_dir/rustup-init" \
        "$rustup_base_url"
    curl --fail --location --silent --show-error \
        --output "$rustup_download_dir/rustup-init.sha256" \
        "$rustup_base_url.sha256"

    expected_sha256="$(awk '{print $1}' "$rustup_download_dir/rustup-init.sha256")"
    actual_sha256="$(sha256sum "$rustup_download_dir/rustup-init" | awk '{print $1}')"

    if [[ "$actual_sha256" != "$expected_sha256" ]]; then
        printf '%s\n' "rustup-init SHA-256 verification failed." >&2
        exit 1
    fi

    chmod +x "$rustup_download_dir/rustup-init"
    mkdir -p "$PALSCHEMA_RUSTUP_HOME" "$PALSCHEMA_CARGO_HOME"

    RUSTUP_HOME="$PALSCHEMA_RUSTUP_HOME" \
    CARGO_HOME="$PALSCHEMA_CARGO_HOME" \
    RUSTUP_INIT_SKIP_PATH_CHECK=yes \
        "$rustup_download_dir/rustup-init" \
        --default-toolchain stable \
        --no-modify-path \
        --profile minimal \
        -y

    export RUSTUP_HOME="$PALSCHEMA_RUSTUP_HOME"
    export CARGO_HOME="$PALSCHEMA_CARGO_HOME"
    export PATH="$CARGO_HOME/bin:$PATH"
}

if [[ "$install_rust_toolchain" == true ]]; then
    if [[ ! -x "$PALSCHEMA_CARGO_HOME/bin/rustup" ]]; then
        install_isolated_rust_toolchain
    fi
    export RUSTUP_HOME="$PALSCHEMA_RUSTUP_HOME"
    export CARGO_HOME="$PALSCHEMA_CARGO_HOME"
    export PATH="$CARGO_HOME/bin:$PATH"
fi

if ! command -v cargo >/dev/null 2>&1 || ! command -v rustc >/dev/null 2>&1; then
    printf '%s\n' \
        "Rust and Cargo are required. Install rustup, or use the isolated setup:" \
        "  scripts/bootstrap-linux.sh --install-rust-toolchain" >&2
    exit 1
fi

rust_target="x86_64-pc-windows-msvc"
rust_target_libdir="$(rustc --print target-libdir --target "$rust_target")"

if [[ ! -d "$rust_target_libdir" ]]; then
    if [[ "$install_rust_toolchain" == true ]]; then
        rustup target add "$rust_target"
    else
        printf '%s\n' \
            "Rust target $rust_target is missing." \
            "Keep the system Rust installation untouched and add an isolated toolchain with:" \
            "  scripts/bootstrap-linux.sh --install-rust-toolchain" >&2
        exit 1
    fi
fi

if [[ -n "${XWIN_DIR:-}" ]]; then
    palschema_xwin_dir="$XWIN_DIR"
else
    palschema_xwin_dir="$PALSCHEMA_CACHE_ROOT/xwin"
fi

xwin_cache_payload_is_valid() {
    local cache_dir="$1"
    [[ -f "$cache_dir/crt/include/vcruntime.h" &&
       -f "$cache_dir/crt/lib/x86_64/msvcrt.lib" &&
       -f "$cache_dir/sdk/include/um/Windows.h" &&
       -f "$cache_dir/sdk/lib/ucrt/x86_64/ucrt.lib" &&
       -f "$cache_dir/sdk/lib/um/x86_64/kernel32.Lib" ]]
}

xwin_cache_is_ready() {
    local cache_dir="$1"
    xwin_cache_payload_is_valid "$cache_dir" &&
        [[ -f "$cache_dir/.palschema-sdk-complete" ]]
}

fsync_directory() {
    python3 - "$1" <<'PY'
import os
import sys

descriptor = os.open(sys.argv[1], os.O_RDONLY | os.O_DIRECTORY)
try:
    os.fsync(descriptor)
finally:
    os.close(descriptor)
PY
}

xwin_parent="$(dirname -- "$palschema_xwin_dir")"
xwin_name="$(basename -- "$palschema_xwin_dir")"
xwin_previous="$xwin_parent/.${xwin_name}.previous"
mkdir -p "$xwin_parent"
exec {xwin_lock_fd}> "$xwin_parent/.${xwin_name}.lock"
flock "$xwin_lock_fd"

# Recover the last complete cache if a previous refresh was interrupted between
# the two directory renames.
if ! xwin_cache_is_ready "$palschema_xwin_dir" &&
    xwin_cache_is_ready "$xwin_previous"; then
    if [[ -e "$palschema_xwin_dir" ]]; then
        xwin_incomplete="$xwin_parent/.${xwin_name}.incomplete.$$"
        mv -- "$palschema_xwin_dir" "$xwin_incomplete"
    else
        xwin_incomplete=""
    fi
    mv -- "$xwin_previous" "$palschema_xwin_dir"
    fsync_directory "$xwin_parent"
    if [[ -n "$xwin_incomplete" ]]; then
        rm -rf -- "$xwin_incomplete"
    fi
fi

xwin_is_pinned() {
    command -v xwin >/dev/null 2>&1 &&
        [[ "$(xwin --version 2>/dev/null)" == "xwin $PALSCHEMA_XWIN_VERSION" ]]
}

# xwin is needed to create the SDK cache, but not to consume a complete cache
# during an offline or clean-container build.
if ! xwin_is_pinned &&
    { [[ "$install_xwin" == true ]] ||
      { [[ "$prepare_sdk" == true ]] &&
        ! xwin_cache_is_ready "$palschema_xwin_dir"; }; }; then
    if [[ "$install_xwin" == true ]]; then
        cargo install --locked --version "$PALSCHEMA_XWIN_VERSION" xwin
        if ! xwin_is_pinned; then
            printf 'Installed xwin does not report pinned version %s.\n' \
                "$PALSCHEMA_XWIN_VERSION" >&2
            exit 1
        fi
    else
        printf '%s\n' \
            "The pinned xwin version is required to prepare the Microsoft CRT/SDK cache." \
            "Rerun with --install-xwin or install it with:" \
            "  cargo install --locked --version $PALSCHEMA_XWIN_VERSION xwin" >&2
        exit 1
    fi
fi

if [[ "$accept_microsoft_license" == true && "$prepare_sdk" != true ]]; then
    printf '%s\n' \
        "--accept-microsoft-license is only meaningful together with --prepare-sdk." >&2
    exit 2
fi

if [[ "$prepare_sdk" == true ]]; then
    if [[ "$accept_microsoft_license" != true ]]; then
        printf '%s\n' \
            "SDK preparation requires explicit license acceptance." \
            "Review Microsoft's terms, then rerun with both:" \
            "  --prepare-sdk --accept-microsoft-license" >&2
        exit 2
    fi

    if xwin_cache_is_ready "$palschema_xwin_dir"; then
        printf 'Reusing complete xwin SDK cache at %s.\n' "$palschema_xwin_dir"
    else
        xwin_stage="$(mktemp -d "$xwin_parent/.${xwin_name}.stage.XXXXXX")"
        if ! xwin --accept-license splat --output "$xwin_stage"; then
            rm -rf -- "$xwin_stage"
            exit 1
        fi
        if ! xwin_cache_payload_is_valid "$xwin_stage"; then
            rm -rf -- "$xwin_stage"
            printf '%s\n' "xwin produced an incomplete CRT/SDK cache." >&2
            exit 1
        fi
        printf '%s\n' "xwin-splat-complete" \
            > "$xwin_stage/.palschema-sdk-complete"
        # Flush the staged SDK and completion marker before publishing it.
        sync -f "$xwin_stage"
        fsync_directory "$xwin_stage"
        fsync_directory "$xwin_parent"

        if [[ -e "$xwin_previous" ]]; then
            rm -rf -- "$xwin_previous"
            fsync_directory "$xwin_parent"
        fi
        if [[ -e "$palschema_xwin_dir" ]]; then
            mv -- "$palschema_xwin_dir" "$xwin_previous"
            fsync_directory "$xwin_parent"
        fi
        if ! mv -- "$xwin_stage" "$palschema_xwin_dir"; then
            if [[ ! -e "$palschema_xwin_dir" && -e "$xwin_previous" ]]; then
                mv -- "$xwin_previous" "$palschema_xwin_dir"
            fi
            exit 1
        fi
        fsync_directory "$xwin_parent"
        if [[ -e "$xwin_previous" ]]; then
            rm -rf -- "$xwin_previous"
            fsync_directory "$xwin_parent"
        fi
    fi
fi

if ! xwin_cache_is_ready "$palschema_xwin_dir"; then
    printf 'The xwin SDK is not prepared at %s.\n' "$palschema_xwin_dir" >&2
    printf '%s\n' \
        "After reviewing the Microsoft terms, prepare it explicitly with:" \
        "  scripts/bootstrap-linux.sh --prepare-sdk --accept-microsoft-license" >&2
    exit 1
fi

if [[ ! -f deps/RE-UE4SS/deps/first/Unreal/CMakeLists.txt ]]; then
    printf '%s\n' \
        "UEPseudo is missing. Initialize the authorized recursive submodules with:" \
        "  git submodule update --init --recursive" >&2
    exit 1
fi

printf '%s\n' \
    "PalSchema Linux-to-Win64 prerequisites are ready." \
    "XWIN_DIR=$palschema_xwin_dir" \
    "Build Dev:      scripts/build-linux.sh dev" \
    "Build Shipping: scripts/build-linux.sh shipping"
