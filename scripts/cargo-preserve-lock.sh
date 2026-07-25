#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd -- "$script_dir/.." && pwd)"

source "$script_dir/lib/build-env.sh"
palschema_configure_build_environment

real_cargo="${PALSCHEMA_REAL_CARGO:-}"
if [[ -z "$real_cargo" ]]; then
    if command -v rustup >/dev/null 2>&1; then
        real_cargo="$(rustup which cargo)"
    else
        real_cargo="$(command -v cargo)"
    fi
fi

if [[ "$real_cargo" == "$0" || "$real_cargo" == "$script_dir/cargo-preserve-lock.sh" ]]; then
    printf '%s\n' "Cargo lockfile guard resolved itself instead of the real Cargo binary." >&2
    exit 1
fi

lock_file="$project_root/deps/RE-UE4SS/deps/first/patternsleuth_bind/Cargo.lock"
if [[ ! -f "$lock_file" ]]; then
    exec "$real_cargo" "$@"
fi

mkdir -p "$PALSCHEMA_CACHE_ROOT"
guard_file="$PALSCHEMA_CACHE_ROOT/patternsleuth-bind-cargo.lock.guard"
exec {guard_fd}>"$guard_file"
flock "$guard_fd"

lock_backup="$(mktemp "$PALSCHEMA_CACHE_ROOT/patternsleuth-bind-cargo.lock.XXXXXX")"
cp -p -- "$lock_file" "$lock_backup"

cleanup() {
    local status=$?

    trap - EXIT
    if ! cmp -s -- "$lock_backup" "$lock_file"; then
        if ! cp -p -- "$lock_backup" "$lock_file"; then
            printf 'Failed to restore %s after Cargo exited.\n' "$lock_file" >&2
            status=1
        fi
    fi
    if ! rm -f -- "$lock_backup"; then
        printf 'Failed to remove temporary lockfile backup: %s\n' "$lock_backup" >&2
        status=1
    fi

    exit "$status"
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

"$real_cargo" "$@"
