# llama.cpp + KunlunK200 library on Arch: build DONE (2026-10-24)

Both trees build, install and run as far as a machine with no K200 can go.
Reproduce with `./build.sh`; re-verify with `./check-no-hw.sh`.
Everything below was observed on Manjaro, kernel 7.1.13-2-MANJARO, glibc 2.44,
gcc 16.2.1, cmake 4.4.3, ninja 1.13.2.

## SDK layout as assembled

`env.sh` exports these three and the `LD_LIBRARY_PATH` the vendor libraries need.

- `XRE=sdk/xpu-4.33.0` - built by hand to match what `install_rt.sh` produces:
  `lib64/` is `xre/lib/*` plus `xre/so/*`, next to `include/`, `bin/`, `tools/`
  and `version.txt`. The fork's CMake looks for `libxpurt` in `lib64` then
  `lib`, and `lib` holds only static archives, so without `lib64` it would link
  the `.a`.
- `XDNN=sdk/xdnn` - `include/` sparse-fetched from
  `sam6321/KunlunK200@5ead5bb10ab6d747e0f33b739826f111e42ec06a`,
  path `third_party/xdnn/include`: 33 files, aggregate sha256 of the sorted
  per-file sums `942003a7c6cc878d64f43fddcdc767fbf1d5f352879eb5ba4adda8f27bf7d3d3`.
  `so/libxpuapi.so` is the KL1 payload already verified in a previous session
  (sha256 `e97dc7d2ce0f34df8d6584d1e9eb49e22344ce55c2cd2dcc9ff0e9ca88bff0f5`).
  This closes the one acquisition gap the earlier notes recorded.
- `XTDK_DIR=sdk/xtdk/xtdk-llvm15-ubuntu1604_x86_64` - XTDK 2.7.98.2, really
  clang-10, unchanged from the earlier session.

## The libstdc++-16 worry was unfounded

`cmake/compile-native.sh` compiles every kernel with `--xpu-device-only` and
links the device ELF with XTDK's own `lld` and `libclang_rt.builtins-xpu.a`. No
host translation unit is ever handed to clang-10, so the clash between clang-10
and Arch's libstdc++ 16 headers cannot arise in this build. The earlier notes
inferred this from the README; it is now read off the build system and confirmed
by a complete build. The axpby example's `main.cpp` was the only host TU that
hit it, and that example is not part of either build.

## KunlunK200 library 0.4.0

75/75 targets. All **32 `.xpu` kernels** compiled, non-empty, with entry symbols
extracted by `llvm-nm` (e.g. `kunlun-common-gemv` -> `_Z9ds_dual16Pf...`,
`kunlun-native-q4` -> `_Z16next_bulk_n64_2dPf...`). Warnings only: an unused
parameter in vendor `xpu/refactor/attention.h` and one `-Wsubobject-linkage` on
`CommonQ4Workspace::Impl`. Installed to `~/.local/kunlun`.

CPU-only self-check passes:

```
CPU_LAYOUT {"cases":30,"mapped_bytes":22216704,"all_fp16_bits":true,"reversible":true,"padded_bpw":4.5}
```

4.5 bpw and reversibility match what `docs/native-q4.md` claims.

## llama.cpp fork

Configured with `-DGGML_KUNLUN=ON -DKUNLUN_USE_SYSTEM_LIBRARY=ON
-DCMAKE_PREFIX_PATH=~/.local/kunlun`. The installed library is bit-for-bit the
commit the fork pins (`vendor/KunlunK200` -> `d98643c`, which is what
`upstream/kunlunK200_lib` is checked out at), so the system-library route is
equivalent to the submodule here and avoids a second clone.

459/459 targets, zero compile errors. `libggml-kunlun.so.0.27.0` built and all
four `kunlun-*` example binaries linked. The single "error" string in the log is
cosmetic: the web UI's pinned `dist.tar.gz.sha256` 404'd at tag `b50`, the build
fell back to `latest`, verified the archive and embedded 70 assets.

## Runtime environment: one real finding

`libxpuapi.so` carries `RPATH=$ORIGIN` and a `DT_NEEDED` on the **unversioned**
`libxpurt.so`. DT_RUNPATH is not transitive, so the caller's runpath cannot
satisfy it and `$XRE/lib64` must be on `LD_LIBRARY_PATH`. This is exactly what
upstream's `setup/install.sh` and `docs/troubleshooting.md` require; `env.sh`
does it. Without it, `ldd` on the backend shows `libxpurt.so => not found`
while `libxpurt.so.1` resolves, which is a confusing symptom worth remembering.

## How far it gets with no card

- `llama-cli --list-devices` loads XPURT 4.33, enumerates zero devices and exits
  0. The backend registers without aborting.
- `k200-bench memory` loads, initializes and reaches `xpu_set_device(0)`, which
  returns `devid(0) not exist` / status `-701`; the library then aborts. That is
  the documented fail-fast API, not a port defect.
- `k200-smoke` prints `FAIL: need two PDs`.
- **CPU end-to-end inference works through the fork's own harness.**
  `kunlun-llm-smoke models/tinyllama-1.1b-chat-v1.0.Q4_0.gguf cpu 8` on
  TinyLlama 1.1B Chat Q4_0 (637,699,456 bytes, sha256
  `da3087fb14aede55fde6eb81a0e55e886810e43509ec82ecdc7aa5d62a03b556`) completes
  "The capital of France is" -> " Paris.\n\n2. The capital", 801 graph nodes,
  1 split, and reports 45.43 PP / 48.15 TG tok/s. This is the CPU baseline and
  the teacher-token source `examples/kunlun/verify.sh` needs later.

## Fork currency

The fork branches from upstream at `f2f667b` (2026-10-17), ggml 0.27.0,
llama.cpp 0.7.0-dev. Its `supports_op` is therefore current with ggml as of a
week ago, and its guards are conservative (contiguity, K % 32, K <= 16384,
`INT_MAX` byte limits, split-buffer and SET_ROWS layout checks). Nothing to
reconcile against upstream unless a rebase is wanted.

## What is left

Everything remaining needs the card:

1. EPS-12V power, enumeration, cooling; then `xpu_smi`, `k200-smoke`,
   `k200-bench`, `examples/kunlun/verify.sh`.
2. Reproduce the published 11.15 TG / 34.67 PP tok/s on Llama 2 7B Q4_0.
3. Value-add: attention on device (ROPE, SOFTMAX, batched strided GEMM) to break
   the PCIe wall, and the INT4 path. Read `docs/native-q4.md` and
   `docs/common-q4.md` first - upstream v0.3/v0.4 already cover part of it.
