#!/usr/bin/env bash
# FFmpeg 7.1 → Android arm64-v8a 交叉编译（211 软解兜底）。
#
# 产物：静态库（libavformat.a / libavcodec.a / libswscale.a / libavutil.a）+ 头文件
# 落在本目录 dist/。**不做动态 ffmpeg**：Android 打不了带版本号的 soname
# （libavcodec.so.61 过不了 APK 的 *.so 过滤），改由上层把静态库封装进
# 单一 `libkivo_ffmpeg.so`（CMake 集成阶段）。LGPL 合规见 README.md。
#
# 前提：
#   NDK   = /c/Android/sdk/ndk/30.0.16248370（可用环境变量覆盖）
#   源码  = /c/re-tools/ffmpeg-src/ 下的 ffmpeg-7.1.tar.{xz,gz}（可用 FFMPEG_SRC_TAR 覆盖）
# 幂等：重跑会清掉 build/ 与 dist/ 重建。
set -euo pipefail

NDK="${NDK:-/c/Android/sdk/ndk/30.0.16248370}"
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${FFMPEG_SRC_DIR:-/c/re-tools/ffmpeg-src}"
API=26

TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/windows-x86_64"
MAKE="$NDK/prebuilt/windows-x86_64/bin/make.exe"
CLANG="$TOOLCHAIN/bin/clang.exe"
CLANGXX="$TOOLCHAIN/bin/clang++.exe"
SYSROOT="$TOOLCHAIN/sysroot"

# 源码 tarball：ffmpeg.org 原包或 GitHub tag 归档（顶层目录名不同，但 --strip-components=1 已吸收）。
# `|| true`：ls 对不存在的候选路径返回非零，pipefail 会把整条管道判死——脚本会在这里静默退出。
TAR="$(ls "$SRC_DIR"/ffmpeg-7.1.tar.xz "$SRC_DIR"/ffmpeg-7.1.tar.gz "$SRC_DIR"/ffmpeg-7.1-gh.tar.gz 2>/dev/null | head -1 || true)"
[ -n "$TAR" ] || { echo "找不到 ffmpeg-7.1 源码 tarball（$SRC_DIR）"; exit 2; }
[ -x "$CLANG" ] || { echo "NDK clang 不存在：$CLANG"; exit 2; }

BUILD="$HERE/build"
DIST="$HERE/dist"
rm -rf "$BUILD" "$DIST"
mkdir -p "$BUILD" "$DIST"

# 宿主编译器：本机没有 gcc（交叉编译的 configure 要一个 C11 宿主编译器做探测），
# 用 zig cc —— 单文件自带 mingw 运行时，语义与 gcc 兼容。封装成包装脚本，
# 让 --host-cc 是单个命令词（configure 内部对 $host_cc 的引用方式不统一，
# 带空格的 "zig cc" 会在部分位置被拆词）。
ZIG_EXE="${ZIG_EXE:-$(ls /c/re-tools/zig-windows-x86_64-*/zig.exe 2>/dev/null | head -1 || true)}"
[ -n "$ZIG_EXE" ] || { echo "找不到 zig.exe（宿主编译器，装到 /c/re-tools/ 或设 ZIG_EXE）"; exit 2; }
printf '#!/usr/bin/env bash\nexec "%s" cc "$@"\n' "$ZIG_EXE" > "$BUILD/zigcc"
chmod +x "$BUILD/zigcc"

echo "== 解包 $(basename "$TAR") =="
tar -xf "$TAR" -C "$BUILD" --strip-components=1

echo "== configure（arm64-v8a / API $API / 静态库 / 只开 demux+decode+swscale）=="
cd "$BUILD"
PATH="$TOOLCHAIN/bin:$PATH" ./configure \
  --prefix="$DIST" \
  --target-os=linux --arch=aarch64 --cpu=armv8-a --enable-cross-compile \
  --sysroot="$SYSROOT" \
  --cc="$CLANG" --cxx="$CLANGXX" --ld="$CLANG" \
  --extra-cflags="--target=aarch64-linux-android$API -fPIC -O2" \
  --extra-cxxflags="--target=aarch64-linux-android$API" \
  --extra-ldflags="--target=aarch64-linux-android$API" \
  --ar="$TOOLCHAIN/bin/llvm-ar.exe" --nm="$TOOLCHAIN/bin/llvm-nm.exe" \
  --ranlib="$TOOLCHAIN/bin/llvm-ranlib.exe" --strip="$TOOLCHAIN/bin/llvm-strip.exe" \
  --host-cc="$BUILD/zigcc" \
  --disable-shared --enable-static --enable-pic \
  --disable-programs --disable-doc --disable-debug \
  --disable-avdevice --disable-postproc --disable-avfilter \
  --disable-network --disable-autodetect --enable-zlib \
  --enable-avformat --enable-avcodec --enable-swscale \
  --enable-protocol=file \
  --enable-demuxer=rm,asf,avi,flv,matroska,mov,mpegts,mpegvideo,mp3,wav,ogg,aac \
  --enable-decoder=rv10,rv20,rv30,rv40,cook,sipr,atrac3,atrac3p,wmv1,wmv2,wmv3,vc1,wmav1,wmav2,wmavoice,wmapro,wmalossless,msmpeg4v2,msmpeg4v3,mpeg4,h264,hevc,aac,mp3,flv1,vp6,mjpeg,png \
  | tail -5

echo "== make -j8 =="
PATH="$TOOLCHAIN/bin:$PATH" "$MAKE" -j8 > "$HERE/build.log" 2>&1
tail -3 "$HERE/build.log"

echo "== 安装到 dist/ =="
PATH="$TOOLCHAIN/bin:$PATH" "$MAKE" install > /dev/null 2>&1

echo "== 产物自检 =="
for lib in libavformat libavcodec libswscale libavutil; do
  f="$DIST/lib/$lib.a"
  [ -f "$f" ] || { echo "缺 $f"; exit 1; }
  echo "  $f  $(du -h "$f" | cut -f1)"
done
# 能力自证：rv10/wmv3/cook 三个符号必须在（对照 tests/real/sd211 两支片）
for sym in ff_rv10_decoder ff_wmv3_decoder ff_cook_decoder; do
  "$TOOLCHAIN/bin/llvm-nm.exe" "$DIST/lib/libavcodec.a" 2>/dev/null | grep -q "$sym" \
    || { echo "自检失败：$sym 不在 libavcodec.a"; exit 1; }
done
echo "OK：dist/ 就绪（静态库 + 头文件），LGPL 合规要求见同目录 README.md"
