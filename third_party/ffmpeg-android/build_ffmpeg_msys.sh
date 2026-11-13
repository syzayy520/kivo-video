#!/bin/bash
set -e
NDK=/c/Android/sdk/ndk/30.0.16248370
TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/windows-x86_64"
SRC=/c/re-tools/ffmpeg-src/FFmpeg-n7.1
BUILD=/c/re-tools/ffmpeg-shared-build
PREFIX=/c/re-tools/ffmpeg-shared-dist
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$TOOLCHAIN/bin:$PATH"
export ANDROID_NDK_ROOT="$NDK"
mkdir -p "$BUILD" "$PREFIX" /c/re-tools/ffmpeg-tmp
export FFMPEG_TMPDIR=/c/re-tools/ffmpeg-tmp
# clang.exe 是 Windows 程序，不认 MSYS 的 /tmp/... 路径 ⇒ 临时目录必须在 Windows 可见处
export TMPDIR=/c/re-tools/ffmpeg-tmp
cd "$BUILD"
"$SRC/configure" \
  --prefix="$PREFIX" --target-os=android --arch=aarch64 --cpu=armv8-a \
  --enable-cross-compile \
  --sysroot="$TOOLCHAIN/sysroot" \
  --cc="$TOOLCHAIN/bin/clang.exe" --cxx="$TOOLCHAIN/bin/clang++.exe" \
  --ld="$TOOLCHAIN/bin/clang.exe" \
  --ar="$TOOLCHAIN/bin/llvm-ar.exe" --nm="$TOOLCHAIN/bin/llvm-nm.exe" \
  --ranlib="$TOOLCHAIN/bin/llvm-ranlib.exe" --strip="$TOOLCHAIN/bin/llvm-strip.exe" \
  --host-cc=/c/msys64/mingw64/bin/gcc.exe \
  --extra-cflags="--target=aarch64-linux-android26 -fPIC -O2" \
  --extra-cxxflags="--target=aarch64-linux-android26" \
  --extra-ldflags="--target=aarch64-linux-android26" \
  --enable-shared --disable-static --enable-pic \
  --disable-programs --disable-doc --disable-debug \
  --disable-avdevice --disable-postproc --disable-autodetect --disable-symver \
  --enable-network --enable-zlib \
  --enable-avformat --enable-avcodec --enable-avfilter --enable-swscale --enable-swresample \
  --enable-protocol=file --enable-protocol=tcp --enable-protocol=http --enable-protocol=https \
  --enable-demuxer=matroska --enable-demuxer=mov --enable-demuxer=mpegts --enable-demuxer=hls \
  --enable-demuxer=rm --enable-demuxer=asf --enable-demuxer=avi --enable-demuxer=flv \
  --enable-demuxer=wav --enable-demuxer=aac --enable-demuxer=ogg --enable-demuxer=mp3 \
  --enable-decoder=h264 --enable-decoder=hevc --enable-decoder=vc1 --enable-decoder=mpeg4 \
  --enable-decoder=vp9 --enable-decoder=truehd --enable-decoder=mlp --enable-decoder=ac3 \
  --enable-decoder=eac3 --enable-decoder=dts --enable-decoder=aac --enable-decoder=flac \
  --enable-decoder=mp3 --enable-decoder=pcm_s16le --enable-decoder=pcm_s24le \
  --enable-decoder=pcm_s32le --enable-decoder=pcm_f32le --enable-decoder=pcm_bluray \
  --enable-parser=h264 --enable-parser=hevc --enable-parser=mpeg4 --enable-parser=aac \
  --enable-parser=vc1 --enable-encoder=png --enable-encoder=mjpeg \
  --enable-muxer=matroska --enable-muxer=mov --enable-muxer=mpegts --enable-muxer=hls \
  --enable-jni \
  --enable-mediacodec
echo "=== configure ok ==="
make -j8
make install
echo "=== 产物 ==="
ls -la "$PREFIX/lib/"
