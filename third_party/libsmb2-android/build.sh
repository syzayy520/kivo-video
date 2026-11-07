#!/usr/bin/env bash
# libsmb2（SenPlayer 同款 SMB2/3 客户端库，220 设计稿 D1）NDK 交叉编译。
#
# 用法（Git Bash）：bash third_party/libsmb2-android/build.sh [arm64]
# 产物：build/libsmb2-android/arm64/liblibsmb2.a（静态库，给 app 的 CMake 链接）
#
# 来源与许可：github.com/sahlberg/libsmb2（LGPL-2.1，动态/静态链接均需保留声明，
# 详见 third_party/libsmb2-src/COPYING）。第三方目录只放源码（git clone，不入库二进制）。
#
# 七坑记录（与 ffmpeg-android/build.sh 同风格，踩过的坑写在这里）：
# 1. NDK 工具链：用 C:/Android/sdk/ndk/27.2.12479018（AGP 9 自带 30.0 也可）。
# 2. 可选依赖全关：ENABLE_LIBKRB5/GSSAPI/LIBDCERPC/LIBQUIC/RDMA=OFF——
#    它们默认 ON，configure 会因缺 krb5/gssapi 头文件直接死。
# 3. TLS 不启用（ENABLE_OPENSSL 默认 OFF）→ 走库内置加密（md4/rc4/aes/sha），
#    NTLMv2 与 SMB3 加密都够用；本卡目标路由器也不提供加密能力（capabilities=0x5）。
# 4. BUILD_SHARED_LIBS=OFF：静态库进 kivo_player，不额外加 .so（LGPL 静态链接需
#    保留源码可得性声明，README 记录来源 tag/commit）。
# 5. 端口：libsmb2 的 server 参数支持 "host:port"，缺省 445（lib/socket.c）。
# 6. Ninja 路径：用 gradle 自带的（android/.gradle 之外，PATH 里 nvidia...）——
#    实际上直接用 cmake --build（内部调 ninja）最稳。
# 7. android-26 对齐 minSdk；再低会缺 poll/getaddrinfo 某些 flag。

set -e
NDK="C:/Android/sdk/ndk/27.2.12479018"
SRC="$(cd "$(dirname "$0")/../libsmb2-src" && pwd)"
OUT="build/libsmb2-android/arm64"

cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_LIBKRB5=OFF \
  -DENABLE_GSSAPI=OFF \
  -DENABLE_LIBDCERPC=OFF \
  -DENABLE_LIBQUIC=OFF \
  -DENABLE_RDMA=OFF \
  -DENABLE_EXAMPLES=OFF \
  -DENABLE_UTILS=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -S "$SRC" -B "$OUT"

cmake --build "$OUT" -j4
echo "== liblibsmb2.a =="
ls -la "$OUT/liblibsmb2.a"
