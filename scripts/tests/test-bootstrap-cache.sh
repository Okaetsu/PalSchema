#!/usr/bin/env bash

set -euo pipefail

repository_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/palschema-bootstrap-test.XXXXXX")"
trap 'rm -rf -- "$test_root"' EXIT

project_root="$test_root/project"
fake_bin="$test_root/bin"
mkdir -p \
    "$project_root/scripts/lib" \
    "$project_root/deps/RE-UE4SS/deps/first/Unreal" \
    "$fake_bin" \
    "$test_root/rust-target"
cp -- "$repository_root/scripts/bootstrap-linux.sh" "$project_root/scripts/"
cp -- "$repository_root/scripts/lib/build-env.sh" "$project_root/scripts/lib/"
touch "$project_root/deps/RE-UE4SS/deps/first/Unreal/CMakeLists.txt"

for tool in clang-cl cmake lld-link llvm-mt llvm-rc llvm-ranlib llvm-lib ninja; do
    printf '%s\n' '#!/usr/bin/env sh' 'exit 0' > "$fake_bin/$tool"
    chmod +x "$fake_bin/$tool"
done
cat > "$fake_bin/rustc" <<'EOF'
#!/usr/bin/env sh
if [ "$1" = "--print" ] && [ "$2" = "target-libdir" ]; then
    printf '%s\n' "$FAKE_RUST_LIBDIR"
    exit 0
fi
exit 0
EOF
cat > "$fake_bin/cargo" <<'EOF'
#!/usr/bin/env sh
exit 0
EOF
cat > "$fake_bin/xwin" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
if [[ "${1:-}" == "--version" ]]; then
    printf '%s\n' "xwin 0.9.0"
    exit 0
fi
output=""
while (($# > 0)); do
    if [[ "$1" == "--output" ]]; then
        output="$2"
        shift
    fi
    shift
done
{
    flock 9
    count=0
    if [[ -f "$FAKE_XWIN_COUNTER" ]]; then
        count="$(cat "$FAKE_XWIN_COUNTER")"
    fi
    printf '%s\n' "$((count + 1))" > "$FAKE_XWIN_COUNTER"
} 9>"$FAKE_XWIN_COUNTER.lock"
mkdir -p \
    "$output/crt/include" \
    "$output/crt/lib/x86_64" \
    "$output/sdk/include/um" \
    "$output/sdk/lib/ucrt/x86_64" \
    "$output/sdk/lib/um/x86_64"
touch \
    "$output/crt/include/vcruntime.h" \
    "$output/crt/lib/x86_64/msvcrt.lib"
if [[ "${FAKE_XWIN_FAIL:-0}" == "1" ]]; then
    exit 1
fi
touch \
    "$output/sdk/include/um/Windows.h" \
    "$output/sdk/lib/ucrt/x86_64/ucrt.lib" \
    "$output/sdk/lib/um/x86_64/kernel32.Lib"
sleep 0.2
EOF
chmod +x "$fake_bin/rustc" "$fake_bin/cargo" "$fake_bin/xwin"

export PATH="$fake_bin:$PATH"
export FAKE_RUST_LIBDIR="$test_root/rust-target"
export FAKE_XWIN_COUNTER="$test_root/xwin-count"
export PALSCHEMA_CACHE_ROOT="$test_root/cache"
export XWIN_DIR="$PALSCHEMA_CACHE_ROOT/xwin"

cd "$project_root"
"$project_root/scripts/bootstrap-linux.sh" \
    --prepare-sdk --accept-microsoft-license >"$test_root/first.out" &
first_pid=$!
"$project_root/scripts/bootstrap-linux.sh" \
    --prepare-sdk --accept-microsoft-license >"$test_root/second.out" &
second_pid=$!
wait "$first_pid"
wait "$second_pid"

if [[ "$(cat "$FAKE_XWIN_COUNTER")" != "1" ]]; then
    printf '%s\n' "Concurrent bootstrap runs performed more than one xwin splat." >&2
    exit 1
fi
if [[ ! -f "$XWIN_DIR/.palschema-sdk-complete" ]]; then
    printf '%s\n' "Concurrent bootstrap did not publish a completed cache." >&2
    exit 1
fi

export PALSCHEMA_CACHE_ROOT="$test_root/failure-cache"
export XWIN_DIR="$PALSCHEMA_CACHE_ROOT/xwin"
export FAKE_XWIN_COUNTER="$test_root/failure-xwin-count"
set +e
FAKE_XWIN_FAIL=1 "$project_root/scripts/bootstrap-linux.sh" \
    --prepare-sdk --accept-microsoft-license \
    >"$test_root/failure.out" 2>"$test_root/failure.err"
failure_status=$?
set -e
if ((failure_status == 0)); then
    printf '%s\n' "Interrupted xwin splat unexpectedly succeeded." >&2
    exit 1
fi
if [[ -e "$XWIN_DIR" ]]; then
    printf '%s\n' "Interrupted xwin splat published a partial cache." >&2
    exit 1
fi
"$project_root/scripts/bootstrap-linux.sh" \
    --prepare-sdk --accept-microsoft-license >"$test_root/retry.out"
if [[ ! -f "$XWIN_DIR/.palschema-sdk-complete" ]]; then
    printf '%s\n' "Bootstrap retry did not publish a completed cache." >&2
    exit 1
fi

# A markerless legacy cache must be regenerated even when it contains every
# former representative sentinel.
export PALSCHEMA_CACHE_ROOT="$test_root/legacy-cache"
export XWIN_DIR="$PALSCHEMA_CACHE_ROOT/xwin"
export FAKE_XWIN_COUNTER="$test_root/legacy-xwin-count"
mkdir -p \
    "$XWIN_DIR/crt/include" \
    "$XWIN_DIR/crt/lib/x86_64" \
    "$XWIN_DIR/sdk/include/um" \
    "$XWIN_DIR/sdk/lib/ucrt/x86_64" \
    "$XWIN_DIR/sdk/lib/um/x86_64"
touch \
    "$XWIN_DIR/crt/include/vcruntime.h" \
    "$XWIN_DIR/crt/lib/x86_64/msvcrt.lib" \
    "$XWIN_DIR/sdk/include/um/Windows.h" \
    "$XWIN_DIR/sdk/lib/ucrt/x86_64/ucrt.lib" \
    "$XWIN_DIR/sdk/lib/um/x86_64/kernel32.Lib"
"$project_root/scripts/bootstrap-linux.sh" \
    --prepare-sdk --accept-microsoft-license >"$test_root/legacy.out"
if [[ "$(cat "$FAKE_XWIN_COUNTER")" != "1" ||
      ! -f "$XWIN_DIR/.palschema-sdk-complete" ]]; then
    printf '%s\n' "Markerless legacy cache was incorrectly certified." >&2
    exit 1
fi

# A custom XWIN_DIR outside the PalSchema cache must never replace an
# unrelated existing directory.
export PALSCHEMA_CACHE_ROOT="$test_root/owned-cache"
export XWIN_DIR="$test_root/unowned-sdk"
export FAKE_XWIN_COUNTER="$test_root/unowned-xwin-count"
mkdir -p "$XWIN_DIR"
printf '%s\n' "keep-me" > "$XWIN_DIR/sentinel"
set +e
"$project_root/scripts/bootstrap-linux.sh" \
    --prepare-sdk --accept-microsoft-license \
    >"$test_root/unowned.out" 2>"$test_root/unowned.err"
unowned_status=$?
set -e
if ((unowned_status == 0)) ||
   ! grep -q "Refusing to replace an unowned XWIN_DIR" \
        "$test_root/unowned.err" ||
   [[ "$(cat "$XWIN_DIR/sentinel")" != "keep-me" ]]; then
    printf '%s\n' "Bootstrap did not preserve an unowned XWIN_DIR." >&2
    exit 1
fi
export PALSCHEMA_CACHE_ROOT="$test_root/legacy-cache"
export XWIN_DIR="$PALSCHEMA_CACHE_ROOT/xwin"
export FAKE_XWIN_COUNTER="$test_root/legacy-xwin-count"

# --install-rust-toolchain must select the PalSchema-local rustup even when
# system cargo and rustc are already available.
isolated_cargo="$test_root/isolated-cargo"
isolated_rustup="$test_root/isolated-rustup"
mkdir -p "$isolated_cargo/bin" "$isolated_rustup"
cp -- "$fake_bin/cargo" "$isolated_cargo/bin/cargo"
cp -- "$fake_bin/rustc" "$isolated_cargo/bin/rustc"
cat > "$isolated_cargo/bin/rustup" <<'EOF'
#!/usr/bin/env sh
printf '%s\n' "$*" >> "$FAKE_ISOLATED_RUSTUP_LOG"
exit 0
EOF
chmod +x \
    "$isolated_cargo/bin/cargo" \
    "$isolated_cargo/bin/rustc" \
    "$isolated_cargo/bin/rustup"
export PALSCHEMA_CARGO_HOME="$isolated_cargo"
export PALSCHEMA_RUSTUP_HOME="$isolated_rustup"
export FAKE_ISOLATED_RUSTUP_LOG="$test_root/isolated-rustup.log"
export FAKE_RUST_LIBDIR="$test_root/missing-rust-target"
"$project_root/scripts/bootstrap-linux.sh" \
    --install-rust-toolchain >"$test_root/isolated-rust.out"
if ! grep -Fxq "target add x86_64-pc-windows-msvc" \
    "$FAKE_ISOLATED_RUSTUP_LOG"; then
    printf '%s\n' "Bootstrap modified or used the wrong Rust toolchain." >&2
    exit 1
fi

printf '%s\n' "PalSchema bootstrap cache tests passed."
