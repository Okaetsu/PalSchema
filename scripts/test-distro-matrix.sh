#!/usr/bin/env bash

set -euo pipefail

show_usage() {
    printf '%s\n' \
        "Usage: scripts/test-distro-matrix.sh [options] [distro ...]" \
        "" \
        "Runs PalSchema's public source checks in clean Linux containers." \
        "" \
        "Distros:" \
        "  debian ubuntu fedora opensuse arch" \
        "" \
        "Options:" \
        "  --list             Print the supported distro/image mapping." \
        "  --no-pull          Use an already available local image." \
        "  --build-shipping   Also cross-build and verify the Win64 Shipping DLL." \
        "  --node-major N     Test with the latest verified Node.js N.x (default: 22)." \
        "  --timeout-minutes N Stop one distro after N minutes (default: 10)." \
        "  -h, --help         Show this help." \
        "" \
        "With no distro arguments, all supported distros are tested." \
        "--build-shipping reuses an already prepared PalSchema cache and never" \
        "downloads or silently accepts the Microsoft SDK license."
}

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
node_major=22
timeout_minutes=10
pull_images=true
build_shipping=false
selected_distros=()

declare -A distro_images=(
    [debian]="debian:stable-slim"
    [ubuntu]="ubuntu:24.04"
    [fedora]="fedora:latest"
    [opensuse]="opensuse/tumbleweed:latest"
    [arch]="archlinux:latest"
)

all_distros=(debian ubuntu fedora opensuse arch)

list_distros() {
    local distro
    for distro in "${all_distros[@]}"; do
        printf '%-10s %s\n' "$distro" "${distro_images[$distro]}"
    done
}

while (($# > 0)); do
    case "$1" in
        --list)
            list_distros
            exit 0
            ;;
        --no-pull)
            pull_images=false
            ;;
        --build-shipping)
            build_shipping=true
            ;;
        --node-major)
            if (($# < 2)); then
                printf '%s\n' "--node-major requires a value." >&2
                exit 2
            fi
            node_major="$2"
            shift
            ;;
        --timeout-minutes)
            if (($# < 2)); then
                printf '%s\n' "--timeout-minutes requires a value." >&2
                exit 2
            fi
            timeout_minutes="$2"
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        -*)
            printf 'Unknown option: %s\n\n' "$1" >&2
            show_usage >&2
            exit 2
            ;;
        *)
            selected_distros+=("$1")
            ;;
    esac
    shift
done

if [[ ! "$node_major" =~ ^[0-9]+$ ]] || ((node_major < 20)); then
    printf '%s\n' "--node-major must be an integer greater than or equal to 20." >&2
    exit 2
fi
if [[ ! "$timeout_minutes" =~ ^[0-9]+$ ]] || ((timeout_minutes < 1)); then
    printf '%s\n' "--timeout-minutes must be a positive integer." >&2
    exit 2
fi
matrix_timeout_seconds="$((timeout_minutes * 60))"
if [[ -n "${PALSCHEMA_MATRIX_TIMEOUT_SECONDS:-}" ]]; then
    if [[ ! "$PALSCHEMA_MATRIX_TIMEOUT_SECONDS" =~ ^[0-9]+$ ]] ||
       ((PALSCHEMA_MATRIX_TIMEOUT_SECONDS < 1)); then
        printf '%s\n' "PALSCHEMA_MATRIX_TIMEOUT_SECONDS must be positive." >&2
        exit 2
    fi
    matrix_timeout_seconds="$PALSCHEMA_MATRIX_TIMEOUT_SECONDS"
fi

if ((${#selected_distros[@]} == 0)); then
    selected_distros=("${all_distros[@]}")
fi

for required_command in docker tail timeout; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        printf '%s is required to run the distro matrix.\n' \
            "$required_command" >&2
        exit 1
    fi
done

if [[ -n "${PALSCHEMA_CACHE_ROOT:-}" ]]; then
    palschema_cache_root="$PALSCHEMA_CACHE_ROOT"
elif [[ -n "${XDG_CACHE_HOME:-}" ]]; then
    palschema_cache_root="$XDG_CACHE_HOME/palschema"
else
    user_home_dir="${HOME:?HOME must be set when XDG_CACHE_HOME is unset}"
    palschema_cache_root="$user_home_dir/.cache/palschema"
fi

if [[ "$build_shipping" == true ]]; then
    required_cache_paths=(
        "$palschema_cache_root/cargo/bin/rustup"
        "$palschema_cache_root/rustup"
        "$palschema_cache_root/xwin/crt"
        "$palschema_cache_root/xwin/sdk"
    )
    for required_cache_path in "${required_cache_paths[@]}"; do
        if [[ ! -e "$required_cache_path" ]]; then
            printf 'Prepared build cache is missing: %s\n' "$required_cache_path" >&2
            printf '%s\n' \
                "Run scripts/bootstrap-linux.sh with the explicit install/SDK options first." >&2
            exit 1
        fi
    done
fi

for distro in "${selected_distros[@]}"; do
    if [[ -z "${distro_images[$distro]:-}" ]]; then
        printf 'Unsupported distro: %s\n\n' "$distro" >&2
        list_distros >&2
        exit 2
    fi
done

matrix_tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/palschema-distro-matrix.XXXXXX")"
archive_path="$matrix_tmp_dir/source.tar"
active_container=""

cleanup() {
    if [[ -n "$active_container" ]]; then
        docker rm --force "$active_container" >/dev/null 2>&1 || true
    fi
    rm -rf -- "$matrix_tmp_dir"
}
trap cleanup EXIT INT TERM

tar \
    --exclude=.git \
    --exclude=build \
    --exclude=dist \
    --exclude=graphify-out \
    --exclude=node_modules \
    -cf "$archive_path" \
    -C "$project_root" \
    .

container_script='
set -euo pipefail

case "$PALSCHEMA_DISTRO" in
    debian|ubuntu)
        export DEBIAN_FRONTEND=noninteractive
        apt-get update
        apt-get install -y --no-install-recommends \
            bash ca-certificates coreutils curl git gzip python3 tar xz-utils
        if [[ "$PALSCHEMA_BUILD_SHIPPING" == 1 ]]; then
            apt-get install -y --no-install-recommends \
                clang clang-tools cmake lld llvm ninja-build util-linux
        fi
        rm -rf /var/lib/apt/lists/*
        ;;
    fedora)
        dnf install -y \
            bash ca-certificates coreutils curl findutils git gzip python3 tar xz
        if [[ "$PALSCHEMA_BUILD_SHIPPING" == 1 ]]; then
            dnf install -y \
                clang clang-tools-extra cmake lld llvm ninja-build util-linux
        fi
        dnf clean all
        ;;
    opensuse)
        zypper --non-interactive refresh
        zypper --non-interactive install -y \
            bash ca-certificates coreutils curl findutils git gzip python3 tar xz
        if [[ "$PALSCHEMA_BUILD_SHIPPING" == 1 ]]; then
            zypper --non-interactive install -y \
                clang cmake lld llvm ninja util-linux
        fi
        zypper clean --all
        ;;
    arch)
        pacman -Syu --noconfirm --needed \
            bash ca-certificates coreutils curl findutils git gzip python tar xz
        if [[ "$PALSCHEMA_BUILD_SHIPPING" == 1 ]]; then
            pacman -S --noconfirm --needed \
                clang cmake lld llvm ninja util-linux
        fi
        ;;
    *)
        printf "Unsupported container distro: %s\n" "$PALSCHEMA_DISTRO" >&2
        exit 2
        ;;
esac

node_checksums_url="https://nodejs.org/dist/latest-v${PALSCHEMA_NODE_MAJOR}.x/SHASUMS256.txt"
curl --fail --location --silent --show-error \
    --output /tmp/node-shasums.txt \
    "$node_checksums_url"
node_archive_line="$(
    grep -E " node-v${PALSCHEMA_NODE_MAJOR}[^ ]*-linux-x64.tar.xz\$" \
        /tmp/node-shasums.txt |
        head -n 1
)"
node_archive="${node_archive_line##* }"
if [[ -z "$node_archive" ]]; then
    printf "Could not resolve a Node.js %s.x Linux x64 archive.\n" "$PALSCHEMA_NODE_MAJOR" >&2
    exit 1
fi
node_version="${node_archive#node-}"
node_version="${node_version%-linux-x64.tar.xz}"
curl --fail --location --silent --show-error \
    --output "/tmp/$node_archive" \
    "https://nodejs.org/dist/$node_version/$node_archive"
(
    cd /tmp
    grep " $node_archive\$" node-shasums.txt | sha256sum --check -
)
tar -xJf "/tmp/$node_archive" -C /usr/local --strip-components=1

mkdir -p /workspace
tar -xf /input/source.tar -C /workspace
cd /workspace
scripts/ci/run-public-source-checks.sh

if [[ "$PALSCHEMA_BUILD_SHIPPING" == 1 ]]; then
    mkdir -p /palschema-cache
    cp -a /palschema-cache-source/. /palschema-cache/
    export PALSCHEMA_CACHE_ROOT=/palschema-cache
    export XWIN_DIR=/palschema-cache/xwin
    scripts/build-linux.sh shipping
    python3 scripts/verify-win64-artifact.py \
        build/win64-xwin-shipping/PalSchema.dll \
        --json-output build/win64-xwin-shipping/pe-contract.json
    sha256sum build/win64-xwin-shipping/PalSchema.dll
fi
'

failures=()
for distro in "${selected_distros[@]}"; do
    image="${distro_images[$distro]}"
    if [[ "$pull_images" == true ]]; then
        docker pull "$image"
    fi

    active_container="palschema-matrix-${distro}-$$"
    distro_log="$matrix_tmp_dir/$distro.log"
    distro_started="$SECONDS"
    printf 'Testing %-10s (%s, timeout %ss)... ' \
        "$distro" "$image" "$matrix_timeout_seconds"
    docker_args=(
        --name "$active_container"
        --rm
        --volume "$matrix_tmp_dir:/input:ro"
        --env "PALSCHEMA_DISTRO=$distro"
        --env "PALSCHEMA_NODE_MAJOR=$node_major"
        --env "PALSCHEMA_BUILD_SHIPPING=$([[ "$build_shipping" == true ]] && printf 1 || printf 0)"
    )
    if [[ "$build_shipping" == true ]]; then
        docker_args+=(
            --volume "$palschema_cache_root:/palschema-cache-source:ro"
        )
    fi
    set +e
    timeout --signal=TERM --kill-after=2s \
        "${matrix_timeout_seconds}s" \
        docker run "${docker_args[@]}" "$image" bash -c "$container_script" \
        2>&1 |
        tail -c 8388608 > "$distro_log"
    run_status="${PIPESTATUS[0]}"
    set -e
    distro_elapsed="$((SECONDS - distro_started))"
    if ((run_status == 0)); then
        printf 'PASS (%ss)\n' "$distro_elapsed"
    else
        printf 'FAIL (%ss, exit %s)\n' "$distro_elapsed" "$run_status"
        if ((run_status == 124)); then
            printf 'Timed out after %s seconds.\n' "$matrix_timeout_seconds" >&2
        fi
        printf '%s\n' "--- last 120 log lines for $distro ---" >&2
        tail -n 120 "$distro_log" >&2
        docker rm --force "$active_container" >/dev/null 2>&1 || true
        failures+=("$distro")
    fi
    active_container=""
done

if ((${#failures[@]} > 0)); then
    printf '\nDistro matrix failures: %s\n' "${failures[*]}" >&2
    exit 1
fi

printf '\nPalSchema distro matrix passed: %s\n' "${selected_distros[*]}"
