# Making two K200s work

Status 2026-10-26: **survey complete; the bench extension is written, the
library and backend changes are not.** Two cards are on order. The blocking
sites below are the full list of what stops four dies, found by reading the
source. Nothing that touches the card can be tested until the cards arrive.

## The decision this rests on

Two cards is four dies, 8 GB each, 32 GB total. The capacity is the reason to
want them; the speed is not, because the W4 GEMV currently runs at 27.9 GB/s
effective against a 256 GB/s die — 11% of one die's own roofline
(`INT4-FEASIBILITY.md`). Four dies multiply a number that is mostly unused.

So the target is **N registered devices with ggml doing layer placement**,
which gives the full 32 GB with no new parallelism code of our own, plus the
existing 2-way tensor split kept *within* each card. N-way tensor parallelism
across cards is deliberately out of scope until the peer matrix says
cross-card copies are cheap (< ~100 us; a token needs ~64 of them).

## Blocking sites

### Library, `upstream/kunlunK200_lib`

| file:line | what |
|---|---|
| `src/k200.cpp:64` | `select()`: `require(id >= 0 && id < 2)` — **the hard stop**, aborts the process |
| `src/k200.cpp:24` | `std::atomic<size_t> allocated[2]` |
| `src/k200.cpp:25` | `std::atomic<size_t> matmuls[2]` |
| `src/k200.cpp:83` | `allocated_bytes()`: same `id < 2` guard |
| `src/k200.cpp:433` | `std::weak_ptr<NativeL3> native_l3[2]` |
| `src/k200.cpp:483` | `native_q4_matmul()`: `require(id >= 0 && id < 2 && ...)` |
| `src/k200.cpp:~551` | `statistics()` returns `{{matmuls[0].load(),matmuls[1].load()},...}` |
| `src/q4-common.inc:18` | `CommonQ4Weight` ctor: `require(id >= 0 && id < 2 && ...)` |
| `src/q4-common.inc:86` | `native_l3[s.id]` — indexes the `[2]` above |
| `src/q4-common.inc:171` | `++matmuls[id]` — the common path's own increment, indexes the `[2]` above |
| `include/kunlun/k200.h:192` | `struct Statistics { size_t matmuls[2]; ... }` — **public ABI change** |

Approach: one `constexpr int max_devices = 8;` in the anonymous namespace,
every `[2]` and `id < 2` keyed to it, real count from `xpu_device_count()`.
`Statistics::matmuls` widens to `[max_devices]` and should gain a `devices`
field rather than leaving callers to guess how many entries are live.

**Not yet surveyed:** `q4-common.inc` past line 60 (`common_q4_matmul`'s own
guard almost certainly has the same `id < 2`), and `q4-common-pack.h`.

### Backend, `upstream/kunlunK200_llama.cpp/ggml/src/ggml-kunlun/ggml-kunlun.cpp`

| line | what |
|---|---|
| 96-98 | `devices[2]`, `normal_types[2]`, `normal_contexts[2]` — registration |
| 66-73 | `tensor_parts`: `ptr[2]`, `bytes[2]`, `physical[2]`, `rows[2]`, `common_weight[2]` |
| 80-94 | `backend_context`: `mv[2]`, `native[2]`, `common[2]`, `i8_mm[2]`, `q4_mm[2]`, `native_logged[2]`, `fallback_logged[2]`, and single `split_x`/`split_y`/`local_y`/`peer_y` |
| 25-28, 48 | statistics arrays `[2]`, `common_m[2]`, `common_phase[2]` |
| 36, 53 | `for (int id=0;id<2;++id)` logging loops |
| ~233 | `repack()`: `for (int id=0;id<2;++id)` row-range split |
| ~250 | `matmul()`: `const int other = 1-main` and the whole 2-way split path |

Only **96-98** is needed for layer placement across four dies. The
`tensor_parts`/`matmul` work is the N-way tensor split and stays deferred; if
the split type survives as-is it should pair dies **on the same board**, i.e.
one split buffer type per card, not a global pair of ids 0 and 1.

**Not yet surveyed:** the file end to end — only lines 1-100 and ~230-300 were
read. Expect more `[2]`s.

### Harness, `upstream/kunlunK200_lib/tools` and `env.sh`

Found by building and dry-running the harnesses on 2026-10-26, not by reading
the library. None of these touch the card, and all three bite the moment two
cards are in the slot.

| file:line | what |
|---|---|
| `kunlunK200_lib/tools/thermal-guard.py:25` | `len(rows)!=2` raises `temperature unavailable`, so with four dies reporting **every guarded test exits 91** before its command even starts — i.e. all of `verify.sh` and both bench stages. The count wants to come from the device count; the per-row 0-150 C check at :27 is the part worth keeping |
| `kunlunK200_llama.cpp/examples/kunlun/verify.sh:17,22` | `for pd in 0 1` — the exactness and fast-split matrices cover two dies by construction, so on four they silently test half the hardware |
| `env.sh:10` | puts only `$XRE/bin` on `PATH`, but `verify.sh:6` invokes the guard as a bare `k200-thermal-guard`, which installs to `$KUNLUN_PREFIX/bin`. As shipped, `verify.sh` dies at its first `run` on an otherwise correct tree |

`check-hw.sh` prepends the prefix bin itself and warns when the die count is
not 2, so bring-up is not blocked on any of this; the warning is there to keep
a wall of exit 91s from reading as a hardware fault. The fixes belong upstream.

## Topology is discoverable

`sdk/xpu-4.33.0/include/xpu/runtime.h`:

- `:35-51` `XPUATTR_BOARDID`, `XPUATTR_ONBOARDID`, `XPUATTR_PCI_ADDRESS`
- `:118` `xpu_device_get_attr(uint64_t * v, XPUDeviceAttr, int devid)`
- `:173` `xpu_memcpy_peer(dst_device, dst_addr, src_device, src_addr, sz)` —
  documented generically as a copy between different devices. No
  `can_access_peer` query and no P2P enable call, so the API accepts
  cross-card copies; whether they ride PCIe P2P or bounce through the host is
  only answerable by measurement.
- `:176-216` streams and events exist. The library uses the default stream and
  `xpu_wait()` throughout. Relevant if transfers are ever overlapped with
  compute across dies.

Dies on one card share a `BOARDID`, so grouping needs no guessing from device
order. Worth exposing from the library as something like
`struct DeviceTopology { int board, on_board; uint64_t pci; };
DeviceTopology topology(int device);` returning -1 on attribute failure rather
than the library's usual fail-fast abort, since firmware support is unknown.
The backend needs the same grouping to pair dies per card.

## Bench extension (done)

`dual-die/dual-die-bench.cpp` asked three questions hardcoded to PD0 and PD1.
It now enumerates every die and runs four:

- **0** the die -> card map from `BOARDID`, with `ONBOARDID`, model, PCI
  address and HBM/L3 capacity per die, and the dies grouped per card
- **A** peer round-trip for **every ordered `(src,dst)` pair** at activation
  size — the matrix that decides whether N-way tensor parallel is worth
  building — reduced to a same-card and a cross-card median, against a
  same-die `device_to_device` copy as the floor, plus a size sweep on one
  representative pair of each kind
- **B** serial vs overlapped GEMV for *n* = 1..N dies, so contention shows up
  as it accumulates rather than only at the top
- **C** N-split GEMV across all dies, gathered onto die 0, against the
  single-die full-N time, with the Q4_0 gold check

Two things the extension needed that the plan above only anticipated:

`dual-die/topology.h` holds the `BOARDID`/`ONBOARDID` reads. It is the
`topology()` the library should own — the backend needs the same grouping to
pair dies per card — and moving it there is a copy plus a rename. Until then
`build.sh` links `-lxpurt` alongside `-lkunlunk200`. It returns -1 on an
attribute failure instead of the library's fail-fast `require()`, because
firmware support for these two attributes is unknown and a bench that cannot
read the map must still run its measurements.

**The bench cannot exceed the library's two-device ceiling**, which the order
of work below did not account for: `select()` aborts the process above it, so
on four dies the extended bench would die in test 0. It now reads the ceiling
off the public ABI — the width of `Statistics::matmuls` *is* the device count
the library accepts — reports it, clamps to it, and names what to widen. That
also means it picks up `max_devices` automatically once that lands. So the
bench is written now but only measures four dies after step 3.

`dual-die/fake-runtime.cpp` is a host stand-in for the parts of
`libkunlunk200` and `libxpurt` the bench calls, linked into
`dual-die-bench-fake` from the same unchanged bench source. It makes every
call return immediately, so its timings are noise by construction, and that is
the whole of what it gives up. What it checks is the logic that would otherwise
sit unexecuted until the cards arrive: the grouping over any die count and with
`BOARDID` failing, the ordered-pair loop and the same/cross split, the gather
offsets end to end through the gold check, and — the part worth having — **die
ownership of every pointer** handed to `copy`, `peer` and `matmul`. The real
library takes a device id and trusts the pointers, so a copy issued with the
wrong die selected is accepted and silently wrong; that is exactly the bug the
old test C's gather had, issuing a die-0 local copy with die 1 selected.

## Order of work

1. ~~Bench extension (no hardware needed)~~ — done
2. Cards arrive: power, `xpu_smi` shows 4 dies, run `check-hw.sh`. The bench
   will report the two-device ceiling and measure dies 0-1 only. Widen
   `thermal-guard.py:25` first or every guarded stage exits 91 — it is a
   one-line change and nothing else in the ladder runs without it
3. Library `max_devices` widening + backend lines 96-98. The bench then
   measures all four with no further change
4. Only if the peer matrix is favourable: N-way tensor split

Steps 2 and 3 are in this order because step 2 is worth running the moment the
cards are in — a same-card figure and a cross-card figure for one pair each is
already most of the decision — but either order works, and doing 3 first gets
the full matrix in one pass.

## Hardware prerequisites

Two slots with spacing, EPS-12V per card, real airflow for passive cards. PCIe
P2P between slots usually wants both cards under the same root complex with
ACS disabled; if that is broken on this board the bench will show host-bounce
latency and step 4 is off the table. Steps 1-3 are what make two cards *work*
regardless.
