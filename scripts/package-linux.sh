#!/usr/bin/env bash

set -euo pipefail

show_usage() {
    printf '%s\n' \
        "Usage: scripts/package-linux.sh [dev|shipping] [--output-dir PATH]" \
        "" \
        "Packages a Linux-built Win64 DLL in the standard UE4SS mod layout."
}

build_flavor="${1:-shipping}"
if (($# > 0)); then
    shift
fi

output_dir=""
while (($# > 0)); do
    case "$1" in
        --output-dir)
            if (($# < 2)); then
                printf '%s\n' "--output-dir requires a path." >&2
                exit 2
            fi
            output_dir="$2"
            shift
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
        archive_suffix="_Dev"
        ;;
    shipping)
        preset="win64-xwin-shipping"
        archive_suffix=""
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
source "$script_dir/lib/build-env.sh"
artifact="$project_root/build/$preset/PalSchema.dll"
pdb="$project_root/build/$preset/PalSchema.pdb"

if [[ ! -f "$artifact" ]]; then
    printf 'Missing build artifact: %s\n' "$artifact" >&2
    printf 'Build it first with: scripts/build-linux.sh %s\n' "$build_flavor" >&2
    exit 1
fi

required_commands=(install mktemp python3 zip)
if [[ "$build_flavor" == "dev" ]]; then
    required_commands+=(node)
fi
for required_command in "${required_commands[@]}"; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        printf 'Missing required packaging command: %s\n' "$required_command" >&2
        exit 1
    fi
done

llvm_readobj="$(palschema_find_llvm_tool llvm-readobj)"
python3 "$script_dir/verify-win64-artifact.py" "$artifact" \
    --llvm-readobj "$llvm_readobj" >/dev/null

version="$(
    awk '
        /VERSION_MAJOR[[:space:]]+[0-9]+/ { major = $3 }
        /VERSION_MINOR[[:space:]]+[0-9]+/ { minor = $3 }
        /VERSION_REVISION[[:space:]]+[0-9]+/ { revision = $3 }
        END { printf "%s.%s.%s", major, minor, revision }
    ' "$project_root/version.h"
)"

if [[ -z "$output_dir" ]]; then
    output_dir="$project_root/dist"
fi
mkdir -p "$output_dir"
output_dir="$(cd -- "$output_dir" && pwd)"

package_dir="$(mktemp -d "${TMPDIR:-/tmp}/palschema-package.XXXXXX")"
trap 'rm -rf -- "$package_dir"' EXIT

mod_root="$package_dir/PalSchema"
install -d "$mod_root/dlls" "$mod_root/mods"
install -m 0644 "$artifact" "$mod_root/dlls/main.dll"
touch "$mod_root/enabled.txt"

if [[ "$build_flavor" == dev ]]; then
    if [[ ! -f "$pdb" ]]; then
        printf 'Missing Dev debug symbols: %s\n' "$pdb" >&2
        exit 1
    fi

    install -m 0644 "$pdb" "$mod_root/dlls/main.pdb"
    cp -a "$project_root/assets/.vscode" "$mod_root/.vscode"
    cp -a "$project_root/assets/examples" "$mod_root/examples"
    node "$script_dir/copy-public-schemas.mjs" \
        "$project_root/assets/schemas" "$mod_root/schemas"
fi

python3 "$script_dir/verify-win64-artifact.py" \
    "$mod_root/dlls/main.dll" --llvm-readobj "$llvm_readobj" >/dev/null

archive_name="PalSchema_${version}_Win64${archive_suffix}.zip"
temporary_archive="$package_dir/$archive_name"
(
    cd "$package_dir"
    zip -q -r "$temporary_archive" PalSchema
)
install -m 0644 "$temporary_archive" "$output_dir/$archive_name"

printf 'Packaged %s\n' "$output_dir/$archive_name"
