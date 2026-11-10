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
# 用法：bash android/app/buildscripts/mpv/stage-libmpv.sh
set -e

ROOT=/c/re-tools
PREBUILT_SRC="$ROOT/mpv-build/mpv/build/libmpv.so"
FFMPEG_LIB="$ROOT/ffmpeg-shared-dist/lib"
FFMPEG_INCLUDE="$ROOT/ffmpeg-shared-dist/include"
STAGE_CPP="C:/kivo video/android/app/src/main/cpp/mpv/prebuilt/arm64-v8a"
STAGE_JNI="C:/kivo video/android/app/src/main/jniLibs/arm64-v8a"

if [ ! -f "$PREBUILT_SRC" ]; then
  echo "缺少 libmpv.so：先跑 build-libmpv.sh" >&2
  exit 1
fi

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
