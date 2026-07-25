#!/usr/bin/env bash

set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$project_root"

required_commands=(bash git node npm python3)
missing_commands=()

for required_command in "${required_commands[@]}"; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        missing_commands+=("$required_command")
    fi
done

if ((${#missing_commands[@]} > 0)); then
    printf 'Missing public-check commands: %s\n' "${missing_commands[*]}" >&2
    exit 1
fi

node_major="$(node --version | sed -E 's/^v([0-9]+).*/\1/')"
if [[ ! "$node_major" =~ ^[0-9]+$ ]] || ((node_major < 20)); then
    printf 'Node.js 20 or newer is required; found %s.\n' "$(node --version)" >&2
    exit 1
fi

printf '%s\n' \
    "PalSchema public source checks" \
    "OS: $(. /etc/os-release 2>/dev/null && printf '%s' "${PRETTY_NAME:-unknown}")" \
    "Kernel: $(uname -srmo)" \
    "Node: $(node --version)" \
    "npm: $(npm --version)" \
    "Python: $(python3 --version 2>&1)"

bash -n \
    scripts/bootstrap-linux.sh \
    scripts/build-linux.sh \
    scripts/cargo-preserve-lock.sh \
    scripts/deploy-proton.sh \
    scripts/package-linux.sh \
    scripts/test-distro-matrix.sh \
    scripts/test-win64-server.sh \
    scripts/tests/test-deploy-transaction.sh \
    scripts/tests/test-bootstrap-cache.sh \
    scripts/tests/test-distro-matrix-output.sh \
    scripts/tests/test-process-scan.sh \
    scripts/tests/test-win64-server-process-guard.sh \
    scripts/ci/run-public-source-checks.sh \
    scripts/lib/build-env.sh \
    scripts/lib/process-scan.sh

python3 -m py_compile scripts/verify-win64-artifact.py
python3 -m unittest discover -s scripts/tests -p 'test_*.py'
node --check scripts/copy-public-schemas.mjs
scripts/tests/test-deploy-transaction.sh
scripts/tests/test-bootstrap-cache.sh
scripts/tests/test-distro-matrix-output.sh
scripts/tests/test-process-scan.sh
scripts/tests/test-win64-server-process-guard.sh
python3 -m json.tool CMakePresets.json >/dev/null
python3 -m json.tool assets/.vscode/settings.json >/dev/null
python3 -m json.tool assets/schemas/schema-index.json >/dev/null

npm ci
npm audit --audit-level=moderate
npm run tools:typecheck
npm run typecheck --workspace palschema-vscode
npm run editor:test
npm run tools:test
npm run tools:build
node tools/palschema-tools/dist/cli.js schemas verify
node tools/palschema-tools/dist/cli.js validate \
    --allow-missing-generated \
    assets/examples
npm run editor:build
npm run editor:package
npm pack \
    --workspace @palschema/tools \
    --pack-destination dist

printf '%s\n' "PalSchema public source checks passed."
