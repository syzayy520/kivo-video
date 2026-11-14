#!/bin/bash
# 253-P1：把交叉编译出的 libmpv 及其 FFmpeg 依赖，整理到 App 的打包目录。
#
# 【为什么不入库】
# libmpv.so 有 33 MB，加上 libav* 共约 22 MB。产物由脚本生成，
# 版本由 mpv-android/mpv 的版本号 + 我们自己的 configure 参数决定，
# 入库会带来二进制冲突与仓库膨胀。仓库里只留**构建脚本**与版本记录。
#
# 【产物落到哪】
#   android/app/src/main/cpp/mpv/prebuilt/arm64-v8a/   ← CMake 的 IMPORTED 库来源
#   android/app/src/main/jniLibs/arm64-v8a/             ← 随 APK 打包的运行期 .so
#
# 用法：
#   bash android/app/buildscripts/mpv/stage-libmpv.sh                       # 用默认构建目录
#   bash android/app/buildscripts/mpv/stage-libmpv.sh /tmp/xxx/libmpv.so    # 指定产物
#
# 【为什么要能指定产物】`build-libmpv.sh --clean` 会把产物编到 /tmp 下的新目录
# （受 safe-delete 保护的范围外，见 build-libmpv.sh 的说明）⇒ 这里必须能接住。
set -e

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=toolchain.env
source "$HERE/toolchain.env"

PREBUILT_SRC="${1:-$KIVO_BUILD/libmpv.so}"
FFMPEG_LIB="$KIVO_FFMPEG/lib"
FFMPEG_INCLUDE="$KIVO_FFMPEG/include"
STAGE_CPP="C:/kivo video/android/app/src/main/cpp/mpv/prebuilt/arm64-v8a"
STAGE_JNI="C:/kivo video/android/app/src/main/jniLibs/arm64-v8a"

if [ ! -f "$PREBUILT_SRC" ]; then
  echo "缺少 libmpv.so：$PREBUILT_SRC" >&2
  echo "先跑 build-libmpv.sh（或把产物路径作为第一个参数传进来）" >&2
  exit 1
fi
echo "源：$PREBUILT_SRC（$(stat -c%s "$PREBUILT_SRC" 2>/dev/null || stat -f%z "$PREBUILT_SRC") 字节）"

mkdir -p "$STAGE_CPP" "$STAGE_JNI"

# libmpv 本体
cp "$PREBUILT_SRC" "$STAGE_CPP/libmpv.so"
cp "$PREBUILT_SRC" "$STAGE_JNI/libmpv.so"

# FFmpeg 动态库（libmpv 的 NEEDED）
for so in libavcodec.so libavfilter.so libavformat.so libavutil.so \
           libswresample.so libswscale.so; do
  if [ -f "$FFMPEG_LIB/$so" ]; then
    cp "$FFMPEG_LIB/$so" "$STAGE_CPP/$so"
    cp "$FFMPEG_LIB/$so" "$STAGE_JNI/$so"
  else
    echo "缺少 $so" >&2
    exit 1
  fi
done

# FFmpeg 的头文件（只拷我们真正用到的几个）：
#   libavcodec/jni.h —— mpv_java_vm.cpp 用它调 av_jni_set_java_vm 注册 JavaVM。
# 【为什么不拷整个 include 树】那是几千个文件，纯属噪音；
# 我们只依赖 JNI 注册这一个接口，别的都不直接调。
mkdir -p "$STAGE_CPP/include/libavcodec"
cp "$FFMPEG_INCLUDE/libavcodec/jni.h" "$STAGE_CPP/include/libavcodec/jni.h"

echo "=== cpp/prebuilt ==="
ls -l "$STAGE_CPP"
echo "=== jniLibs ==="
ls -l "$STAGE_JNI"

# ---------------------------------------------------------------------------
# 补丁由 build-libmpv.sh 在**编译前**应用（不是这里）。
#
# 【为什么挪走】补丁改的是 mpv 的 C 源码，必须在 meson/ninja **之前**生效；
# 放在 stage（只做搬运）这一步里毫无意义，反而让人误以为"搬一下就有补丁"。
# 现在 build-libmpv.sh 会：复位源码 → 打 ES3 补丁 → 打 HDR surface 补丁 →
# 校验补丁标记 → 编译 → 校验产物里的标记。任何一步不过就中止。
#
# 【历史坑，记录一下】
# 253 时代这里有句"现代 NDK 没有 libpthread.a ⇒ 要造空归档占位"。
# 2026-11-15 用 NDK 30.0.16248370 + meson 1.12.1 完整重编（225/225）**没有踩到**：
# meson 用的是 .pc 的 `Libs:`（-lavcodec），不是 `Libs.private:`（-pthread），
# 所以链接时根本没传 -lpthread。⇒ 该坑在现配置下不存在，不需要占位文件。
#
# 【别忘了两个地方都要换】真正随 APK 打包的是 src/main/jniLibs/，
# cpp/mpv/prebuilt/ 只是 CMake 的 IMPORTED 来源 —— 本脚本两处都拷 ✓。
