# Source this to build or run anything in ~/dev/kunlun-k200.
# libxpuapi.so has RPATH=$ORIGIN and a DT_NEEDED on the unversioned
# "libxpurt.so", which RUNPATH on the caller cannot satisfy (RUNPATH is not
# transitive), so XRE's lib64 must be on LD_LIBRARY_PATH -- matching what
# upstream's install.sh and docs/troubleshooting.md require.
K200_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
export XRE="$K200_ROOT/sdk/xpu-4.33.0"
export XDNN="$K200_ROOT/sdk/xdnn"
export XTDK_DIR="$K200_ROOT/sdk/xtdk/xtdk-llvm15-ubuntu1604_x86_64"
export PATH="$XRE/bin:$PATH"
export LD_LIBRARY_PATH="$XRE/lib64:$XDNN/so${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
