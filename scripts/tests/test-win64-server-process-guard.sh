#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/palschema-server-guard.XXXXXX")"
guard_pid=""
cleanup() {
    if [[ -n "$guard_pid" ]]; then
        kill "$guard_pid" >/dev/null 2>&1 || true
    fi
    rm -rf -- "$test_root"
}
trap cleanup EXIT

project_root="$test_root/project"
game_root="$test_root/game"
fake_bin="$test_root/bin"
win64_root="$game_root/Pal/Binaries/Win64"
ue4ss_root="$win64_root/ue4ss"
mkdir -p \
    "$project_root/scripts/lib" \
    "$ue4ss_root/Mods/PalSchema/dlls" \
    "$fake_bin"
cp -- "$repository_root/scripts/test-win64-server.sh" "$project_root/scripts/"
cp -- "$repository_root/scripts/lib/process-scan.sh" "$project_root/scripts/lib/"
touch \
    "$win64_root/PalServer-Win64-Shipping-Cmd.exe" \
    "$win64_root/dwmapi.dll" \
    "$ue4ss_root/UE4SS.dll" \
    "$ue4ss_root/UE4SS-settings.ini" \
    "$ue4ss_root/MemberVariableLayout.ini" \
    "$ue4ss_root/Mods/PalSchema/dlls/main.dll"

for command_name in ss wine wineserver; do
    printf '%s\n' '#!/usr/bin/env sh' 'exit 0' > "$fake_bin/$command_name"
    chmod +x "$fake_bin/$command_name"
done

bash -c 'exec -a ./PalServer-Win64-Shipping-Cmd.exe sleep 30' &
guard_pid=$!
sleep 0.1

set +e
PATH="$fake_bin:$PATH" \
    "$project_root/scripts/test-win64-server.sh" \
        --game-dir "$game_root" \
        --cycles 1 \
        >"$test_root/guard.out" 2>"$test_root/guard.err"
status=$?
set -e

if ((status == 0)) ||
   ! grep -q "already running" "$test_root/guard.err"; then
    printf '%s\n' \
        "Server smoke harness did not reject a relative executable command line." >&2
    exit 1
fi

printf '%s\n' "PalSchema Win64 server process guard tests passed."
