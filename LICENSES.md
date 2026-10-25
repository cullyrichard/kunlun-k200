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
| `include/xpu/`, `include/xpurt_priv/` | 13 | no header |
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

The 13 headers under `include/` are Kunlunxin's own ioctl and ABI definitions
and carry no license header. They are included because the module cannot be
compiled without them (`Kbuild:71` adds `-I$(src)/../include`), and this
repository is private. **Before making this repository public**, either strip
them and restore the build from the vendor tarball, or re-declare the ioctl
numbers and struct layouts from the MIT-licensed driver's own usage.

Everything outside `driver/` is original work in this repository.
