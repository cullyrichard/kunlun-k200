# What the K200's activation quantization costs, measured on CPU

Run 2026-10-25 on the Manjaro workstation, no K200 present. The device's
activation quantization is reproduced exactly in the CPU backend
(`patches/llamacpp-kunlun-emu.patch`, `KUNLUN_EMU`) and scored against the
CPU Q4_0 reference, so the quality question is answered before the card is.

**Verdict: decode is free, prefill is not.** The batch-1 GEMV path costs
nothing measurable (−0.02% perplexity at context 1024). The batched GEMM path,
which is what prompt processing runs, costs **+1.4% to +2.6% perplexity and
flips 5-7% of greedy tokens**. The whole difference is one residual pass that
the GEMM path does not do.

## Protocol and provenance

`examples/kunlun/QUALITY.md`: 8192 pinned IDs, 6 threads, batch/ubatch 8,
Flash Attention off, state cleared between independent blocks, no extra BOS at
block boundaries. 8160 targets scored at context 256, 8184 at context 1024.

| | sha256 |
|---|---|
| `models/llama-2-7b.Q4_0.gguf` (3,825,807,040 B) | `78b8f977…dd24d48` |
| `quality/input.ids` | `8e9a6a44…c2d911fd` |
| WikiText-2 raw test text | `aca2f467…066f58fc` |
| source parquet | `5f1bea06…ab246d91` |

Both corpus hashes match the values pinned in `QUALITY.md`, so this is the
intended fixed corpus. These are fixed comparison scores, not a claim to
reproduce anyone's canonical WikiText perplexity protocol.

## The four modes

| mode | activation scale | components | what it is on device |
|---|---|---|---|
| `pb1` | per 32-wide block | 1 | nothing — the CPU reference's own granularity, a control |
| `pr1` | per src1 row | 1 | common-Q4 **GEMM**, `m != 1`: prefill and any batched run |
| `pt2` | whole src1 buffer | 2 (primary + residual) | common-Q4 and native-Q4 **GEMV**, batch 1: decode |
| `pt1` | whole src1 buffer | 1 | ablation: `pt2` with the residual dropped |

Read off the library, not inferred: the GEMV path sets `components=2`
(`src/q4-common.inc:83`, `src/k200.cpp:537`), and the GEMM path sets
`last_components=1` with per-row scales from `kunlun-common-quant-row-fused`
mode 1, "A8 row" (`src/q4-common.inc:124-168`). Rounding is half-away-from-zero
clamped to ±127 in both the emulator and `kernels/native-q4/quant.h:10`.

## Results

Context 256, 8160 scored, baseline perplexity 11.2887:

| mode | logits NMSE | mean KL | max KL | max abs | greedy agree | perplexity | Δ |
|---|---:|---:|---:|---:|---:|---:|---:|
| `pb1` | 0 | 0 | 0 | 0 | 8160/8160 (100%) | 11.2887 | +0.000% |
| `pt2` | 0.000903 | 0.00137 | 0.244 | 9.73 | 7986/8160 (97.87%) | 11.2795 | −0.081% |
| `pr1` | 0.01526 | 0.02236 | 4.236 | 21.25 | 7591/8160 (93.03%) | 11.5820 | +2.598% |
| `pt1` | 0.06165 | 0.07120 | 4.331 | 21.59 | 7228/8160 (88.58%) | 12.0412 | +6.665% |

Context 1024, 8184 scored, baseline perplexity 7.5081:

| mode | logits NMSE | mean KL | max KL | max abs | greedy agree | perplexity | Δ |
|---|---:|---:|---:|---:|---:|---:|---:|
| `pb1` | 0 | 0 | 0 | 0 | 8184/8184 (100%) | 7.5081 | +0.000% |
| `pt2` | 0.000418 | 0.000662 | 0.119 | 2.50 | 8073/8184 (98.64%) | 7.5069 | −0.016% |
| `pr1` | 0.008878 | 0.01278 | 3.457 | 16.30 | 7780/8184 (95.06%) | 7.6140 | +1.410% |
| `pt1` | 0.02626 | 0.03406 | 4.802 | 16.65 | 7560/8184 (92.38%) | 7.7328 | +2.992% |

`pb1` coming out at exactly zero at both contexts is the control working: the
harness, the emulator and the reference agree bit-for-bit when the emulator is
asked to do what the CPU already does.

## What the numbers say

**The residual component is worth far more than scale granularity.** Dropping
it (`pt2` → `pt1`) costs 68× NMSE at context 256 and 63× at 1024. Refining the
scale instead, from one global value to one per row (`pt1` → `pr1`), recovers
only 4.0× and 3.0×. Two coarse components beat one fine component by 17-21×.
That is a strong argument for the design the GEMV path already has, and against
spending effort on finer activation scales.

**Decode is effectively exact.** `pt2` perplexity sits −0.08% / −0.02% from
baseline — below it, i.e. inside the noise of a 4.5-bpw weight format, not a
real improvement. At 97.9-98.6% greedy agreement, generation quality is not
where K200 effort belongs.

**Prefill carries the real cost**, and it is a deliberate trade. The GEMM path
skips the residual because a second pass doubles GEMM work, and prefill is
compute-bound where decode is not. The price is now measured: +1.4% to +2.6%
perplexity and 5-7% of argmax tokens changed. Whether that is worth 2× prefill
compute is a product question, but it should be a decision rather than an
inherited default — the kernels for a residual already exist and the GEMM path
simply does not call them.

**Longer context halves the damage.** Every mode roughly halves its NMSE from
context 256 to 1024. Better-conditioned predictions are more robust to the same
perturbation; it is not evidence that quantization error itself shrinks.

**The tails are heavy even when the mean is tiny.** `pt2` at context 256 has a
mean KL of 0.0014 but a max of 0.244 and a worst single logit off by 9.7. Mean
metrics alone would hide that some positions are badly disturbed.

## Limits of this measurement

- **Activations only.** Weights stay Q4_0 with per-32-block scales, which is
  what the native and common Q4 paths keep on device. The optional
  `GGML_KUNLUN_REQUANT_I8` rowwise-INT8 weight path is not emulated and its
  error is not in these numbers.
- **Not the hardware.** This is the device's arithmetic reproduced on CPU, so
  it captures quantization error exactly and captures nothing about kernel
  bugs, accumulation order inside the MAC, HBM behaviour or launch sequencing.
  It narrows what the card has to prove; it does not stand in for it.
- **Teacher-forced.** Every position is scored against reference history.
  Agreement under teacher forcing does not establish free-running long-form
  quality, as `QUALITY.md` warns.
- **One model, one corpus.** Llama 2 7B Q4_0 on WikiText-2 test.

Reproduce with `quality/sweep.sh 256 1024` after applying
`patches/llamacpp-kunlun-emu.patch` to the fork at `73635ca`. Roughly 4h50m on
six cores.
