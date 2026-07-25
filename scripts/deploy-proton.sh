#!/usr/bin/env bash

set -euo pipefail

show_usage() {
    printf '%s\n' \
        "Usage:" \
        "  scripts/deploy-proton.sh [dev|shipping] [--game-dir PATH] [--dry-run]" \
        "  scripts/deploy-proton.sh --rollback BACKUP_DIR [--game-dir PATH] [--dry-run]" \
        "" \
        "Deploys only PalSchema into an existing, separately installed UE4SS." \
        "Refuses to run while the Windows Palworld client is active."
}

build_flavor="shipping"
game_dir=""
rollback_dir=""
dry_run=false

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

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd -- "$script_dir/.." && pwd)"

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

palworld_client_is_running() {
    local cmdline
    local argument

    for cmdline in /proc/[0-9]*/cmdline; do
        while IFS= read -r -d '' argument; do
            case "$argument" in
                Palworld-Win64-Shipping.exe|*/Palworld-Win64-Shipping.exe|*\\Palworld-Win64-Shipping.exe)
                    return 0
                    ;;
            esac
        done < "$cmdline" 2>/dev/null || true
    done

    return 1
}

if [[ ! -f "$win64_dir/Palworld-Win64-Shipping.exe" ]]; then
    printf 'Not a Palworld client installation: %s\n' "$game_dir" >&2
    exit 1
fi
if [[ ! -f "$ue4ss_root/UE4SS.dll" || ! -d "$mods_dir" ]]; then
    printf 'A separate UE4SS installation was not found under: %s\n' "$win64_dir" >&2
    exit 1
fi
if palworld_client_is_running; then
    printf '%s\n' "Refusing to deploy while the Palworld Windows client is active." >&2
    exit 1
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

    if [[ ! -d "$rollback_dir_real/PalSchema" &&
          ! -f "$rollback_dir_real/no-previous-install" ]]; then
        printf 'Not a valid PalSchema deployment backup: %s\n' "$rollback_dir_real" >&2
        exit 1
    fi

    if [[ "$dry_run" == true ]]; then
        printf 'Would roll back %s using %s\n' "$target_dir" "$rollback_dir_real"
        exit 0
    fi

    rollback_id="rollback-$(date -u +%Y%m%dT%H%M%SZ)-$$"
    rollback_safety="$backup_root/$rollback_id"
    mkdir -p "$rollback_safety"
    if [[ -d "$target_dir" ]]; then
        mv -- "$target_dir" "$rollback_safety/PalSchema"
    else
        touch "$rollback_safety/no-previous-install"
    fi

    if [[ -d "$rollback_dir_real/PalSchema" ]]; then
        cp -a -- "$rollback_dir_real/PalSchema" "$target_dir"
    fi

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

if [[ "$dry_run" == true ]]; then
    printf 'Would deploy %s to %s/dlls/main.dll\n' "$artifact" "$target_dir"
    if [[ -d "$target_dir" ]]; then
        printf 'Would preserve the existing mod below %s\n' "$backup_root"
    else
        printf '%s\n' "No existing PalSchema installation would be replaced."
    fi
    exit 0
fi

mkdir -p "$backup_root"
deploy_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
backup_dir="$backup_root/$deploy_id"
mkdir -p "$backup_dir"

stage_dir="$(mktemp -d "$mods_dir/.PalSchema.stage.XXXXXX")"
cleanup_stage() {
    if [[ -n "${stage_dir:-}" && -d "$stage_dir" ]]; then
        rm -rf -- "$stage_dir"
    fi
}
trap cleanup_stage EXIT

install -d "$stage_dir/dlls" "$stage_dir/mods"
install -m 0644 "$artifact" "$stage_dir/dlls/main.dll"
touch "$stage_dir/enabled.txt"
if [[ "$build_flavor" == dev ]]; then
    install -m 0644 "$project_root/build/$preset/PalSchema.pdb" \
        "$stage_dir/dlls/main.pdb"
    cp -a "$project_root/assets/.vscode" "$stage_dir/.vscode"
    cp -a "$project_root/assets/examples" "$stage_dir/examples"
    cp -a "$project_root/assets/schemas" "$stage_dir/schemas"
fi

if [[ -d "$target_dir" ]]; then
    mv -- "$target_dir" "$backup_dir/PalSchema"
else
    touch "$backup_dir/no-previous-install"
fi

if ! mv -- "$stage_dir" "$target_dir"; then
    if [[ -d "$backup_dir/PalSchema" && ! -e "$target_dir" ]]; then
        mv -- "$backup_dir/PalSchema" "$target_dir"
    fi
    printf '%s\n' "Deployment failed; the previous PalSchema installation was restored." >&2
    exit 1
fi
stage_dir=""

{
    printf 'flavor=%s\n' "$build_flavor"
    printf 'artifact=%s\n' "$artifact"
    printf 'sha256=%s\n' "$(sha256sum "$artifact" | awk '{print $1}')"
    printf 'deployed_at_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$backup_dir/deployment.txt"

printf 'Deployed PalSchema to %s\n' "$target_dir"
printf 'Rollback with: scripts/deploy-proton.sh --rollback %s --game-dir %s\n' \
    "$backup_dir" "$game_dir"
