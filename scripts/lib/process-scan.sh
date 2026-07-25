#!/usr/bin/env bash

# Return 0 when a matching Win64 Palworld process exists, 1 after a complete
# negative scan, and 2 when any stable process command line cannot be read.
palschema_target_process_status() {
    local target_kind="$1"
    local proc_root="$2"
    local cmdline
    local cmdline_fd
    local argument
    local incomplete=false
    local cmdlines=("$proc_root"/[0-9]*/cmdline)

    if [[ ! -d "$proc_root" || ! -r "$proc_root" ||
          "${cmdlines[0]}" == "$proc_root/[0-9]*/cmdline" ]]; then
        return 2
    fi

    for cmdline in "${cmdlines[@]}"; do
        if ! { exec {cmdline_fd}<"$cmdline"; } 2>/dev/null; then
            # A process that vanished between glob expansion and open is benign.
            if [[ -e "$cmdline" ]]; then
                incomplete=true
            fi
            continue
        fi

        while IFS= read -r -d '' argument <&"$cmdline_fd"; do
            if [[ "$target_kind" == "client" ]]; then
                case "$argument" in
                    Palworld-Win64-Shipping.exe|*/Palworld-Win64-Shipping.exe|*\\Palworld-Win64-Shipping.exe)
                        exec {cmdline_fd}<&-
                        return 0
                        ;;
                esac
            else
                case "$argument" in
                    PalServer.exe|*/PalServer.exe|*\\PalServer.exe|\
                    PalServer-Win64-Shipping.exe|*/PalServer-Win64-Shipping.exe|*\\PalServer-Win64-Shipping.exe|\
                    PalServer-Win64-Shipping-Cmd.exe|*/PalServer-Win64-Shipping-Cmd.exe|*\\PalServer-Win64-Shipping-Cmd.exe)
                        exec {cmdline_fd}<&-
                        return 0
                        ;;
                esac
            fi
        done
        exec {cmdline_fd}<&-
    done

    if [[ "$incomplete" == true ]]; then
        return 2
    fi
    return 1
}
