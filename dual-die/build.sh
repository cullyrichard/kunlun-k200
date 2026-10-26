#!/usr/bin/env bash
# Build the multi-die bench, twice:
#
#   dual-die-bench       against the real library and XRE. topology.h calls
#                        xpu_device_get_attr directly, so this links libxpurt
#                        as well: the library exposes no topology query yet
#                        (MULTI-CARD.md). Running it needs env.sh's
#                        LD_LIBRARY_PATH, rpath or not -- see env.sh.
#   dual-die-bench-fake  the same source against fake-runtime.cpp, a host
#                        stand-in. Runs the bench's logic with no card and no
#                        vendor library; its timings are meaningless by design.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
. "$here/../env.sh"
prefix="${KUNLUN_PREFIX:-$HOME/.local/kunlun}"

warnings=(-std=c++17 -O2 -Wall -Wextra)

g++ "${warnings[@]}" \
  -I"$prefix/include" -I"$XRE/include" \
  "$here/dual-die-bench.cpp" \
  -L"$prefix/lib" -lkunlunk200 \
  -L"$XRE/lib64" -lxpurt \
  -Wl,-rpath,"$prefix/lib" -Wl,-rpath,"$XRE/lib64" \
  -o "$here/dual-die-bench"
echo "built $here/dual-die-bench"

g++ "${warnings[@]}" \
  -I"$prefix/include" -I"$XRE/include" \
  "$here/dual-die-bench.cpp" "$here/fake-runtime.cpp" \
  -o "$here/dual-die-bench-fake"
echo "built $here/dual-die-bench-fake"
