#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/palschema-process-scan.XXXXXX")"
trap 'rm -rf -- "$test_root"' EXIT
source "$repository_root/scripts/lib/process-scan.sh"

mkdir -p "$test_root/100"
printf 'wine\0C:\\PalServer-Win64-Shipping-Cmd.exe\0' \
    > "$test_root/100/cmdline"
if ! palschema_target_process_status server "$test_root"; then
    printf '%s\n' "Process scan did not detect a matching Win64 server." >&2
    exit 1
fi

rm -rf -- "$test_root/100"
mkdir -p "$test_root/200"
printf 'unrelated\0process\0' > "$test_root/200/cmdline"
set +e
palschema_target_process_status server "$test_root"
negative_status=$?
set -e
if ((negative_status != 1)); then
    printf 'Expected a complete negative scan, got status %s.\n' \
        "$negative_status" >&2
    exit 1
fi

rm -rf -- "$test_root/200"
set +e
palschema_target_process_status server "$test_root"
empty_status=$?
palschema_target_process_status server "$test_root/missing"
missing_status=$?
set -e
if ((empty_status != 2 || missing_status != 2)); then
    printf 'Expected empty and missing proc roots to fail closed, got %s and %s.\n' \
        "$empty_status" "$missing_status" >&2
    exit 1
fi

mkdir -p "$test_root/300"
python3 - "$test_root/300/cmdline" <<'PY'
import socket
import sys

sock = socket.socket(socket.AF_UNIX)
sock.bind(sys.argv[1])
sock.close()
PY
set +e
palschema_target_process_status server "$test_root"
incomplete_status=$?
set -e
if ((incomplete_status != 2)); then
    printf 'Expected an incomplete scan to fail closed with status 2, got %s.\n' \
        "$incomplete_status" >&2
    exit 1
fi

printf '%s\n' "PalSchema process scan tests passed."
