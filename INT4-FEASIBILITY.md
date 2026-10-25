# INT4 on the K200: feasible, and pointless as a speed target

Evaluated 2026-10-24 against primary sources in `upstream/kunlunK200_lib`
(v0.4, `d98643c`). No hardware here; every number below is the fork author's
measurement on a real K200, read out of the tree rather than re-run.

**Verdict: INT4 works, is not faster than INT8, and nothing in the inference
loop is MAC-bound anyway. Keep W4 for capacity, drop it as a performance
target.**

## What "INT4 is not feasible" actually refers to

Two different things get conflated.

- `xdnn::fc` does not implement INT4. It returns `NOT_IMPLEMENT`, alongside
  BF16 and TF32 (`docs/programming.md:18`, `docs/glossary.md:27`). There is no
  vendor GEMM to call.
- The device instruction does run. `bench/kernels/cdnn_mac_int4_pipe.xpu`
  issues `mm_int4` in `MAC_INT8_D4W4` mode, scheduled like the 20210701
  `fc_int8` CDNN images, driven by `bench/k200_int4_bench.cpp`, gold-checked
  against a CPU reference. It measures 220.2 TOPS at 900 MHz rising to 300.9 at
  1250 MHz (`docs/performance.md:15`, `:36`).

So INT4 is implementable by hand. That is the whole of the good news.

## There is no INT4 datapath

The hardware has INT8 and INT16 MAC datapaths and nothing below them:

> There is no FP32 multiplier and no INT4 datapath that runs faster than INT8.
> — `docs/glossary.md:47`

> A 2×INT8 guess (461 at 900 MHz) is false: packed INT4 issues at the INT8
> rate. — `docs/performance.md:5`

> `mm_int4` runs, but it issues at the INT8 rate. It is not twice INT8.
> — `docs/glossary.md:65`

The measurements agree. Matched at 900 MHz, on the owned pipes:

| Pipe | Shape | TOPS |
|---|---|---:|
| INT4 `cdnn_mac_int4_pipe` | 4057088×512×4096 | 220.2 |
| INT8 pipe2 | 2064384×512×4096 | 212.0 |

A 4% edge, which is pipe scheduling, not precision. The wider gap at 1250 MHz
(300.9 vs 238.5) is not INT4 pulling ahead either: `docs/performance.md:48`
records that the owned INT8 pipe and both FP32 paths "go flat above 1 GHz"
because HBM was never overclocked and the step time stops being the MAC. That
is the INT8 pipe going bandwidth-bound, measured with less data per MAC than
the INT4 pipe carries.

**Correction to earlier notes.** The claim that "INT4 220-300 with no vendor
GEMM = the card's most underused capability" was wrong. INT4 is not above INT8
in any matched comparison; the 220-300 range is just the INT8 rate seen through
a hand-written pipe.

## Nothing in the inference loop is MAC-bound

Derived from the GEMV medians in `docs/native-q4.md:64-73` (Llama 2 7B Q4_0,
B1, 900 MHz), summed over the model: 32 layers × (4 × 4096×4096 + 2 ×
4096×11008 + 11008×4096) plus the 4096×32000 output head.

Using the CDNN medians that the shipped MUL_MAT path takes, per-token matmul is
about **78.9 ms of the measured 89.7 ms** (11.15 tok/s) — **88% of the token
budget**. CPU attention, PCIe, KV cache and everything else together are the
remaining ~12%.

**Correction to earlier notes.** The generation gap is *not* "attention-on-CPU
crossing PCIe". Moving attention on-device is worth about 12%, not 6×.

And the MAC is idle even in prefill, where the batching is best: 34.67 tok/s
prompt on 7B is 2 × 6.74e9 × 34.67 ≈ 0.47 TOPS, or **0.22% of the 212 TOPS
INT8 peak**. The MAC was never the constraint, so a faster MAC precision could
not have helped.

## The limiter is GEMV decode efficiency, not precision

Effective bandwidth against roughly 256 GB/s per die, from the same medians at
4096×4096:

| Path | Weight bytes | Median ms | Effective GB/s | % of peak |
|---|---:|---:|---:|---:|
| CDNN INT8 | 16.8 MB | 0.232 | 72.3 | 28% |
| Native W4 | 9.44 MB | 0.338 | 27.9 | 11% |

The W4 path reads **1.78× fewer bytes and is 1.46× slower**. That is decisive:
if precision were the lever, the path that moves less data would win. The
limiter is how efficiently the kernel decodes and feeds, plus launch overhead
(native is 5 launches / 3 waits per GEMV).

Headroom, in order of size:

- W4 GEMV merely reaching the INT8 path's *already poor* 72.3 GB/s → ~18 tok/s
  (1.6×).
- The actual roofline, 3.79 GB at 256 GB/s → ~67 tok/s (6×).

All of it is bandwidth and launch cost. None of it is precision.

## What INT4 is genuinely for: capacity

`PERFORMANCE.md:33`, on the shipped rowwise INT8 weight storage: it "improves
reuse and does not solve compressed-weight capacity." That is the real
constraint, and 4-bit weights are the real answer to it. 13B at the native
path's 4.5 bpw is 7.3 GB and fits one 8 GB die; the same weights at INT8's
~8 bpw are 13.2 GB and do not.

This is already shipped — native Q4 (v0.3) and common-F Q4 weight handles
(v0.4) — so the capacity win is banked, not pending.

## Why a faster INT4 GEMM is not the next step

`mm_int4` is **D4W4**: 4-bit activations as well as weights. Production is
**W4A8**, primary plus residual, and even that carries a measurable cost — a
128-token batch-32 prefill sits at ~0.0287 raw-logit NMSE with two FCs
(`PERFORMANCE.md:47`). Dropping activations to 4 bits would need
rotation-based quantization to stay accurate, is a research project, and at the
end of it the kernel still issues at the INT8 rate.

## Recommendation

1. Drop INT4 as a performance target. Keep W4 for capacity.
2. Spend performance effort on making the W4 GEMV reach the INT8 path's
   effective bandwidth (~1.6×), then chase the roofline (~6×): fewer launches,
   better decode, keep weights resident.
3. Attention on device is a real but modest ~12%. Worth doing, not a
   breakthrough.
