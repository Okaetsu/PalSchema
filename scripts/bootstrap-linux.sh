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
    clang-cl
    cmake
    curl
    flock
    git
    lld-link
    llvm-lib
    llvm-mt
    llvm-rc
    ninja
    sha256sum
)
missing_commands=()

for required_command in "${required_commands[@]}"; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        missing_commands+=("$required_command")
    fi
done

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

if ! command -v cargo >/dev/null 2>&1 || ! command -v rustc >/dev/null 2>&1; then
    if [[ "$install_rust_toolchain" == true ]]; then
        install_isolated_rust_toolchain
    else
        printf '%s\n' \
            "Rust and Cargo are required. Install rustup, or use the isolated setup:" \
            "  scripts/bootstrap-linux.sh --install-rust-toolchain" >&2
        exit 1
    fi
fi

rust_target="x86_64-pc-windows-msvc"
rust_target_libdir="$(rustc --print target-libdir --target "$rust_target")"

if [[ ! -d "$rust_target_libdir" ]]; then
    if [[ "$install_rust_toolchain" == true ]]; then
        if ! command -v rustup >/dev/null 2>&1; then
            install_isolated_rust_toolchain
        fi
        rustup target add "$rust_target"
    else
        printf '%s\n' \
            "Rust target $rust_target is missing." \
            "Keep the system Rust installation untouched and add an isolated toolchain with:" \
            "  scripts/bootstrap-linux.sh --install-rust-toolchain" >&2
        exit 1
    fi
fi

if ! command -v xwin >/dev/null 2>&1; then
    if [[ "$install_xwin" == true ]]; then
        cargo install --locked xwin
    else
        printf '%s\n' \
            "xwin is missing. Rerun with --install-xwin or install it with:" \
            "  cargo install --locked xwin" >&2
        exit 1
    fi
fi

if [[ -n "${XWIN_DIR:-}" ]]; then
    palschema_xwin_dir="$XWIN_DIR"
else
    palschema_xwin_dir="$PALSCHEMA_CACHE_ROOT/xwin"
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

    mkdir -p "$palschema_xwin_dir"
    xwin --accept-license splat --output "$palschema_xwin_dir"
fi

if [[ ! -d "$palschema_xwin_dir/crt" || ! -d "$palschema_xwin_dir/sdk" ]]; then
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
