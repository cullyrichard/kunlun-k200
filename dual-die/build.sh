#!/usr/bin/env bash
# Build the dual-die bench against the installed KunlunK200 library.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
. "$here/../env.sh"
prefix="${KUNLUN_PREFIX:-$HOME/.local/kunlun}"

g++ -std=c++17 -O2 -Wall -Wextra \
  -I"$prefix/include" \
  "$here/dual-die-bench.cpp" \
  -L"$prefix/lib" -lkunlunk200 \
  -Wl,-rpath,"$prefix/lib" \
  -o "$here/dual-die-bench"
echo "built $here/dual-die-bench"
