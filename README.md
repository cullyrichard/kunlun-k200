# Kunlun KL1 (K200) on modern Linux

Porting Kunlunxin's KL1 stack — kernel driver, vendor userspace, and the
llama.cpp backend that runs on it — to a current Arch/Manjaro system
(kernel 7.1.13, glibc 2.44, gcc 16.2.1). **No K200 hardware is present yet**,
so everything here is verified as far as a machine without the card can go.

## Status

| piece | state |
|---|---|
| Kernel module (`kunlun.ko`, `kunlun_peermem.ko`) | builds clean on 7.1.13, loads, initialises, unloads; `no supported device found` is the correct no-card answer |
| Vendor userspace (XRE 4.33.0.1) | runs unmodified; `xpu_smi` reports runtime and driver 4.33 |
| KunlunK200 library 0.4.0 | 75/75 targets, all 32 `.xpu` kernels compile |
| llama.cpp fork | 459/459 targets, backend registers, CPU inference end-to-end |
| Everything behind `probe()` | untested — needs the card |

`PORTING.md` is the driver write-up, `LLAMA-BUILD-NOTES.md` the userspace and
llama.cpp build, `INT4-FEASIBILITY.md` the analysis of where the performance
actually goes (short version: not the MAC).
`QUALITY-RESULTS.md` measures what the device's activation quantization costs,
on CPU, ahead of the card: decode is free, prefill is not.

## Layout

```
driver/      Kunlunxin kunlun-module-dkms-4.33.0, imported pristine, then
             ported to 7.1 on top -- see git history and LICENSES.md
patches/     the same port as a standalone patch against the vendor tarball,
             its build log, and the KUNLUN_EMU patch for the llama.cpp fork
quality/     fixed-input quality harness: pinned WikiText-2 IDs and the sweep
             that scores each K200 activation-quantization mode on CPU
dual-die/    PD0+PD1 peer-copy, bandwidth and N-split GEMV benchmarks
byte-map.csv weight-layout map for the native Q4 path
build.sh     rebuild the library and the llama.cpp fork from a staged SDK
env.sh       XRE/XDNN/XTDK paths and the LD_LIBRARY_PATH the vendor libs need
```

`sdk/`, `upstream/`, `models/` and `build/` are deliberately untracked; see
`.gitignore`.

## Building the driver

At `HEAD` the tree is already ported, so:

```sh
cd driver/kunlun_module
make -j8 IGNORE_XEN_PRESENCE=1 IGNORE_PREEMPT_RT_PRESENCE=1 \
     IGNORE_CC_MISMATCH=1 KERNELDIR=/lib/modules/$(uname -r)/build modules
```

Needs the matching kernel headers, and `pahole` for the BTF step. To apply the
port to a fresh vendor tarball instead, `patch -p1 <
patches/kunlun-4.33.0-kernel71.patch` from the `kunlun-module-dkms-4.33.0`
directory. `PORTING.md` explains each of the 13 changes and why it was needed.

## Upstream pins

The two forks this work builds against are not vendored here:

- `github.com/UnagiDojyou/kunlunK200_llama.cpp` @ `73635ca`
- `github.com/UnagiDojyou/kunlunK200_lib` @ `d98643c` (v0.4.0)

Clone them into `upstream/`, stage the SDK as `LLAMA-BUILD-NOTES.md` describes,
then `./build.sh`. `check-no-hw.sh` re-verifies what is checkable without a card.

## Licensing

The driver is genuinely open source — MIT, per file, from Kunlunxin, NVIDIA and
Baidu. `LICENSES.md` has the full breakdown, the two things excluded from the
import, and what to do before this repository is made public.
