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
CLANG="$TOOLCHAIN/bin/clang.exe"
CLANGXX="$TOOLCHAIN/bin/clang++.exe"
SYSROOT="$TOOLCHAIN/sysroot"

# 源码 tarball：ffmpeg.org 原包或 GitHub tag 归档（顶层目录名不同，但 --strip-components=1 已吸收）。
# `|| true`：ls 对不存在的候选路径返回非零，pipefail 会把整条管道判死——脚本会在这里静默退出。
TAR="$(ls "$SRC_DIR"/ffmpeg-7.1.tar.xz "$SRC_DIR"/ffmpeg-7.1.tar.gz "$SRC_DIR"/ffmpeg-7.1-gh.tar.gz 2>/dev/null | head -1 || true)"
[ -n "$TAR" ] || { echo "找不到 ffmpeg-7.1 源码 tarball（$SRC_DIR）"; exit 2; }
[ -x "$CLANG" ] || { echo "NDK clang 不存在：$CLANG"; exit 2; }

# make/sh 必须同属一个 msys 运行时：NDK 的 native make 在 spawn sh 时撞
# Windows 32K 命令行上限，libavcodec 的 ar 行（几百个 .o）**在参数进到 sh 之前
# 就被截断**——ar 层的 @response 包装救不了（它收到的参数已经是断的）。
# MSYS2 的 make+sh 内部 spawn 不经 CreateProcess，无此限制（ar 包装继续保留，
# 作为 msys→native 最后一跳的兜底）。脚本应从 MSYS2 bash 里跑：
#   /c/msys64/usr/bin/bash -lc 'cd "/c/kivo video" && bash third_party/ffmpeg-android/build.sh'
if [ -d /c/msys64/usr/bin ]; then
  export PATH="/c/msys64/usr/bin:$PATH"
fi
MAKE="$(command -v make || true)"
[ -n "$MAKE" ] || MAKE="$NDK/prebuilt/windows-x86_64/bin/make.exe"

# 构建目录**必须在无空格路径**：ffmpeg 的构建系统（生成的 config.sh / make 规则）
# 内部不做引号保护，仓库路径 "C:\kivo video" 的空格会把它打碎
# （实测：config.sh 第 4 行被空格劈成两个词、$(AR) 带空格路径无法执行）。
# 所以 BUILD/DIST 默认落在 /c/re-tools/ 下；仓库里只保留本脚本与 README。
BUILD="${FFMPEG_BUILD_DIR:-/c/re-tools/ffmpeg-android/build}"
DIST="${FFMPEG_DIST_DIR:-/c/re-tools/ffmpeg-android/dist}"
rm -rf "$BUILD" "$DIST"
mkdir -p "$BUILD" "$DIST"

# 宿主编译器：本机没有 gcc（交叉编译的 configure 要一个 C11 宿主编译器做探测），
# 用 zig cc —— 单文件自带 mingw 运行时，语义与 gcc 兼容。封装成包装脚本，
# 让 --host-cc 是单个命令词（configure 内部对 $host_cc 的引用方式不统一，
# 带空格的 "zig cc" 会在部分位置被拆词）。
ZIG_EXE="${ZIG_EXE:-$(ls /c/re-tools/zig-windows-x86_64-*/zig.exe 2>/dev/null | head -1 || true)}"
[ -n "$ZIG_EXE" ] || { echo "找不到 zig.exe（宿主编译器，装到 /c/re-tools/ 或设 ZIG_EXE）"; exit 2; }
# zig 要全局缓存目录：MSYS2 登录 shell 不带 APPDATA，zig 会报
# `error: AppDataDirUnavailable` 然后整个 configure 死在宿主编译器探测。
export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-/c/re-tools/ffmpeg-android/zig-cache}"
printf '#!/usr/bin/env bash\nexec "%s" cc "$@"\n' "$ZIG_EXE" > "$BUILD/zigcc"
chmod +x "$BUILD/zigcc"

# ar 包装：libavcodec 有几百个 .o，ar 命令行超出 Windows CreateProcess 的
# 32K 上限，**输出文件名会被截断**（实测：`llvm-ar: error: libavcode: No such
# file or directory`——libavcodec.a 尾字符没了）。把对象列表转成 @response
# 文件（llvm-ar 原生支持）绕过。
export LLVM_AR="$TOOLCHAIN/bin/llvm-ar.exe"
cat > "$BUILD/ar" << ARWRAPPER
#!/usr/bin/env bash
set -e
mode="\$1"; out="\$2"; shift 2
rsp="\$out.rsp"
{ printf '%s\n' "\$mode"; printf '%s\n' "\$out"; printf '%s\n' "\$@"; } > "\$rsp"
exec "$LLVM_AR" "@\$rsp"
ARWRAPPER
chmod +x "$BUILD/ar"

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
  --ar="$BUILD/ar" --nm="$TOOLCHAIN/bin/llvm-nm.exe" \
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
# 能力自证：rv10/wmv3/cook 三个符号必须在（对照 tests/real/sd211 两支片）。
# 不能用 `nm | grep -q`：pipefail 下 grep 提前退出会给 nm 送 SIGPIPE，
# 18MB 的符号表一匹配就死，管道按失败算——"自检失败"全是这个假象。
for sym in ff_rv10_decoder ff_wmv3_decoder ff_cook_decoder; do
  cnt=$("$TOOLCHAIN/bin/llvm-nm.exe" "$DIST/lib/libavcodec.a" 2>/dev/null | grep -c "$sym" || true)
  [ "${cnt:-0}" -ge 1 ] || { echo "自检失败：$sym 不在 libavcodec.a"; exit 1; }
done
echo "OK：dist/ 就绪（静态库 + 头文件），LGPL 合规要求见同目录 README.md"
