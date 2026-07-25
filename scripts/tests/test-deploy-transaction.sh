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
#!/usr/bin/env python3
import struct


def align4(content):
    return content + b"\0" * (-len(content) % 4)


def node(key, value=b"", value_type=0, children=()):
    encoded_key = f"{key}\0".encode("utf-16-le")
    if isinstance(value, str):
        encoded_value = f"{value}\0".encode("utf-16-le")
        value_length = len(encoded_value) // 2
    else:
        encoded_value = value
        value_length = len(encoded_value)
    content = align4(b"\0" * 6 + encoded_key)
    content += encoded_value
    content = align4(content)
    content += b"".join(align4(child) for child in children)
    return struct.pack("<HHH", len(content), value_length, value_type) + content[6:]


version_ms = 6
version_ls = 1 << 16
fixed = struct.pack(
    "<13I",
    0xFEEF04BD,
    0x00010000,
    version_ms,
    version_ls,
    version_ms,
    version_ls,
    0x3F,
    0,
    0x40004,
    1,
    0,
    0,
    0,
)
table = node(
    "040904B0",
    children=(
        node("ProductName", "PalSchema", 1),
        node("ProductVersion", "0.6.1.0", 1),
    ),
)
resource = node(
    "VS_VERSION_INFO",
    fixed,
    children=(node("StringFileInfo", children=(table,)),),
)
groups = [
    resource[index:index + 4].hex().upper()
    for index in range(0, len(resource), 4)
]

print("""\
Format: COFF-x86-64
ImageFileHeader {
  Characteristics [ (0x2022)
    IMAGE_FILE_DLL
  ]
}
ImageOptionalHeader {
  Characteristics [ (0x8160)
    IMAGE_DLL_CHARACTERISTICS_DYNAMIC_BASE
    IMAGE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA
    IMAGE_DLL_CHARACTERISTICS_NX_COMPAT
  ]
}
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
Data (""")
for index in range(0, len(groups), 4):
    print(f"  {index * 4:04X}: {' '.join(groups[index:index + 4])} |data|")
print(")")
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

nested_link_backup="$backup_root/20000101T000000Z-5"
external_mods="$test_root/external-mods"
mkdir -p "$nested_link_backup/PalSchema/dlls" "$external_mods"
printf '%s\n' "regular-backup" \
    > "$nested_link_backup/PalSchema/dlls/main.dll"
printf '%s\n' "external-mod-sentinel" > "$external_mods/sentinel.txt"
ln -s "$external_mods" "$nested_link_backup/PalSchema/mods"
set +e
PATH="$fake_bin:$PATH" \
    "$project_root/scripts/deploy-proton.sh" \
        --rollback "$nested_link_backup" \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/nested-link.out" 2>"$test_root/nested-link.err"
nested_link_status=$?
set -e
if ((nested_link_status == 0)) ||
   ! grep -q "Backup tree contains a symlink" "$test_root/nested-link.err"; then
    printf '%s\n' "Rollback accepted a nested backup symlink." >&2
    exit 1
fi
if [[ "$(cat "$external_mods/sentinel.txt")" != "external-mod-sentinel" ]]; then
    printf '%s\n' "Rejected nested symlink rollback modified external data." >&2
    exit 1
fi

# Starting the server from a marker-rename wrapper reproduces the old
# check-then-mv race. The atomic swap plus post-swap scan must restore the live
# installation and fail closed.
real_mv="$(command -v mv)"
cat > "$fake_bin/mv" <<EOF
#!/usr/bin/env bash
set -euo pipefail
if [[ "\$*" == *".active-transaction.tmp."* &&
      ! -f "$test_root/race-started" ]]; then
    touch "$test_root/race-started"
    bash -c 'exec -a PalServer-Win64-Shipping-Cmd.exe sleep 30' &
    printf '%s\n' "\$!" > "$test_root/race-process.pid"
fi
exec "$real_mv" "\$@"
EOF
chmod +x "$fake_bin/mv"
pre_race_hash="$(sha256sum "$target_root/dlls/main.dll" | awk '{print $1}')"
set +e
PATH="$fake_bin:$PATH" \
    "$project_root/scripts/deploy-proton.sh" shipping \
        --target server \
        --game-dir "$game_root" \
        >"$test_root/race.out" 2>"$test_root/race.err"
race_status=$?
set -e
if [[ -f "$test_root/race-process.pid" ]]; then
    kill "$(cat "$test_root/race-process.pid")" >/dev/null 2>&1 || true
fi
if ((race_status == 0)) ||
   ! grep -q "started during the atomic deployment" "$test_root/race.err"; then
    printf '%s\n' "Process-start race was not detected after the atomic swap." >&2
    exit 1
fi
if [[ "$(sha256sum "$target_root/dlls/main.dll" | awk '{print $1}')" \
      != "$pre_race_hash" ]]; then
    printf '%s\n' "Process-start race did not restore the previous live mod." >&2
    exit 1
fi

printf '%s\n' "PalSchema deploy transaction tests passed."
