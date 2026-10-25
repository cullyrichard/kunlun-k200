#!/usr/bin/env bash
# Everything that can be checked with no K200 present. Each line is evidence
# that the toolchain, the SDK handshake and the CPU paths are intact.
set -uo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
. "$root/env.sh"
model="${MODEL:-$root/models/tinyllama-1.1b-chat-v1.0.Q4_0.gguf}"

echo "== compiled device kernels =="
ls "$root/build/library/lib/kunlunk200/kernels"/*.bin | wc -l
echo "== CPU-only Q4 layout self-check =="
"$root/build/library/k200-common-q4-layout"
echo "== shared library resolution =="
ldd "$root/build/llama/bin/libggml-kunlun.so" | grep -E "kunlunk200|xpuapi|xpurt|not found"
echo "== backend registers, enumerates zero devices, exits clean =="
"$root/build/llama/bin/llama-cli" --list-devices
echo "== CPU end-to-end inference through the fork's harness =="
"$root/build/llama/bin/kunlun-llm-smoke" "$model" cpu 8 /dev/null 2>&1 |
  grep -E "^K200PERF|^TOKENS"
