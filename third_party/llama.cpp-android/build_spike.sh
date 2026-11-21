#!/usr/bin/env bash
# T-0 spike 可执行构建：NDK 工具链链 llama dist 静态库 → arm64 kivo_ai_spike。
# 前置：build.sh 已出 dist（four libs + include）。从 MSYS2 bash 跑。
set -euo pipefail

NDK="${NDK:-/c/Android/sdk/ndk/30.0.16248370}"
API=26
ABI=arm64-v8a
DIST="${LLAMA_DIST_DIR:-/c/re-tools/llama.cpp-android/dist}"
BUILD_ROOT="${LLAMA_SPIKE_BUILD_DIR:-/c/re-tools/llama.cpp-android/spike-build}"
HERE="$(cd "$(dirname "$0")" && pwd)"

TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
CMAKE_EXE="${CMAKE_EXE:-/c/Program Files/CMake/bin/cmake.exe}"
NINJA_EXE="C:/Users/Administrator/AppData/Local/Microsoft/WinGet/Links/ninja.exe"

# 纯 ASCII 路径断言（与 build.sh 坑 ① 同源）。
case "$BUILD_ROOT" in
    *[!a-zA-Z0-9_./:-]*) echo "路径含非 ASCII（坑 ①）"; exit 2 ;;
esac

[ -f "$DIST/lib/libllama.a" ] || { echo "缺 dist/lib/libllama.a —— 先跑 build.sh"; exit 2; }

mkdir -p "$BUILD_ROOT"

echo "== configure spike =="
"$CMAKE_EXE" -S "$HERE/spike" -B "$BUILD_ROOT" \
    -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$NINJA_EXE" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$API" \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLAMA_DIST_DIR="C:/re-tools/llama.cpp-android/dist"

echo "== build spike =="
"$CMAKE_EXE" --build "$BUILD_ROOT" --target kivo_ai_spike -j 8

echo "== 产物 =="
ls -la "$BUILD_ROOT/kivo_ai_spike"
echo "OK: kivo_ai_spike 就绪（adb push 到 /data/local/tmp/ 后可直接跑）"