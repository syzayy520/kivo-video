#!/usr/bin/env bash
# llama.cpp → Android arm64-v8a 交叉编译（AI 字幕翻译内核）。
#
# 依赖链：llama.cpp（MIT，ggml 推理内核）→ libllama.a / libggml*.a（静态库）。
#
# 产物：dist/lib/*.a + dist/include/*.h，供 android/app/src/main/cpp/CMakeLists.txt
#      以 IMPORTED target 方式链接（与 ffmpeg-android / libass-android 同一范式）。
#
# 与 libass-android 的路线差异：libass 是 autotools/meson 链，本脚本是 **CMake 链**。
# 因此本脚本**不需要 MSYS2 的 make/ninja 参与编译**（CMake 自带 NDK 交叉支持），
# 也就不必踩 libass build.sh 里记录的那 7 个 MSYS2 坑。
# 但脚本仍写成 bash：仓库 third_party 下的构建脚本一律是 build.sh，从 MSYS2 跑。
#
# 五个已踩平的坑（全部沉淀在本脚本里）：
#   ① 构建目录必须**纯 ASCII**：路径含中文时 ninja 报
#      `failed recompaction: Permission denied`，CMake 直接 Generate 失败。
#      libass/ffmpeg 都用 /c/re-tools/... 就是这个原因。
#   ② GGML_BACKEND_DL=OFF 是**硬开关**，不是可选项：默认开的话静态链接进 App 后
#      运行时找不到 ggml-*-vulkan.so，Vulkan 静默不可用（不报错，只是没 GPU 加速）。
#   ③ ANDROID_STL=c++_static：工具目标要静态可执行文件；静态库场景下
#      与 BUILD_SHARED_LIBS=OFF 配套，避免 c++_shared 依赖打进 APK。
#   ④ GGML_NATIVE=OFF：NDK 交叉编译时宿主 CPU 特性探测没有意义，
#      开着会按宿主（x86）选指令集，产物在 arm64 上可能非法指令。
#   ⑤ Vulkan 需要 **glslc**（LunarG Vulkan SDK）：ggml-vulkan 的 CMake 是
#      `find_package(Vulkan COMPONENTS glslc REQUIRED)`，硬依赖。
#      本机没装 SDK 时默认关 Vulkan；装了设 LLAMA_VULKAN=1 打开。
#
# 前提：llama.cpp 源码在 /c/re-tools/llama.cpp-src/（LLAMA_SRC_DIR 可覆盖）；
# 从 MSYS2 bash 里跑：/c/msys64/usr/bin/bash -lc 'cd "/c/kivo video" && bash ...'
set -euo pipefail

NDK="${NDK:-/c/Android/sdk/ndk/30.0.16248370}"
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${LLAMA_SRC_DIR:-/c/re-tools/llama.cpp-src}"
API=26
ABI=arm64-v8a
DIST_WIN="C:/re-tools/llama.cpp-android/dist"
DIST="${LLAMA_DIST_DIR:-/c/re-tools/llama.cpp-android/dist}"
BUILD_ROOT="${LLAMA_BUILD_DIR:-/c/re-tools/llama.cpp-android/build}"

TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/windows-x86_64"
NM="$TOOLCHAIN/bin/llvm-nm.exe"
# NDK 的 CMake toolchain 文件在 $NDK/build/cmake/ 下（不在 toolchains/llvm/ 里）。
ANDROID_TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"

# ① 纯 ASCII 路径 —— 见坑 ①
case "$DIST$BUILD_ROOT$SRC_DIR" in
    *[!a-zA-Z0-9_./:-]*)
        echo "路径含非 ASCII 字符，ninja 会报 failed recompaction（坑 ①）"; exit 2 ;;
esac

if [ ! -d "$SRC_DIR" ]; then
    echo "找不到 llama.cpp 源码: $SRC_DIR"
    echo "  clone: git clone --depth 1 https://github.com/ggml-org/llama.cpp.git $SRC_DIR"
    exit 2
fi

mkdir -p "$BUILD_ROOT" "$DIST/lib" "$DIST/include"

# ---- 工具定位 ----
# 【坑 ⑥】MSYS2 的 PATH 里没有 cmake / ninja（Windows 侧装的）。
# 这与 libass build.sh 里的坑 ⑤ 同源 —— MSYS2 自带的 meson/python
# 在 Windows 上自检会拒跑，libass 的解法是「Windows Python 的 meson 全路径直呼」。
# 这里照抄那个解法：解析出 Windows 形式的全路径再调用。
CMAKE_EXE="${CMAKE_EXE:-}"
if [ -z "$CMAKE_EXE" ]; then
    if command -v cmake >/dev/null 2>&1; then
        CMAKE_EXE="$(command -v cmake)"
    elif [ -f "/c/Program Files/CMake/bin/cmake.exe" ]; then
        CMAKE_EXE="/c/Program Files/CMake/bin/cmake.exe"
    else
        echo "找不到 cmake —— 设 CMAKE_EXE=<全路径> 或装 CMake"
        exit 2
    fi
fi

# ninja 同理：Windows 侧用 winget 装的，链接目录不在 MSYS2 的 PATH 里。
# 【坑 ⑦】MSYS2 的 PATH 传给 CMake（Windows 原生程序）时不完全等价 ——
# CMake 报 "Could not find a build program corresponding to Ninja"。
# 解法与坑 ⑥ 同源：把 ninja 的 **Windows 形式全路径**显式交给 CMake。
CMAKE_MAKE_PROGRAM=""
NINJA_DIR="/c/Users/Administrator/AppData/Local/Microsoft/WinGet/Links"
if [ -f "$NINJA_DIR/ninja.exe" ]; then
    CMAKE_MAKE_PROGRAM="C:/Users/Administrator/AppData/Local/Microsoft/WinGet/Links/ninja.exe"
elif command -v ninja >/dev/null 2>&1; then
    CMAKE_MAKE_PROGRAM="$(command -v ninja)"
    case "$CMAKE_MAKE_PROGRAM" in
        /c/*) CMAKE_MAKE_PROGRAM="C:/${CMAKE_MAKE_PROGRAM#/c/}" ;;
    esac
fi

# ---- 生成器：优先 ninja，退回 Unix Makefiles ----
if [ -n "$CMAKE_MAKE_PROGRAM" ]; then
    GENERATOR="Ninja"
else
    GENERATOR="Unix Makefiles"
fi

# ---- Vulkan 开关（坑 ⑤）----
GLSL_ARGS=()
VULKAN_LIB_NAME=""
if [ "${LLAMA_VULKAN:-0}" = "1" ]; then
    GLSLC="$(command -v glslc || true)"
    if [ -z "$GLSLC" ] && [ -d /c/VulkanSDK ]; then
        GLSLC="$(ls -d /c/VulkanSDK/*/Bin/glslc.exe 2>/dev/null | head -1 || true)"
    fi
    if [ -z "$GLSLC" ]; then
        echo "LLAMA_VULKAN=1 但找不到 glslc —— 请先装 LunarG Vulkan SDK（坑 ⑤）"
        exit 2
    fi
    GLSL_ARGS+=("-DVulkan_GLSLC_EXECUTABLE=$GLSLC" "-DGGML_VULKAN=ON")
    VULKAN_LIB_NAME="libggml-vulkan.a"
    echo "Vulkan: ON（glslc=$GLSLC）"
else
    GLSL_ARGS+=("-DGGML_VULKAN=OFF")
    echo "Vulkan: OFF（设 LLAMA_VULKAN=1 可打开，需要 LunarG SDK 的 glslc）"
fi

echo "== configure =="
if [ -n "$CMAKE_MAKE_PROGRAM" ]; then
    MAKE_ARGS=(-DCMAKE_MAKE_PROGRAM="$CMAKE_MAKE_PROGRAM")
else
    MAKE_ARGS=()
fi

"$CMAKE_EXE" -S "$SRC_DIR" -B "$BUILD_ROOT" \
    -G "$GENERATOR" \
    "${MAKE_ARGS[@]}" \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_TOOLCHAIN" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$API" \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLAMA_CURL=OFF \
    -DGGML_OPENMP=OFF \
    -DGGML_NATIVE=OFF \
    -DGGML_LLAMAFILE=OFF \
    -DGGML_OPENCL=OFF \
    -DGGML_RPC=OFF \
    -DGGML_BACKEND_DL=OFF \
    -DGGML_STATIC=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DLLAMA_BUILD_TESTS=OFF \
    -DLLAMA_BUILD_EXAMPLES=OFF \
    -DLLAMA_BUILD_SERVER=OFF \
    -DLLAMA_BUILD_TOOLS=OFF \
    -DLLAMA_BUILD_COMMON=OFF \
    "${GLSL_ARGS[@]}"

echo "== build =="
"$CMAKE_EXE" --build "$BUILD_ROOT" --target llama -j 8

echo "== 收集产物 =="
rm -f "$DIST"/lib/*.a
FOUND=0
for lib in libllama.a libggml.a libggml-base.a libggml-cpu.a; do
    src="$(find "$BUILD_ROOT" -name "$lib" -type f | head -1)"
    if [ -n "$src" ]; then
        cp "$src" "$DIST/lib/"
        echo "  $lib  $(du -k "$src" | cut -f1) KB"
        FOUND=$((FOUND + 1))
    else
        echo "  缺 $lib"
    fi
done
if [ -n "$VULKAN_LIB_NAME" ]; then
    src="$(find "$BUILD_ROOT" -name "$VULKAN_LIB_NAME" -type f | head -1)"
    if [ -n "$src" ]; then
        cp "$src" "$DIST/lib/"
        echo "  $VULKAN_LIB_NAME  $(du -k "$src" | cut -f1) KB"
    fi
fi
[ "$FOUND" -eq 4 ] || { echo "产物不全（$FOUND/4）"; exit 1; }

# 头文件：llama.h / ggml.h / ggml-*.h
rm -f "$DIST"/include/*.h
find "$SRC_DIR/include" -maxdepth 1 -name "*.h" -exec cp {} "$DIST/include/" \;
find "$SRC_DIR/ggml/include" -maxdepth 1 -name "*.h" -exec cp {} "$DIST/include/" \;
find "$SRC_DIR/ggml/src" -maxdepth 1 -name "*.h" -exec cp {} "$DIST/include/" \; 2>/dev/null || true
echo "  头文件 $(ls "$DIST/include" | wc -l) 个"

# ---- 符号自检（对齐 libass build.sh 的做法）----
echo "== 符号自检 =="
# 【坑 ⑧】不要写 `"$NM" "$lib" | grep -q "$sym"`，也不要写 `echo "$syms" | grep -q ...`：
#   grep -q 命中后立刻退出 → 管道另一端收到 EPIPE/SIGPIPE → 返回非零 →
#   脚本顶部的 `set -o pipefail` 把整条管道判为失败 → 符号明明在库里却报「不在任何库」。
#   （实测 llvm-nm 的管道在 MSYS2 下返回 74。）
#   解法：**完全不用管道** —— 先把符号表抓进变量，再用 herestring 喂给 grep
#   （herestring 走临时 fd，不产生管道，也就没有 SIGPIPE）。
for sym in llama_model_load_from_file llama_init_from_model llama_decode llama_sampler_sample; do
    found=0
    for lib in "$DIST"/lib/*.a; do
        syms="$("$NM" "$lib" 2>/dev/null || true)"
        if grep -q "$sym" <<< "$syms"; then
            found=1
            echo "  $sym <- $(basename "$lib")"
            break
        fi
    done
    [ "$found" = 1 ] || { echo "自检失败：$sym 不在任何库"; exit 1; }
done

ls -la "$DIST/lib/"
echo "OK：llama.cpp-android dist 就绪（$DIST）"
