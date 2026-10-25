# Kunlun KL1 (K200) driver + userspace on modern Arch

Status as of 2026-10-24. No K200 hardware present yet; everything below was
verified on the Manjaro workstation (kernel 7.1.13-2-MANJARO, glibc 2.44,
gcc 16.2.1 20260810).

## Userspace: works unmodified

XRE 4.33.0.1's prebuilt binaries need no compat shims on Arch.

| object | max GLIBC | max GLIBCXX | max CXXABI |
|---|---|---|---|
| libxpurt.so.4.33.0 | 2.17 | 3.4.22 | 1.3 |
| libxpuml.so.4.33.0 | 2.4 | - | - |
| xpu_smi | 2.17 | 3.4.22 | 1.3 |
| libxpuapi.so (XDNN KL1) | 2.14 | 3.4.21 | 1.3.7 |

Host provides GLIBC 2.44 / GLIBCXX 3.4.36 / CXXABI 1.3.17 — all satisfied.
`libxpuapi.so` uses the **new** `__cxx11` ABI (27 undefined `__cxx11` refs, zero
old-ABI `basic_string` refs), so do **not** build llama.cpp with
`_GLIBCXX_USE_CXX11_ABI=0`.

Verified, not just inferred:
- `dlopen()` of libxpuml.so, libxpurt.so and libxpuapi.so all succeed.
- A test program links against both `libxpurt.a` (static, gcc 16.2.1) and
  `libxpurt.so`, runs, and `xpu_device_count()` returns rc=0, count=0 — the
  correct no-card answer.
- `xpu_smi` runs and exits 0 with no devices.

`libxpuapi.so` has `NEEDED libxpurt.so` (unversioned) with `RPATH $ORIGIN`;
XRE's `so/` already ships the `libxpurt.so -> libxpurt.so.1` symlink, so keep
them in one directory.

## Kernel module: needs patches/kunlun-4.33.0-kernel71.patch

The driver's `kunlun_module/` is NVIDIA's open-GPU-kernel-module build system
rebranded (`NV_VERSION_STRING="510.39.01"`, 48 NVIDIA-copyright files under
`kunlun/port/`, stock 5898-line `conftest.sh` with 222 compile tests). So the
porting burden is NVIDIA's 2022-era kernel-interface layer, and NVIDIA's newer
releases are the reference for any further breakage.

With the patch, both `kunlun.ko` and `kunlun_peermem.ko` build clean against
7.1.13: 0 errors, 370 warnings (314 `-Wempty-body`, 52 `-Wmissing-prototypes`,
2 unused vars, 1 old-style declaration — all cosmetic). vermagic matches the
running kernel and all 288 undefined symbols resolve against `Module.symvers`.

### Load test (2026-10-24, no hardware)

`insmod kunlun.ko` succeeds on 7.1.13 and initialises cleanly:

```
kunlun: [INFO] load kunlun driver 4.33.0, commit 0322c392...(Mon Nov 25 15:50:57 2024 +0800)
kunlun: [INFO] create /proc/xpu
kunlun: [INFO] device number major= 234 minor= 0 ~ 128
kunlun: [INFO] create cdev for minor 0 ~ 127
kunlun: [ERR ] no supported device found
kunlun: [INFO] module load success
```

`no supported device found` is the correct answer with no card. It creates
`/proc/xpu` (version 4.33.0, `probe_error` empty, `wait_mode` hybrid),
`/sys/class/xpu`, chrdev major 234, and the control node `/dev/xpuctrl`
(minor 128).

End-to-end with the vendor userspace: `xpu_smi` now prints
`Runtime Version: 4.33 / Driver Version: 4.33` with an empty device table —
before the module was loaded it printed nothing at all. So the patched driver
and the 2022-era userspace do talk to each other on kernel 7.1 / glibc 2.44.

`rmmod kunlun` also succeeds; `/proc/xpu`, `/dev/xpuctrl`, `/sys/class/xpu` and
the chrdev major are all released. No warnings, oops, lockdep splats or
refcount complaints across load or unload. Kernel taint is 12288 (bit 12
out-of-tree + bit 13 unsigned), as expected for an unsigned OOT module.

**Still untested:** everything behind `probe()` — the whole hardware path.

### What the patch does

Four of the fixes are build-infrastructure, and they account for most of it:
the original tree produced 232 error lines, of which only ~25 were real.

1. **`EXTRA_CFLAGS` -> `ccflags-y`** (`Kbuild`). Modern Kbuild honors
   `EXTRA_CFLAGS` nowhere (zero hits in 7.1's `Makefile`/`Makefile.lib`), so
   every `-I` path and `-D` the driver set was being silently dropped.
2. **`-DNV_LINUX`** (`Kbuild`). 7.1 is `-nostdinc` with no `-isystem <gcc
   include>` at all, so `nv.h`'s `#include <stddef.h>` fallback cannot resolve;
   `NV_LINUX` selects the `<linux/stddef.h>` branch.
3. **`-std=gnu11` in conftest** (`conftest.sh`). gcc 16 defaults to `gnu23`,
   where `bool`/`true`/`false` are keywords, so *every* conftest compile died
   inside `linux/stddef.h`. Everything was misdetected.
4. **`-fms-extensions` in conftest** (`conftest.sh`). 7.1's `struct filename`
   embeds a named struct anonymously (`CC_FLAGS_DIALECT`), so without it a
   `static_assert` in `linux/fs.h` fails and every test including `linux/mm.h`
   reported absent.
5. **`-fpermissive` in conftest** (`conftest.sh`). conftest's "functions" tests
   use inverted logic that assumes implicit-function-declaration and friends
   are *warnings*; gcc 14+ made them errors, which inverted ~130 results
   (e.g. `jiffies_to_timespec`, removed in 5.6, was reported present).

Fixes 3-5 together took the error count from 232 to 25 without touching a line
of driver C.

Genuine API drift, fixed in the driver's own idiom:

6. `vm_area_struct_has_const_vm_flags` conftest probed `offsetof(struct
   vm_area_struct, __vm_flags)`; that member is now `flags` (`vma_flags_t`), so
   the probe now tests for `vm_flags_set()` instead. The driver already had
   working `nv_vm_flags_set/clear` helpers behind the macro.
7. `in_irq()` -> `in_hardirq()` (`nv-time.h` x2, `os-interface.c`).
8. `del_timer()` -> `timer_delete()`, removed in 6.15; compat macro for < 6.2
   added to `kl_compat.h` (which also gained `<linux/version.h>`).
9. `hrtimer_init()` -> `hrtimer_setup()`, removed in 6.15; `hrtimer_setup()`
   folds in the `.function` assignment. Compat macro for < 6.15 in
   `kl_compat.h`; three call sites in `kl2/main.c` and `kl2/video_perf.c`.
10. `follow_pfn()` removed -> `follow_pfnmap_start()`/`follow_pfnmap_end()`
    (6.12+), behind a new `follow_pfnmap_start` conftest (`os-mlock.c`).
11. `dma_map_ops.map_resource` hook gone -> new `dma_map_ops_has_map_resource`
    conftest; when absent, return true, since the generic `dma_map_resource()`
    path now covers every implementation (`nv-dma.c`).
12. `registered_fb[]`/`num_registered_fb` made private in 6.1 -> new
    `num_registered_fb` conftest guards `os_get_screen_info()`'s loop
    (`os-interface.c`). Console-framebuffer probing is dead weight on a
    compute card anyway.
13. `kl2/dma.c`: `os_lock_user_pages(addr, ...)` passed a `u64` to a `void *`
    parameter — a long-standing sloppiness that only became an error under
    gcc 14+. Now cast explicitly.

### Reproducing the build

```sh
tar xzf xre-ubuntu_2004_x86_64.tar.gz
cd xre-ubuntu_2004_x86_64/kunlun-module-dkms-4.33.0
patch -p1 < .../kunlun-4.33.0-kernel71.patch
cd kunlun_module
make -j8 IGNORE_XEN_PRESENCE=1 IGNORE_PREEMPT_RT_PRESENCE=1 \
     IGNORE_CC_MISMATCH=1 KERNELDIR=/lib/modules/$(uname -r)/build modules
```

Needs `linux71-headers` (or the matching headers for your kernel) and `pahole`
— the latter only for the BTF step; without it the link succeeds and the BTF
stage fails with `pahole: command not found`.

Manjaro builds its kernel with gcc 16.2.1, the same gcc that is installed, so
there is no real compiler mismatch; the `IGNORE_CC_MISMATCH=1` warning is only
`cc` vs `gcc` in the version string. `CONFIG_MODVERSIONS` is off and
`CONFIG_MODULE_SIG_FORCE` is off, so an unsigned out-of-tree module loads (with
a taint).

## What this changes about the distro plan

The earlier "Debian first, then port to Arch" plan was motivated partly by
userspace risk (old vendor binaries vs new glibc) and partly by the driver vs
compiler question. Both are now answered for Arch: userspace runs as-is, and the
driver compiles against a kernel far newer than anything the vendor tested.
Remaining Arch risk is confined to runtime behaviour, which needs the card.

## Next

- With hardware: power (EPS-12V), enumeration, cooling, `xpu_smi`, then the
  llama.cpp backend.
