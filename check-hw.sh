#!/usr/bin/env bash
# Everything that needs a K200 present, in bring-up order. Each stage is
# cheaper and more diagnostic than the one after it, so the first failure is
# the informative one. check-no-hw.sh is the complement: what can be checked
# with no card at all. Run this the moment a card is in the slot.
#
# Stages 0 and 1 gate the rest. With no card the script says so and exits
# without pretending to have tested anything.
#
#   MODEL=...            Q4_0 GGUF for the model stages (default llama-2-7b)
#   KUNLUN_LOG_DIR=...   where per-test logs land (default hw-results/<stamp>)
#   K200_HW_QUALITY=1    also run the on-device quality sweep (slow, 7B)
set -uo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
. "$root/env.sh"

prefix="${KUNLUN_PREFIX:-$HOME/.local/kunlun}"
bin="$root/build/llama/bin"
model="${MODEL:-$root/models/llama-2-7b.Q4_0.gguf}"
export KUNLUN_LOG_DIR="${KUNLUN_LOG_DIR:-$root/hw-results/$(date +%Y%m%d-%H%M%S)}"

# verify.sh invokes the thermal guard as a bare name, and env.sh puts only
# XRE's bin on PATH -- the library's own bin is where k200-thermal-guard
# installs. Without this every guarded test dies as "command not found".
export PATH="$prefix/bin:$PATH"

fail=0
stage() { printf '\n== %s ==\n' "$*"; }
ok()    { printf 'PASS  %s\n' "$*"; }
bad()   { printf 'FAIL  %s\n' "$*"; fail=$((fail + 1)); }
note()  { printf 'NOTE  %s\n' "$*"; }

stage "stage 0: preflight (nothing here touches the card)"
if lsmod | grep -q '^kunlun'; then
    ok "kernel module loaded: $(lsmod | grep '^kunlun' | awk '{print $1}' | paste -sd,)"
else
    bad "kunlun.ko not loaded -- build it per README.md, then modprobe kunlun"
fi
# The guard latches a thermal abort into its own KUNLUN_LOG_DIR, but each run
# here gets a fresh timestamped one, so it would never see an older latch.
# Honour the latch across runs instead: an abort means a die hit 80 C, and the
# next thing to do is find out why, not load it again.
latched="$(find "$root/hw-results" -name THERMAL_ABORT 2>/dev/null | head -1)"
if [ -n "$latched" ]; then
    bad "thermal abort latched in a previous run: $latched"
    cat "$latched"
    if [ "${K200_IGNORE_THERMAL_LATCH:-0}" != 1 ]; then
        echo "Refusing to load the card again. Investigate cooling, then either"
        echo "remove that file or re-run with K200_IGNORE_THERMAL_LATCH=1."
        exit 1
    fi
    note "K200_IGNORE_THERMAL_LATCH=1 set; continuing anyway"
else
    ok "no thermal abort latched in $root/hw-results"
fi
command -v k200-thermal-guard >/dev/null && ok "thermal guard on PATH" \
    || bad "k200-thermal-guard not found -- cmake --install the library"
command -v python3 >/dev/null && ok "python3 present (guard and compare-logits need it)" \
    || bad "python3 missing"
[ -f "$model" ] && ok "model $(basename "$model")" || bad "model not found: $model"

stage "stage 1: the card enumerates"
smi="$(xpu_smi -m 2>/dev/null)"
dies="$(printf '%s' "$smi" | awk 'NF>=22' | wc -l)"
if [ "$dies" -eq 0 ]; then
    echo "No K200 visible: xpu_smi -m returned no rows."
    echo "With no card this is the expected result and nothing below can run."
    echo "check-no-hw.sh is what verifies the tree in this state."
    exit 1
fi
mkdir -p "$KUNLUN_LOG_DIR"
ok "$dies die(s) reporting temperature"
printf '%s\n' "$smi"
if [ "$dies" -ne 2 ]; then
    note "thermal-guard.py hardcodes len(rows)!=2 and raises on anything else,"
    note "so every guarded stage below will exit 91 until that check is widened"
    note "(upstream/kunlunK200_lib/tools/thermal-guard.py, sample())."
    note "MULTI-CARD.md tracks this alongside the library's two-device ceiling."
fi
"$bin/llama-cli" --list-devices

stage "stage 2: library device smoke"
k200-thermal-guard --name k200-smoke --seconds 120 -- k200-smoke \
    > "$KUNLUN_LOG_DIR/k200-smoke.txt" 2>&1 \
    && ok "k200-smoke" || { bad "k200-smoke"; tail -20 "$KUNLUN_LOG_DIR/k200-smoke.txt"; }

stage "stage 3: upstream verification suite (contract, ops, exactness, logits)"
# The fork's own suite: backend contract, test-backend-ops, the gemv-bench
# exactness matrix over both dies, then llm-smoke in none/layer/row with
# teacher-forced logits compared against the CPU run. Every test is guarded
# and bounded; it stops at the first failure by design.
LLAMA_BIN_DIR="$bin" MODEL="$model" \
    "$root/upstream/kunlunK200_llama.cpp/examples/kunlun/verify.sh" \
    && ok "verify.sh" || bad "verify.sh (logs in $KUNLUN_LOG_DIR)"

stage "stage 4: multi-die bench"
# Topology, the ordered peer matrix, serial-vs-overlapped GEMV, and the
# N-split GEMV with its gold check. Clamps itself to the library's
# two-device ceiling and names what to widen -- see MULTI-CARD.md.
k200-thermal-guard --name dual-die --seconds 900 -- "$root/dual-die/dual-die-bench" \
    > "$KUNLUN_LOG_DIR/dual-die.txt" 2>&1
case $? in
    0) ok "dual-die bench"; grep -E "card\(s\)|median|speedup|gold check" "$KUNLUN_LOG_DIR/dual-die.txt" ;;
    *) bad "dual-die bench"; tail -20 "$KUNLUN_LOG_DIR/dual-die.txt" ;;
esac

if [ "${K200_HW_QUALITY:-0}" = 1 ]; then
    stage "stage 5: on-device quality against the CPU reference"
    # QUALITY-RESULTS.md measured the activation-quantization modes on CPU.
    # This is the same protocol with the work actually on the die: the CPU
    # run writes the reference logits, each device mode is scored against it.
    ids="${IDS:-$root/quality/input.ids}"
    ctx="${CTX:-256}"
    dir="$KUNLUN_LOG_DIR/quality"
    mkdir -p "$dir"
    "$bin/kunlun-quality-eval" "$model" cpu "$ids" "$ctx" "$dir/cpu.bin" \
        > "$dir/cpu.txt" 2>"$dir/cpu.err" \
        && ok "CPU reference" || bad "CPU reference"
    for mode in none layer row; do
        k200-thermal-guard --name "quality-$mode" --seconds 1800 -- \
            "$bin/kunlun-quality-eval" "$model" "$mode" "$ids" "$ctx" - "$dir/cpu.bin" \
            > "$dir/$mode.txt" 2>&1 \
            && { ok "quality $mode"; grep -iE "nmse|ppl|top1" "$dir/$mode.txt" | tail -3; } \
            || { bad "quality $mode"; tail -10 "$dir/$mode.txt"; }
    done
    rm -f "$dir/cpu.bin"
fi

stage "result"
echo "logs: $KUNLUN_LOG_DIR"
if [ "$fail" -eq 0 ]; then
    echo "all stages passed"
else
    echo "$fail stage(s) failed"
fi
exit $(( fail > 0 ))
