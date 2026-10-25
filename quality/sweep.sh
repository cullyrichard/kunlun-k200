#!/usr/bin/env bash
# Measure the accuracy of each K200 activation-quantization choice on CPU.
# Protocol is examples/kunlun/QUALITY.md: 8192 pinned IDs, 6 threads,
# batch/ubatch 8, flash attention off, state cleared between blocks.
#
# Modes, as documented in ggml-cpu.c: pb1 is the CPU reference granularity and
# must come out at nmse 0; pr1 is the common-Q4 GEMM path, which is what this
# batched protocol would hit on device; pt2 is the batch-1 GEMV path, i.e.
# decode; pt1 is pt2 with the residual component dropped.
set -uo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
. "$root/env.sh"
bin="$root/build/emu/bin/kunlun-quality-eval"
model="${MODEL:-$root/models/llama-2-7b.Q4_0.gguf}"
ids="${IDS:-$root/quality/input.ids}"
out="${OUT:-$root/quality}"

for ctx in "$@"; do
  dir="$out/ctx$ctx"
  mkdir -p "$dir"
  echo "=== context $ctx baseline (CPU q4_0 reference) ==="
  KUNLUN_EMU=off "$bin" "$model" cpu "$ids" "$ctx" "$dir/cpu.bin" \
    2>"$dir/baseline.err" | tee "$dir/baseline.txt"
  for mode in pb1 pr1 pt2 pt1; do
    echo "=== context $ctx mode $mode ==="
    KUNLUN_EMU=$mode "$bin" "$model" cpu "$ids" "$ctx" - "$dir/cpu.bin" \
      2>"$dir/$mode.err" | tee "$dir/$mode.txt"
  done
  rm -f "$dir/cpu.bin"
done
