#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/palschema-matrix-output-test.XXXXXX")"
trap 'rm -rf -- "$test_root"' EXIT

fake_bin="$test_root/bin"
mkdir -p "$fake_bin"
cat > "$fake_bin/docker" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
case "${1:-}" in
    run)
        if [[ "${FAKE_DOCKER_SLEEP:-0}" == "1" ]]; then
            sleep 30
        fi
        for index in $(seq 1 5000); do
            printf 'very-noisy-container-line-%s\n' "$index"
        done
        exit "${FAKE_DOCKER_STATUS:-0}"
        ;;
    rm|pull)
        exit 0
        ;;
    *)
        exit 0
        ;;
esac
EOF
chmod +x "$fake_bin/docker"

PATH="$fake_bin:$PATH" \
    "$repository_root/scripts/test-distro-matrix.sh" \
        --no-pull debian >"$test_root/success.out" 2>&1
if grep -q "very-noisy-container-line" "$test_root/success.out"; then
    printf '%s\n' "Successful matrix output leaked container logs." >&2
    exit 1
fi
if ! grep -q "PASS" "$test_root/success.out"; then
    printf '%s\n' "Successful matrix output did not report a concise PASS." >&2
    exit 1
fi

set +e
PATH="$fake_bin:$PATH" \
PALSCHEMA_MATRIX_TIMEOUT_SECONDS=1 \
FAKE_DOCKER_SLEEP=1 \
    "$repository_root/scripts/test-distro-matrix.sh" \
        --no-pull debian >"$test_root/timeout.out" 2>&1
timeout_status=$?
set -e
if ((timeout_status == 0)); then
    printf '%s\n' "Timed-out matrix unexpectedly succeeded." >&2
    exit 1
fi
if ! grep -q "Timed out after 1 seconds" "$test_root/timeout.out"; then
    printf '%s\n' "Timed-out matrix did not report its hard limit." >&2
    exit 1
fi
if (($(wc -c < "$test_root/timeout.out") > 32768)); then
    printf '%s\n' "Timed-out matrix emitted excessive terminal output." >&2
    exit 1
fi

printf '%s\n' "PalSchema distro matrix output tests passed."
