#!/usr/bin/env bash

# Shared environment setup for PalSchema's Linux-hosted cross-build scripts.
# This file is sourced; it intentionally does not enable shell options.

PALSCHEMA_LLVM_VERSION_SUFFIXES=(22 21 20 19 18 17 16 15)

palschema_configure_build_environment() {
    if [[ -n "${XDG_CACHE_HOME:-}" ]]; then
        palschema_cache_root="$XDG_CACHE_HOME/palschema"
    else
        user_home_dir="${HOME:?HOME must be set when XDG_CACHE_HOME is unset}"
        palschema_cache_root="$user_home_dir/.cache/palschema"
    fi

    export PALSCHEMA_CACHE_ROOT="${PALSCHEMA_CACHE_ROOT:-$palschema_cache_root}"
    export PALSCHEMA_RUSTUP_HOME="${PALSCHEMA_RUSTUP_HOME:-$PALSCHEMA_CACHE_ROOT/rustup}"
    export PALSCHEMA_CARGO_HOME="${PALSCHEMA_CARGO_HOME:-$PALSCHEMA_CACHE_ROOT/cargo}"

    if [[ -x "$PALSCHEMA_CARGO_HOME/bin/rustup" ]]; then
        export RUSTUP_HOME="$PALSCHEMA_RUSTUP_HOME"
        export CARGO_HOME="$PALSCHEMA_CARGO_HOME"
        export PATH="$CARGO_HOME/bin:$PATH"
    fi
}

palschema_find_llvm_tool() {
    local tool_name="$1"
    local candidate
    local suffix

    if command -v "$tool_name" >/dev/null 2>&1; then
        command -v "$tool_name"
        return 0
    fi
    for suffix in "${PALSCHEMA_LLVM_VERSION_SUFFIXES[@]}"; do
        candidate="$tool_name-$suffix"
        if command -v "$candidate" >/dev/null 2>&1; then
            command -v "$candidate"
            return 0
        fi
    done

    printf 'Unable to find %s (including supported version-suffixed names).\n' \
        "$tool_name" >&2
    return 1
}

palschema_llvm_tool_available() {
    palschema_find_llvm_tool "$1" >/dev/null 2>&1
}
