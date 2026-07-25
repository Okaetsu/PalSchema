#!/usr/bin/env bash

set -euo pipefail

show_usage() {
    printf '%s\n' \
        "Usage: scripts/build-linux.sh [dev|shipping] [--configure-only]" \
        "" \
        "Cross-compiles the Win64 PalSchema DLL from a Linux host."
}

build_flavor="${1:-dev}"
configure_only=false

if (($# > 0)); then
    shift
fi

while (($# > 0)); do
    case "$1" in
        --configure-only)
            configure_only=true
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

case "$build_flavor" in
    dev)
        preset="win64-xwin-dev"
        ;;
    shipping)
        preset="win64-xwin-shipping"
        ;;
    -h|--help)
        show_usage
        exit 0
        ;;
    *)
        printf 'Unknown build flavor: %s\n\n' "$build_flavor" >&2
        show_usage >&2
        exit 2
        ;;
esac

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd -- "$script_dir/.." && pwd)"
cd "$project_root"

source "$script_dir/lib/build-env.sh"
palschema_configure_build_environment

"$script_dir/bootstrap-linux.sh"

if command -v rustup >/dev/null 2>&1; then
    rust_compiler="$(rustup which rustc)"
    rust_cargo="$(rustup which cargo)"
else
    rust_compiler="$(command -v rustc)"
    rust_cargo="$(command -v cargo)"
fi

export PALSCHEMA_REAL_CARGO="$rust_cargo"

cmake \
    --preset "$preset" \
    -DRust_COMPILER="$rust_compiler" \
    -DRust_CARGO="$script_dir/cargo-preserve-lock.sh"

if [[ "$configure_only" != true ]]; then
    cmake --build --preset "$preset" --target PalSchema

    artifact="$project_root/build/$preset/PalSchema.dll"
    if [[ ! -f "$artifact" ]]; then
        printf 'Build completed without the expected artifact: %s\n' "$artifact" >&2
        exit 1
    fi

    printf 'Built %s\n' "$artifact"
fi
