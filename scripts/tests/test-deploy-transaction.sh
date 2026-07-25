#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/palschema-deploy-test.XXXXXX")"
trap 'rm -rf -- "$test_root"' EXIT

project_root="$test_root/project"
game_root="$test_root/game"
fake_bin="$test_root/bin"
mkdir -p \
    "$project_root/scripts" \
    "$project_root/scripts/lib" \
    "$project_root/build/win64-xwin-shipping" \
    "$game_root/Pal/Binaries/Win64/ue4ss/Mods/PalSchema/dlls" \
    "$fake_bin"
cp -- "$repository_root/scripts/deploy-proton.sh" "$project_root/scripts/"
cp -- "$repository_root/scripts/verify-win64-artifact.py" "$project_root/scripts/"
cp -- "$repository_root/scripts/lib/build-env.sh" "$project_root/scripts/lib/"
cp -- "$repository_root/scripts/lib/process-scan.sh" "$project_root/scripts/lib/"
cp -- "$repository_root/version.h" "$project_root/version.h"
touch "$game_root/Pal/Binaries/Win64/PalServer-Win64-Shipping-Cmd.exe"
touch "$game_root/Pal/Binaries/Win64/ue4ss/UE4SS.dll"
printf '%s\n' "old-install" \
    > "$game_root/Pal/Binaries/Win64/ue4ss/Mods/PalSchema/dlls/main.dll"

python3 - "$project_root/build/win64-xwin-shipping/PalSchema.dll" <<'PY'
from pathlib import Path
import sys

Path(sys.argv[1]).write_bytes(
    "PalSchema\0".encode("utf-16-le") + "0.6.1.0\0".encode("utf-16-le")
)
PY

cat > "$fake_bin/llvm-readobj" <<'EOF'
#!/usr/bin/env sh
cat <<'OUTPUT'
Format: COFF-x86-64
IMAGE_FILE_DLL
IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE
IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA
IMAGE_DLL_CHARACTERISTICS_NX_COMPAT
Export {
  Name: start_mod
}
Export {
  Name: uninstall_mod
}
Import {
  Name: UE4SS.dll
}
Type: VERSIONINFO
Data (
  0000: 50007200 6F006400 75006300 74004E00 |data|
  0010: 61006D00 65000000 50006100 6C005300 |data|
  0020: 63006800 65006D00 61000000 50007200 |data|
  0030: 6F006400 75006300 74005600 65007200 |data|
  0040: 73006900 6F006E00 00003000 2E003600 |data|
  0050: 2E003100 2E003000 00000000 |data|
)
OUTPUT
EOF
chmod +x "$fake_bin/llvm-readobj"

set +e
PATH="$fake_bin:$PATH" \
PALSCHEMA_TEST_INTERRUPT_AFTER_BACKUP=1 \
    "$project_root/scripts/deploy-proton.sh" shipping \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/interrupted.out" 2>"$test_root/interrupted.err"
interrupt_status=$?
set -e
if ((interrupt_status != 143)); then
    printf 'Expected interrupted deploy to exit 143, got %s.\n' \
        "$interrupt_status" >&2
    exit 1
fi
if [[ "$(cat "$game_root/Pal/Binaries/Win64/ue4ss/Mods/PalSchema/dlls/main.dll")" \
      != "old-install" ]]; then
    printf '%s\n' "Interrupted deploy did not restore the previous installation." >&2
    exit 1
fi
if [[ -e "$game_root/Pal/Binaries/Win64/ue4ss/.palschema-backups/.active-transaction" ]]; then
    printf '%s\n' "Interrupted deploy left an active transaction marker." >&2
    exit 1
fi

backup_root="$game_root/Pal/Binaries/Win64/ue4ss/.palschema-backups"
mods_root="$game_root/Pal/Binaries/Win64/ue4ss/Mods"
target_root="$mods_root/PalSchema"
crash_id="20000101T000000Z-2"
stale_stage_name=".PalSchema.stage.CRASHED"
mkdir -p \
    "$backup_root/$crash_id/PalSchema/dlls" \
    "$mods_root/$stale_stage_name/dlls"
printf '%s\n' "power-loss-old-install" \
    > "$backup_root/$crash_id/PalSchema/dlls/main.dll"
printf '%s\n' "abandoned-stage" \
    > "$mods_root/$stale_stage_name/dlls/main.dll"
rm -rf -- "$target_root"
printf '%s\n%s\n' "$crash_id" "$stale_stage_name" \
    > "$backup_root/.active-transaction"

PATH="$fake_bin:$PATH" \
    "$project_root/scripts/deploy-proton.sh" shipping \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/success.out"
if ! grep -q "Recovered interrupted PalSchema transaction" \
    "$test_root/success.out"; then
    printf '%s\n' "A persisted power-loss transaction was not recovered." >&2
    exit 1
fi
if ! grep -q "Deployed PalSchema" "$test_root/success.out"; then
    printf '%s\n' "Recovered deployment did not complete." >&2
    exit 1
fi
if [[ -e "$mods_root/$stale_stage_name" ||
      -e "$backup_root/.active-transaction" ]]; then
    printf '%s\n' "Power-loss recovery left stale transaction state." >&2
    exit 1
fi
if ! grep -R -q "power-loss-old-install" "$backup_root"/*/PalSchema/dlls/main.dll; then
    printf '%s\n' "Recovered installation was not preserved by the next deploy." >&2
    exit 1
fi

live_hash="$(sha256sum "$target_root/dlls/main.dll" | awk '{print $1}')"
missing_id="20000101T000000Z-3"
printf '%s\n\n' "$missing_id" > "$backup_root/.active-transaction"
set +e
PATH="$fake_bin:$PATH" \
    "$project_root/scripts/deploy-proton.sh" shipping \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/missing-backup.out" 2>"$test_root/missing-backup.err"
missing_backup_status=$?
set -e
if ((missing_backup_status == 0)) ||
   ! grep -q "transaction backup is missing" "$test_root/missing-backup.err"; then
    printf '%s\n' "Missing transaction backup did not fail closed." >&2
    exit 1
fi
if [[ "$(sha256sum "$target_root/dlls/main.dll" | awk '{print $1}')" != "$live_hash" ]]; then
    printf '%s\n' "Missing-backup recovery modified the live installation." >&2
    exit 1
fi
rm -f -- "$backup_root/.active-transaction"

symlink_id="20000101T000000Z-4"
external_recovery="$test_root/external-recovery"
mkdir -p "$external_recovery/PalSchema/dlls"
printf '%s\n' "external-sentinel" \
    > "$external_recovery/PalSchema/dlls/main.dll"
ln -s "$external_recovery" "$backup_root/$symlink_id"
printf '%s\n\n' "$symlink_id" > "$backup_root/.active-transaction"
set +e
PATH="$fake_bin:$PATH" \
    "$project_root/scripts/deploy-proton.sh" shipping \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/symlink-recovery.out" 2>"$test_root/symlink-recovery.err"
symlink_recovery_status=$?
set -e
if ((symlink_recovery_status == 0)) ||
   ! grep -q "backup contains a symlink" "$test_root/symlink-recovery.err"; then
    printf '%s\n' "Symlinked transaction backup did not fail closed." >&2
    exit 1
fi
if [[ "$(cat "$external_recovery/PalSchema/dlls/main.dll")" != "external-sentinel" ]]; then
    printf '%s\n' "Symlinked recovery moved or modified the external directory." >&2
    exit 1
fi
rm -f -- "$backup_root/.active-transaction" "$backup_root/$symlink_id"

printf '%s\n' "not-a-valid-transaction" > "$backup_root/.active-transaction"
set +e
PATH="$fake_bin:$PATH" \
    "$project_root/scripts/deploy-proton.sh" shipping \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/malformed.out" 2>"$test_root/malformed.err"
malformed_status=$?
set -e
if ((malformed_status == 0)) ||
   ! grep -q "Invalid deployment transaction marker" "$test_root/malformed.err"; then
    printf '%s\n' "Malformed transaction marker did not fail closed." >&2
    exit 1
fi
rm -f -- "$backup_root/.active-transaction"

bad_backup="$game_root/Pal/Binaries/Win64/ue4ss/.palschema-backups/20000101T000000Z-1"
mkdir -p "$bad_backup/PalSchema/dlls"
ln -s /definitely-missing "$bad_backup/PalSchema/dlls/main.dll"
set +e
PATH="$fake_bin:$PATH" \
    "$project_root/scripts/deploy-proton.sh" \
        --rollback "$bad_backup" \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/rollback.out" 2>"$test_root/rollback.err"
rollback_status=$?
set -e
if ((rollback_status == 0)); then
    printf '%s\n' "Rollback accepted an invalid staged main.dll." >&2
    exit 1
fi
if [[ ! -f "$game_root/Pal/Binaries/Win64/ue4ss/Mods/PalSchema/dlls/main.dll" ]]; then
    printf '%s\n' "Rejected rollback modified the live installation." >&2
    exit 1
fi

printf '%s\n' "PalSchema deploy transaction tests passed."
