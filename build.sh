#!/usr/bin/env bash
# Rebuild KunlunK200 lib + the llama.cpp fork from the staged SDK.
# Needs sdk/{xpu-4.33.0,xdnn,xtdk} and upstream/ already in place; see
# LLAMA-BUILD-NOTES.md for how those were assembled.
set -euo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
. "$root/env.sh"
prefix="${KUNLUN_PREFIX:-$HOME/.local/kunlun}"
jobs="${JOBS:-8}"

cmake -S "$root/upstream/kunlunK200_lib" -B "$root/build/library" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix"
cmake --build "$root/build/library" -j"$jobs"
cmake --install "$root/build/library"

cmake -S "$root/upstream/kunlunK200_llama.cpp" -B "$root/build/llama" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DGGML_KUNLUN=ON \
  -DKUNLUN_USE_SYSTEM_LIBRARY=ON -DCMAKE_PREFIX_PATH="$prefix" \
  -DLLAMA_BUILD_EXAMPLES=ON -DLLAMA_BUILD_TESTS=ON \
  -DLLAMA_BUILD_SERVER=ON -DLLAMA_BUILD_APP=ON
cmake --build "$root/build/llama" -j"$jobs"
