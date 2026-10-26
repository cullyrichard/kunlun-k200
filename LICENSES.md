# Licensing of the imported driver

`driver/` is an import of Kunlunxin's `kunlun-module-dkms-4.33.0`, taken from
the XRE 4.33.0.1 release tarball (`xre-ubuntu_2004_x86_64.tar.gz`). The XRE
bundle as a whole ships under a Kunlunxin EULA, but the kernel module's own
files carry explicit per-file grants, and those are what govern the files here.

Counted over the 169 imported files:

| scope | files | license |
|---|---:|---|
| `kunlun_module/` C and headers | 142 | `SPDX-License-Identifier: MIT` |
| `kunlun_peermem/kunlun_peermem.c` | 1 | `Linux-OpenIB`, matching its `MODULE_LICENSE` |
| `kunlun_module/` build files | 7 | no header: `conftest.sh`, `Kbuild`, `Makefile`, `kunlun.Kbuild`, `kunlun-sources.Kbuild`, `kunlun_peermem.Kbuild`, `99-kunlun.rules` |
| `include/xpu/`, `include/xpurt_priv/` | 0 | **not imported** — re-declared instead, below |
| `Makefile`, `dkms.conf` | 2 | no header |

140 of the MIT files carry the full "Permission is hereby granted, free of
charge" text; all notices are preserved as imported. Copyright on the MIT set
is Kunlunxin (94 files), NVIDIA (48, inherited from the open-GPU-kernel-module
port layer this driver is built on) and Baidu (15). `kunlun/kl_main.c` declares
`MODULE_LICENSE("GPL")`.

Two deliberate exclusions from the import:

- `include/cudart/` — a bundled copy of NVIDIA's CUDA headers (`cuda.h`,
  `cuda_runtime_api.h` and ~20 more), proprietary and restricted. The kernel
  module never includes one of them; verified by grepping every `#include` of
  `cudart/` across the module sources, which returns nothing. Dropped.
- `sdk/xre/kunlun_module/*.ko` — 136 MB of prebuilt modules for Ubuntu 5.11
  kernels. Not source, not useful here.

## The interface headers under `include/`

The module cannot compile without the ioctl and ABI definitions that the
vendor ships under `include/` (`Kbuild:71` adds `-I$(src)/../include`). There
are 18 such files in the tarball, and unlike the driver sources none of them
carries a license grant: each asserts `copyright (C) 2018 Baidu, Inc` or
`Copyright 2018 Baidu Inc. All Rights Reserved.` and stops there. Copyright
asserted, permission absent — so redistributing them is not ours to do, and
the XRE EULA independently forbids it (Art. 2.5, Art. 3.2).

They are therefore **not imported**. `include/` holds re-declarations instead:

- Five of the 18 are not needed at all. `bincommon.h`, `xarray.h` and
  `variadic_table.h` are userspace C++ (`std::shared_ptr<XArray>`), which a
  kernel module cannot use; `runtime.h` and `runtime_ex.h` are never
  included. They are simply gone.
- The remaining 13 keep their paths, because the driver sources include them
  by path, but not their contents. Each holds only what the MIT-licensed
  sources actually use: 166 constants and ioctl numbers, and the 48 types
  they need, in `kl_abi_types.h`.
- Structure layouts were recovered from the compiled driver's own debug info
  rather than transcribed, and the constants are the interface's numbers.
  `xpu_strerror`'s message strings are rewritten; nothing reads them but the
  log.

Those numbers and layouts are facts about an interface, and they have to
match the vendor runtime bit for bit or the driver cannot talk to it — which
is the thing `driver/check-abi.sh` proves. Point `VENDOR_INCLUDE` at an
extracted vendor tarball and it compiles the same probe against both header
sets and compares what the compiler made of each: 58 definitions identical
including every field offset, 165 constants identical in value. Re-declaring
an interface this way is the case *Google LLC v. Oracle America* (2021)
addressed directly; copying the files would not have been.

**Still outstanding before this repository can be made public:** the vendor
headers remain in git history, in the import commit `00b895e`. Publishing
exposes history, so that needs rewriting — deleting them from the tip is not
enough.

Everything outside `driver/` is original work in this repository.
