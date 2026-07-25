#!/usr/bin/env bash

set -euo pipefail

show_usage() {
    printf '%s\n' \
        "Usage: scripts/test-win64-server.sh --game-dir PATH [options]" \
        "" \
        "Runs repeatable PalSchema startup smoke tests against an isolated" \
        "Win64 Palworld Dedicated Server under Wine." \
        "" \
        "Options:" \
        "  --game-dir PATH       Explicit isolated server root (required)." \
        "  --cycles N            Number of start/stop cycles (default: 3)." \
        "  --game-port PORT      Isolated UDP game port (default: 18211)." \
        "  --query-port PORT     Isolated UDP query port (default: 37015)." \
        "  --startup-timeout S   Seconds allowed for each startup (default: 120)." \
        "  --hot-reload-file P   Touch this safe fixture after each startup." \
        "  --reload-timeout S    Seconds allowed for hot-reload (default: 30)." \
        "  --evidence-dir PATH   Output directory for sanitized logs." \
        "  -h, --help            Show this help."
}

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
game_dir=""
cycles=3
game_port=18211
query_port=37015
startup_timeout=120
reload_timeout=30
hot_reload_file=""
evidence_dir=""

while (($# > 0)); do
    case "$1" in
        --game-dir|--cycles|--game-port|--query-port|--startup-timeout|--hot-reload-file|--reload-timeout|--evidence-dir)
            if (($# < 2)); then
                printf '%s requires a value.\n' "$1" >&2
                exit 2
            fi
            case "$1" in
                --game-dir) game_dir="$2" ;;
                --cycles) cycles="$2" ;;
                --game-port) game_port="$2" ;;
                --query-port) query_port="$2" ;;
                --startup-timeout) startup_timeout="$2" ;;
                --hot-reload-file) hot_reload_file="$2" ;;
                --reload-timeout) reload_timeout="$2" ;;
                --evidence-dir) evidence_dir="$2" ;;
            esac
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

if [[ -z "$game_dir" ]]; then
    printf '%s\n' "--game-dir is required; automatic server discovery is intentionally disabled." >&2
    exit 2
fi

for numeric_value in "$cycles" "$game_port" "$query_port" "$startup_timeout" "$reload_timeout"; do
    if [[ ! "$numeric_value" =~ ^[0-9]+$ ]] || ((numeric_value < 1)); then
        printf 'Expected a positive integer, found: %s\n' "$numeric_value" >&2
        exit 2
    fi
done

if ((game_port > 65535 || query_port > 65535)); then
    printf '%s\n' "Ports must be between 1 and 65535." >&2
    exit 2
fi
if ((game_port == query_port)); then
    printf '%s\n' "Game and query ports must be different." >&2
    exit 2
fi

game_dir="$(cd -- "$game_dir" && pwd)"
win64_dir="$game_dir/Pal/Binaries/Win64"
server_exe="$win64_dir/PalServer-Win64-Shipping-Cmd.exe"
ue4ss_dir="$win64_dir/ue4ss"
ue4ss_log="$ue4ss_dir/UE4SS.log"
palschema_dll="$ue4ss_dir/Mods/PalSchema/dlls/main.dll"
mods_dir="$ue4ss_dir/Mods/PalSchema/mods"
wine_prefix="$game_dir/.compat/pfx"

required_files=(
    "$server_exe"
    "$win64_dir/dwmapi.dll"
    "$ue4ss_dir/UE4SS.dll"
    "$ue4ss_dir/UE4SS-settings.ini"
    "$ue4ss_dir/MemberVariableLayout.ini"
    "$palschema_dll"
)
for required_file in "${required_files[@]}"; do
    if [[ ! -f "$required_file" ]]; then
        printf 'Required isolated-server file is missing: %s\n' "$required_file" >&2
        exit 1
    fi
done

hot_reload_mod_name=""
if [[ -n "$hot_reload_file" ]]; then
    if [[ ! -f "$hot_reload_file" ]]; then
        printf 'Hot-reload fixture is missing: %s\n' "$hot_reload_file" >&2
        exit 1
    fi
    hot_reload_file="$(
        cd -- "$(dirname -- "$hot_reload_file")"
        printf '%s/%s' "$PWD" "$(basename -- "$hot_reload_file")"
    )"
    case "$hot_reload_file" in
        "$mods_dir"/*)
            hot_reload_relative="${hot_reload_file#"$mods_dir"/}"
            hot_reload_mod_name="${hot_reload_relative%%/*}"
            ;;
        *)
            printf 'Hot-reload fixture must be inside %s\n' "$mods_dir" >&2
            exit 1
            ;;
    esac
fi

required_commands=(grep kill setsid ss timeout wine wineserver)
for required_command in "${required_commands[@]}"; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        printf 'Required command is missing: %s\n' "$required_command" >&2
        exit 1
    fi
done

if pgrep -f -- "$server_exe" >/dev/null 2>&1; then
    printf 'The selected Win64 server is already running: %s\n' "$server_exe" >&2
    exit 1
fi

port_is_open() {
    local port="$1"
    ss -H -lun "sport = :$port" | grep -q .
}

for port in "$game_port" "$query_port"; do
    if port_is_open "$port"; then
        printf 'UDP port %s is already in use; choose an isolated port.\n' "$port" >&2
        exit 1
    fi
done

if [[ -z "$evidence_dir" ]]; then
    timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
    evidence_dir="$project_root/build/runtime-evidence/$timestamp"
fi
mkdir -p "$evidence_dir" "$game_dir/.compat"
evidence_dir="$(cd -- "$evidence_dir" && pwd)"

expected_loaders=(
    enums
    raw
    blueprints
    resources
    pals
    npcs
    items
    skins
    appearance
    buildings
    helpguide
    spawns
    translations
)
expected_signature_count=22
launcher_pid=""

stop_selected_server() {
    local wait_iteration

    if [[ -n "$launcher_pid" ]] && kill -0 "$launcher_pid" >/dev/null 2>&1; then
        kill -INT -- "-$launcher_pid" >/dev/null 2>&1 || true
        for wait_iteration in {1..20}; do
            if ! kill -0 "$launcher_pid" >/dev/null 2>&1; then
                break
            fi
            sleep 1
        done
    fi

    if [[ -n "$launcher_pid" ]] && kill -0 "$launcher_pid" >/dev/null 2>&1; then
        kill -TERM -- "-$launcher_pid" >/dev/null 2>&1 || true
        sleep 2
    fi
    if [[ -n "$launcher_pid" ]] && kill -0 "$launcher_pid" >/dev/null 2>&1; then
        kill -KILL -- "-$launcher_pid" >/dev/null 2>&1 || true
    fi

    WINEPREFIX="$wine_prefix" wineserver -k >/dev/null 2>&1 || true
    launcher_pid=""
}
trap stop_selected_server EXIT INT TERM

printf '%s\n' \
    "PalSchema Win64 server smoke suite" \
    "Server: $game_dir" \
    "Cycles: $cycles" \
    "Ports: $game_port/$query_port" \
    "Hot reload: ${hot_reload_file:-not requested}" \
    "Evidence: $evidence_dir"

printf 'cycle\tstartup_seconds\tsignatures\tloaders\tgame_port\tquery_port\thot_reload\n' \
    > "$evidence_dir/summary.tsv"

for ((cycle = 1; cycle <= cycles; cycle++)); do
    cycle_dir="$evidence_dir/cycle-$cycle"
    console_log="$cycle_dir/wine-console.log"
    mkdir -p "$cycle_dir"

    if [[ -f "$ue4ss_log" ]]; then
        mv -- "$ue4ss_log" "$cycle_dir/ue4ss-before-start.log"
    fi

    cycle_started_at="$(date +%s)"
    (
        cd "$win64_dir"
        exec setsid env \
            WINEPREFIX="$wine_prefix" \
            WINEDLLOVERRIDES="dwmapi=n,b" \
            WINEDEBUG="-all" \
            SteamAppId="2394010" \
            wine "./$(basename -- "$server_exe")" \
                Pal \
                "-port=$game_port" \
                "-queryport=$query_port" \
                -players=4 \
                -useperfthreads \
                -NoAsyncLoadingThread \
                -UseMultithreadForDS
    ) >"$console_log" 2>&1 &
    launcher_pid=$!

    cycle_deadline=$((cycle_started_at + startup_timeout))
    cycle_ready=false
    while (( $(date +%s) <= cycle_deadline )); do
        if ! kill -0 "$launcher_pid" >/dev/null 2>&1; then
            printf 'Cycle %s exited before reaching readiness.\n' "$cycle" >&2
            break
        fi

        if [[ -f "$ue4ss_log" ]] &&
            grep -Fq "Event loop start" "$ue4ss_log" &&
            port_is_open "$game_port" &&
            port_is_open "$query_port"; then
            all_loaders_ready=true
            for loader in "${expected_loaders[@]}"; do
                if ! grep -Fq "Loader '$loader' initialized." "$ue4ss_log"; then
                    all_loaders_ready=false
                    break
                fi
            done
            if [[ "$all_loaders_ready" == true ]]; then
                cycle_ready=true
                break
            fi
        fi
        sleep 1
    done

    if [[ -f "$ue4ss_log" ]]; then
        cp -- "$ue4ss_log" "$cycle_dir/UE4SS.log"
    fi

    if [[ "$cycle_ready" != true ]]; then
        printf 'Cycle %s did not become ready within %s seconds.\n' \
            "$cycle" "$startup_timeout" >&2
        exit 1
    fi

    if grep -Eiq '\[PalSchema\].*(\[error\]|\[fatal\])' "$ue4ss_log"; then
        printf 'Cycle %s contains a PalSchema error or fatal log entry.\n' "$cycle" >&2
        exit 1
    fi

    signature_count="$(grep -Fc "[PalSchema] Found " "$ue4ss_log")"
    if ((signature_count != expected_signature_count)); then
        printf 'Cycle %s found %s signatures; expected %s.\n' \
            "$cycle" "$signature_count" "$expected_signature_count" >&2
        exit 1
    fi

    loader_count=0
    for loader in "${expected_loaders[@]}"; do
        if grep -Fq "Loader '$loader' initialized." "$ue4ss_log"; then
            loader_count=$((loader_count + 1))
        fi
    done

    hot_reload_result="not-run"
    if [[ -n "$hot_reload_file" ]]; then
        if ! grep -Fq "Auto-reload is enabled." "$ue4ss_log"; then
            printf '%s\n' \
                "Hot-reload was requested, but PalSchema auto-reload is disabled." >&2
            exit 1
        fi

        reload_marker="Auto-reloaded mod $hot_reload_mod_name"
        reload_count_before="$(grep -Fc "$reload_marker" "$ue4ss_log" || true)"
        touch -- "$hot_reload_file"
        reload_deadline=$(($(date +%s) + reload_timeout))
        while (( $(date +%s) <= reload_deadline )); do
            reload_count_after="$(grep -Fc "$reload_marker" "$ue4ss_log" || true)"
            if ((reload_count_after > reload_count_before)); then
                hot_reload_result="passed"
                break
            fi
            sleep 1
        done
        if [[ "$hot_reload_result" != "passed" ]]; then
            printf 'Cycle %s did not auto-reload %s within %s seconds.\n' \
                "$cycle" "$hot_reload_mod_name" "$reload_timeout" >&2
            exit 1
        fi
        cp -- "$ue4ss_log" "$cycle_dir/UE4SS.log"
    fi

    startup_seconds=$(($(date +%s) - cycle_started_at))
    printf '%s\t%s\t%s\t%s\topen\topen\t%s\n' \
        "$cycle" \
        "$startup_seconds" \
        "$signature_count" \
        "$loader_count" \
        "$hot_reload_result" \
        >> "$evidence_dir/summary.tsv"

    stop_selected_server

    for port in "$game_port" "$query_port"; do
        if port_is_open "$port"; then
            printf 'Cycle %s left UDP port %s open after shutdown.\n' "$cycle" "$port" >&2
            exit 1
        fi
    done

    printf 'Cycle %s/%s passed in %ss: %s signatures, %s loaders.\n' \
        "$cycle" "$cycles" "$startup_seconds" "$signature_count" "$loader_count"
done

trap - EXIT INT TERM
printf 'All %s cycle(s) passed. Evidence: %s\n' "$cycles" "$evidence_dir"
