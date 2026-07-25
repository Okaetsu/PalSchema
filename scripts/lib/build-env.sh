#!/usr/bin/env bash

# Shared environment setup for PalSchema's Linux-hosted cross-build scripts.
# This file is sourced; it intentionally does not enable shell options.

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
