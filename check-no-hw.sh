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
echo "== multi-die bench logic, 4 fake dies on 2 fake cards =="
K200_FAKE_DIES=4 K200_LIBRARY_DEVICES=4 "$root/dual-die/dual-die-bench-fake" 2>&1 |
  grep -E "FAKE RUNTIME|device_count|card\(s\)|-card median|gold check|split speedup|^done"
echo "== same bench with BOARDID unreadable, 2 dies =="
K200_FAKE_DIES=2 K200_LIBRARY_DEVICES=2 K200_FAKE_NO_BOARDID=1 \
  "$root/dual-die/dual-die-bench-fake" 2>&1 |
  grep -E "BOARDID unreadable|gold check|^done"
echo "== real bench against the real library: SKIP, exit 0, with no card =="
"$root/dual-die/dual-die-bench" && echo "exit 0"
