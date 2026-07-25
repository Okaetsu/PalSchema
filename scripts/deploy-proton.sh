#!/usr/bin/env bash

set -euo pipefail

show_usage() {
    printf '%s\n' \
        "Usage:" \
        "  scripts/deploy-proton.sh [dev|shipping] [--target auto|client|server]" \
        "      [--game-dir PATH] [--dry-run]" \
        "  scripts/deploy-proton.sh --rollback BACKUP_DIR [--target auto|client|server]" \
        "      [--game-dir PATH] [--dry-run]" \
        "" \
        "Deploys only PalSchema into an existing, separately installed UE4SS." \
        "Refuses to run unless the selected Windows Palworld target can be" \
        "confirmed stopped through a complete /proc command-line scan."
}

build_flavor="shipping"
game_dir=""
rollback_dir=""
dry_run=false
target_kind="auto"

if (($# > 0)) && [[ "$1" != --* ]]; then
    build_flavor="$1"
    shift
fi

while (($# > 0)); do
    case "$1" in
        --game-dir)
            if (($# < 2)); then
                printf '%s\n' "--game-dir requires a path." >&2
                exit 2
            fi
            game_dir="$2"
            shift
            ;;
        --rollback)
            if (($# < 2)); then
                printf '%s\n' "--rollback requires a backup directory." >&2
                exit 2
            fi
            rollback_dir="$2"
            shift
            ;;
        --target)
            if (($# < 2)); then
                printf '%s\n' "--target requires auto, client, or server." >&2
                exit 2
            fi
            target_kind="$2"
            shift
            ;;
        --dry-run)
            dry_run=true
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
    *)
        printf 'Unknown build flavor: %s\n\n' "$build_flavor" >&2
        show_usage >&2
        exit 2
        ;;
esac

case "$target_kind" in
    auto|client|server)
        ;;
    *)
        printf 'Unknown target: %s\n\n' "$target_kind" >&2
        show_usage >&2
        exit 2
        ;;
esac

required_commands=(
    awk
    cp
    flock
    install
    mktemp
    mv
    python3
    realpath
    rm
    sha256sum
)
if [[ "$build_flavor" == "dev" ]]; then
    required_commands+=(node)
fi
for required_command in "${required_commands[@]}"; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        printf 'Missing required deployment command: %s\n' "$required_command" >&2
        exit 1
    fi
done

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd -- "$script_dir/.." && pwd)"
source "$script_dir/lib/build-env.sh"
source "$script_dir/lib/process-scan.sh"

if [[ -z "$game_dir" ]]; then
    default_game_dir="$HOME/.local/share/Steam/steamapps/common/Palworld"
    if [[ -d "$default_game_dir" ]]; then
        game_dir="$default_game_dir"
    else
        printf '%s\n' \
            "Palworld was not found in Steam's default library." \
            "Pass its installation directory with --game-dir." >&2
        exit 1
    fi
fi

game_dir="$(realpath -e -- "$game_dir")"
win64_dir="$game_dir/Pal/Binaries/Win64"
ue4ss_root="$win64_dir/ue4ss"
mods_dir="$ue4ss_root/Mods"
target_dir="$mods_dir/PalSchema"
backup_root="$ue4ss_root/.palschema-backups"
transaction_marker="$backup_root/.active-transaction"
stage_dir=""
transaction_active=false

if [[ "$target_kind" == "auto" ]]; then
    if [[ -f "$win64_dir/Palworld-Win64-Shipping.exe" ]]; then
        target_kind="client"
    elif [[ -f "$win64_dir/PalServer-Win64-Shipping.exe" ||
            -f "$win64_dir/PalServer-Win64-Shipping-Cmd.exe" ]]; then
        target_kind="server"
    else
        printf 'Not a Palworld Win64 client or server installation: %s\n' "$game_dir" >&2
        exit 1
    fi
fi

if [[ "$target_kind" == "client" &&
      ! -f "$win64_dir/Palworld-Win64-Shipping.exe" ]]; then
    printf 'Not a Palworld Win64 client installation: %s\n' "$game_dir" >&2
    exit 1
elif [[ "$target_kind" == "server" &&
        ! -f "$win64_dir/PalServer-Win64-Shipping.exe" &&
        ! -f "$win64_dir/PalServer-Win64-Shipping-Cmd.exe" ]]; then
    printf 'Not a Palworld Win64 dedicated-server installation: %s\n' "$game_dir" >&2
    exit 1
fi
if [[ ! -f "$ue4ss_root/UE4SS.dll" || ! -d "$mods_dir" ]]; then
    printf 'A separate UE4SS installation was not found under: %s\n' "$win64_dir" >&2
    exit 1
fi
if [[ -L "$target_dir" || -L "$backup_root" ]]; then
    printf '%s\n' "Refusing to deploy through a symlinked PalSchema target or backup root." >&2
    exit 1
fi

ue4ss_root_real="$(realpath -e -- "$ue4ss_root")"
mods_dir_real="$(realpath -e -- "$mods_dir")"
case "$mods_dir_real/" in
    "$ue4ss_root_real"/*/)
        ;;
    *)
        printf 'UE4SS Mods directory escapes its installation: %s\n' "$mods_dir_real" >&2
        exit 1
        ;;
esac
mods_dir="$mods_dir_real"
target_dir="$mods_dir/PalSchema"
backup_root="$ue4ss_root_real/.palschema-backups"
transaction_marker="$backup_root/.active-transaction"

assert_target_stopped() {
    if palschema_target_process_status "$target_kind" /proc; then
        printf 'Refusing to deploy while the Palworld Windows %s is active.\n' \
            "$target_kind" >&2
        exit 1
    else
        process_status=$?
        if ((process_status == 2)); then
            printf '%s\n' \
                "Refusing to deploy because one or more stable /proc process command lines are unreadable." \
                "Run as an account with complete process visibility after stopping the selected Win64 target." >&2
            exit 1
        fi
    fi
}

assert_target_stopped

validate_backup_id() {
    [[ "$1" =~ ^(rollback-)?[0-9]{8}T[0-9]{6}Z-[0-9]+$ ]]
}

validate_stage_name() {
    [[ -z "$1" || "$1" =~ ^\.PalSchema\.stage\.[A-Za-z0-9]+$ ]]
}

recover_incomplete_transaction() {
    local marker_lines=()
    local backup_id
    local stage_name
    local recovery_backup
    local stale_stage

    if [[ ! -f "$transaction_marker" ]]; then
        transaction_active=false
        return 0
    fi
    mapfile -t marker_lines < "$transaction_marker"
    backup_id="${marker_lines[0]:-}"
    stage_name="${marker_lines[1]:-}"
    if ! validate_backup_id "$backup_id" || ! validate_stage_name "$stage_name"; then
        printf 'Invalid deployment transaction marker: %s\n' "$transaction_marker" >&2
        return 1
    fi
    recovery_backup="$backup_root/$backup_id"
    if [[ -L "$recovery_backup" || -L "$recovery_backup/PalSchema" ]]; then
        printf 'Deployment transaction backup contains a symlink: %s\n' \
            "$recovery_backup" >&2
        return 1
    fi
    if [[ ! -d "$recovery_backup" ]]; then
        printf 'Deployment transaction backup is missing: %s\n' "$recovery_backup" >&2
        return 1
    fi
    recovery_backup="$(realpath -e -- "$recovery_backup")"
    case "$recovery_backup/" in
        "$backup_root"/*/)
            ;;
        *)
            printf 'Deployment transaction backup escapes its root: %s\n' \
                "$recovery_backup" >&2
            return 1
            ;;
    esac

    if [[ ! -e "$target_dir" && -d "$recovery_backup/PalSchema" ]]; then
        mv -- "$recovery_backup/PalSchema" "$target_dir"
        printf 'Recovered interrupted PalSchema transaction from %s\n' \
            "$recovery_backup"
    elif [[ ! -e "$target_dir" &&
            ! -f "$recovery_backup/no-previous-install" ]]; then
        printf '%s\n' "Interrupted transaction has neither a live mod nor a recoverable backup." >&2
        return 1
    fi

    if [[ -n "$stage_name" ]]; then
        stale_stage="$mods_dir/$stage_name"
        if [[ -d "$stale_stage" && ! -L "$stale_stage" ]]; then
            rm -rf -- "$stale_stage"
        fi
    fi
    rm -f -- "$transaction_marker"
    transaction_active=false
}

begin_transaction() {
    local backup_id="$1"
    local stage_name="$2"
    local temporary_marker="$backup_root/.active-transaction.tmp.$$"

    validate_backup_id "$backup_id"
    validate_stage_name "$stage_name"
    {
        printf '%s\n' "$backup_id"
        printf '%s\n' "$stage_name"
    } > "$temporary_marker"
    mv -- "$temporary_marker" "$transaction_marker"
    transaction_active=true
}

commit_transaction() {
    rm -f -- "$transaction_marker"
    transaction_active=false
}

cleanup() {
    local status=$?
    if [[ "$transaction_active" == true ]]; then
        recover_incomplete_transaction || true
    fi
    if [[ -n "$stage_dir" && -d "$stage_dir" && ! -L "$stage_dir" ]]; then
        rm -rf -- "$stage_dir"
    fi
    return "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if [[ "$dry_run" != true ]]; then
    mkdir -p "$backup_root"
    if [[ -L "$backup_root" ]]; then
        printf 'Refusing to use a symlinked backup root: %s\n' "$backup_root" >&2
        exit 1
    fi
    backup_root="$(realpath -e -- "$backup_root")"
    case "$backup_root/" in
        "$ue4ss_root_real"/*/)
            ;;
        *)
            printf 'PalSchema backup root escapes UE4SS: %s\n' "$backup_root" >&2
            exit 1
            ;;
    esac
    transaction_marker="$backup_root/.active-transaction"
    exec {deployment_lock_fd}> "$backup_root/.deploy.lock"
    if ! flock -n "$deployment_lock_fd"; then
        printf '%s\n' "Another PalSchema deploy or rollback is active." >&2
        exit 1
    fi
    recover_incomplete_transaction
fi

if [[ -n "$rollback_dir" ]]; then
    if [[ ! -d "$backup_root" ]]; then
        printf 'No PalSchema backup root exists at: %s\n' "$backup_root" >&2
        exit 1
    fi

    backup_root_real="$(realpath -e -- "$backup_root")"
    rollback_dir_real="$(realpath -e -- "$rollback_dir")"
    case "$rollback_dir_real/" in
        "$backup_root_real"/*/)
            ;;
        *)
            printf 'Rollback directory must be below: %s\n' "$backup_root_real" >&2
            exit 1
            ;;
    esac
    if [[ -L "$rollback_dir_real/PalSchema" ]]; then
        printf '%s\n' "Refusing to restore a symlinked PalSchema backup." >&2
        exit 1
    fi
    if [[ ! -d "$rollback_dir_real/PalSchema" &&
          ! -f "$rollback_dir_real/no-previous-install" ]]; then
        printf 'Not a valid PalSchema deployment backup: %s\n' "$rollback_dir_real" >&2
        exit 1
    fi

    if [[ "$dry_run" == true ]]; then
        printf 'Would roll back %s using %s\n' "$target_dir" "$rollback_dir_real"
        exit 0
    fi

    if [[ -d "$rollback_dir_real/PalSchema" ]]; then
        if [[ ! -f "$rollback_dir_real/PalSchema/dlls/main.dll" ||
              -L "$rollback_dir_real/PalSchema/dlls/main.dll" ]]; then
            printf 'Rollback backup has no regular dlls/main.dll: %s\n' \
                "$rollback_dir_real" >&2
            exit 1
        fi
        stage_dir="$(mktemp -d "$mods_dir/.PalSchema.stage.XXXXXX")"
        cp -a -- "$rollback_dir_real/PalSchema/." "$stage_dir/"
        if [[ ! -f "$stage_dir/dlls/main.dll" ]]; then
            printf '%s\n' "Rollback staging did not produce dlls/main.dll." >&2
            exit 1
        fi
    fi

    rollback_id="rollback-$(date -u +%Y%m%dT%H%M%SZ)-$$"
    rollback_safety="$backup_root/$rollback_id"
    assert_target_stopped
    mkdir -p "$rollback_safety"
    if [[ -d "$target_dir" ]]; then
        :
    else
        touch "$rollback_safety/no-previous-install"
    fi
    begin_transaction "$rollback_id" \
        "$([[ -n "$stage_dir" ]] && basename "$stage_dir" || true)"
    if [[ -d "$target_dir" ]]; then
        mv -- "$target_dir" "$rollback_safety/PalSchema"
    fi
    if [[ -n "$stage_dir" ]]; then
        mv -- "$stage_dir" "$target_dir"
        stage_dir=""
    fi
    commit_transaction

    printf 'Rolled back PalSchema from %s\n' "$rollback_dir_real"
    printf 'Previous deployed state preserved at %s\n' "$rollback_safety"
    exit 0
fi

artifact="$project_root/build/$preset/PalSchema.dll"
if [[ ! -f "$artifact" ]]; then
    printf 'Missing build artifact: %s\n' "$artifact" >&2
    printf 'Build it first with: scripts/build-linux.sh %s\n' "$build_flavor" >&2
    exit 1
fi
llvm_readobj="$(palschema_find_llvm_tool llvm-readobj)"
python3 "$script_dir/verify-win64-artifact.py" "$artifact" \
    --llvm-readobj "$llvm_readobj" >/dev/null

if [[ "$build_flavor" == "dev" &&
      ! -f "$project_root/build/$preset/PalSchema.pdb" ]]; then
    printf 'Missing Dev debug symbols: %s\n' \
        "$project_root/build/$preset/PalSchema.pdb" >&2
    exit 1
fi

if [[ "$dry_run" == true ]]; then
    printf 'Would deploy %s to %s/dlls/main.dll\n' "$artifact" "$target_dir"
    if [[ -d "$target_dir" ]]; then
        printf 'Would preserve the existing mod below %s\n' "$backup_root"
    else
        printf '%s\n' "No existing PalSchema installation would be replaced."
    fi
    exit 0
fi

deploy_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
backup_dir="$backup_root/$deploy_id"

stage_dir="$(mktemp -d "$mods_dir/.PalSchema.stage.XXXXXX")"
install -d "$stage_dir/dlls" "$stage_dir/mods"
install -m 0644 "$artifact" "$stage_dir/dlls/main.dll"
touch "$stage_dir/enabled.txt"
if [[ "$build_flavor" == "dev" ]]; then
    install -m 0644 "$project_root/build/$preset/PalSchema.pdb" \
        "$stage_dir/dlls/main.pdb"
    cp -a "$project_root/assets/.vscode" "$stage_dir/.vscode"
    cp -a "$project_root/assets/examples" "$stage_dir/examples"
    node "$script_dir/copy-public-schemas.mjs" \
        "$project_root/assets/schemas" "$stage_dir/schemas"
fi

if [[ ! -f "$stage_dir/dlls/main.dll" ]]; then
    printf '%s\n' "Deployment staging did not produce dlls/main.dll." >&2
    exit 1
fi
assert_target_stopped
mkdir -p "$backup_dir"
if [[ ! -d "$target_dir" ]]; then
    touch "$backup_dir/no-previous-install"
fi
begin_transaction "$deploy_id" "$(basename "$stage_dir")"
if [[ -d "$target_dir" ]]; then
    mv -- "$target_dir" "$backup_dir/PalSchema"
fi
if [[ "${PALSCHEMA_TEST_INTERRUPT_AFTER_BACKUP:-0}" == "1" ]]; then
    kill -TERM "$$"
fi
mv -- "$stage_dir" "$target_dir"
stage_dir=""
commit_transaction

{
    printf 'flavor=%s\n' "$build_flavor"
    printf 'target=%s\n' "$target_kind"
    printf 'artifact=%s\n' "$artifact"
    printf 'sha256=%s\n' "$(sha256sum "$artifact" | awk '{print $1}')"
    printf 'deployed_at_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$backup_dir/deployment.txt"

printf 'Deployed PalSchema to %s\n' "$target_dir"
printf 'Rollback with: scripts/deploy-proton.sh --rollback %s --target %s --game-dir %s\n' \
    "$backup_dir" "$target_kind" "$game_dir"
