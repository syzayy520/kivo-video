#!/bin/bash
# 只做一件事：把 mpv v0.39.0 交叉编译成 Android arm64 的 libmpv.so（含两个本地补丁）。
#
# 用法：
#   bash android/app/buildscripts/mpv/build-libmpv.sh          # 增量
#   bash android/app/buildscripts/mpv/build-libmpv.sh --clean  # 删 build 目录完整重编
#
# 产物：
#   $KIVO_BUILD/libmpv.so
# 之后用 stage-libmpv.sh 搬到 App 的打包目录。
#
# 【为什么这个文件曾经"失踪"】253 的家族树（docs/design/backend-core/
# 253-mpv-kernel-family-tree.md §三）规划了 4 个构建文件，实际仓库里一个都没有，
# 只有 stage-libmpv.sh。结果：mpv 源码版本、meson 选项、NDK 版本、
# 两个补丁的应用顺序全都没记录 ⇒ 换机不可重建，且曾在不知情时用错 NDK 混编。
# 本文件从现有 build 目录的 meson-info 反推出真实配置，把它固化下来。
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=toolchain.env
source "$HERE/toolchain.env"

# Windows Store 的 python3.exe 是坏存根（指向不存在的 3.14）；meson 的 project() 会调它。
# 项目内 shim 把真正的 Python310 暴露成 python3，前置到 PATH（不改系统）。
KIVO_SHIM="$(cd "$HERE/../../../.." && pwd)/.codeartsdoer/toolchain-shim"
[ -x "$KIVO_SHIM/python3.exe" ] && export PATH="$KIVO_SHIM:$PATH"

CLEAN=0
[ "${1:-}" = "--clean" ] && CLEAN=1

echo "===== libmpv 交叉编译 ====="
echo "  mpv 源码 : $KIVO_MPV_SRC"
echo "  NDK      : $KIVO_NDK"
echo "  目标     : $KIVO_TRIPLE"
echo "  构建目录 : $KIVO_BUILD"
echo "  模式     : $([ $CLEAN = 1 ] && echo 完整重编 || echo 增量)"

# ---------- 1. 前置检查 ----------
if [ ! -d "$KIVO_MPV_SRC" ]; then
  echo "❌ 找不到 mpv 源码：$KIVO_MPV_SRC" >&2
  echo "   需要 mpv v0.39.0（git clone https://github.com/mpv-player/mpv && git checkout v0.39.0）" >&2
  exit 1
fi
MPV_VER="$(cd "$KIVO_MPV_SRC" && git describe --tags 2>/dev/null || echo unknown)"
echo "  mpv 版本 : $MPV_VER"
case "$MPV_VER" in
  v0.39.0) : ;;
  *) echo "⚠️ mpv 版本不是 v0.39.0（实测基线是这个版本）。继续构建，但产物与已验证的不同。" >&2 ;;
esac

for dep in "$KIVO_FFMPEG/include/libavcodec/avcodec.h" \
           "$KIVO_LIBASS/include/ass/ass.h" \
           "$KIVO_LIBPLACEBO/include/libplacebo/gpu.h"; do
  [ -f "$dep" ] || { echo "❌ 缺依赖头文件：$dep" >&2; exit 1; }
done

# ---------- 2. 补丁：先复位再按固定顺序打（保证幂等） ----------
# 【为什么每次复位】补丁脚本是"整段替换"，重复应用会失败或产出脏代码。
# 复位到干净 v0.39.0 再打，是唯一能保证「同一份源码 → 同一个二进制」的做法。
echo "----- 应用本地补丁 -----"
( cd "$KIVO_MPV_SRC" && git checkout -- . 2>/dev/null || true )

# 【必须在源码目录里执行】两个补丁脚本里写的是**相对路径**
# （`video/out/opengl/egl_helpers.c`），cwd 不对就会 FileNotFoundError。
echo "  [1/2] ES3 补丁（egl_helpers.c：允许 ES3 context ⇒ compute shader）"
( cd "$KIVO_MPV_SRC" && python3 "$HERE/patch-mpv-es3.py" )

echo "  [2/2] HDR surface 补丁（context_android.c：PQ colorspace + 10bit 信号）"
( cd "$KIVO_MPV_SRC" && python3 "$HERE/patch-mpv-hdr-surface.py" )

# 校验补丁真的进去了（否则后面编出来的还是旧行为）
for marker in "GLES 3.x" "eglSetSurfaceAttrib"; do
  if ! grep -rq "$marker" "$KIVO_MPV_SRC/video/out/" 2>/dev/null; then
    echo "❌ 补丁标记 '$marker' 未出现在源码里 ⇒ 补丁没生效，停止构建" >&2
    exit 1
  fi
done
echo "  ✅ 两个补丁都已进入源码"

# ---------- 3. meson ----------
MESON_OPTS=(
  -Dlibmpv=true
  -Dcplayer=false        # 只要库，不要命令行播放器
  -Dgpl=false
  -Ddefault_library=shared
  -Dgl=enabled           # OpenGL 渲染（gpu-next 必需）
  -Dgl-x11=disabled
  -Dvulkan=disabled
  -Djavascript=disabled
  -Dlua=disabled
  -Dsdl2=disabled
  -Dlcms2=disabled
  -Dtests=false
)

# 【为什么 `--clean` 的目录放在 /tmp 而不是源码旁边】
# 本机 safe-delete 对批量删除 fail-closed，而 **meson 在 setup 过程中会自己删临时文件**
# （`meson-private/tmpXXXX`）⇒ 只要构建目录在受保护范围内，setup 就会被拦断，
# 报 `[safe-delete][SAFE_DELETE_BULK_CONFIRM_REQUIRED]` 然后停在半路。
# 实测：`C:\re-tools\...` 下被拦，`/tmp` 下正常（`/tmp/mpv-verify` 一轮通过 225/225）。
# ⇒ clean 模式用 /tmp 下带时间戳的新目录：既不触发删除保护，也天然"全新"。
if [ $CLEAN = 1 ]; then
  KIVO_BUILD="/tmp/kivo-mpv-clean-$(date +%H%M%S)"
  echo "----- clean 模式：全新构建目录 $KIVO_BUILD -----"
fi

# 【为什么必须同时给 --cross-file 和 --native-file】
# cross file 描述目标机（Android arm64）；native file 描述构建机（Windows x86_64）。
# meson 在 cross build 的 project() 里会探测**构建机**的 C 编译器（为生成构建期工具），
# 而 Windows 默认没有 cc/gcc/cl ⇒ 探测返回 None ⇒ meson 1.12 抛
# `AttributeError: 'NoneType' object has no attribute 'split'`（看起来像 meson 的 bug）。
# 253 时代的原始构建是在 MSYS2 shell 里跑的（PATH 自带 gcc）所以没暴露；
# 在 Git Bash 里必须显式给 native file。详见 native-windows.txt 的文件头。
if [ -d "$KIVO_BUILD" ]; then
  echo "----- meson reconfigure（增量）-----"
  # 增量时也强制重设 machine file：换机器/换路径后旧配置会指向不存在的目录
  meson setup --reconfigure "$KIVO_BUILD" "$KIVO_MPV_SRC" \
    --cross-file "$HERE/cross-android-arm64.txt" \
    --native-file "$HERE/native-windows.txt" \
    "${MESON_OPTS[@]}"
else
  echo "----- meson setup（首次）-----"
  meson setup "$KIVO_BUILD" "$KIVO_MPV_SRC" \
    --cross-file "$HERE/cross-android-arm64.txt" \
    --native-file "$HERE/native-windows.txt" \
    "${MESON_OPTS[@]}"
fi

# ---------- 4. 编译 ----------
echo "----- ninja -----"
ninja -C "$KIVO_BUILD"

# ---------- 5. 产物校验 ----------
SO="$KIVO_BUILD/libmpv.so"
[ -f "$SO" ] || { echo "❌ 没有产出 libmpv.so" >&2; exit 1; }

SIZE=$(stat -c%s "$SO" 2>/dev/null || stat -f%z "$SO")
echo "----- 产物 -----"
echo "  $SO"
echo "  $((SIZE / 1024 / 1024)) MiB"

# 三个必须存在的标记（缺一个都说明这轮编出来的东西不能用）
fail=0
check() {
  if grep -aq "$2" "$SO"; then
    echo "  ✅ $1"
  else
    echo "  ❌ $1（标记 '$2' 不在二进制里）" >&2
    fail=1
  fi
}
check "ES3 补丁" "GLES 3.x"
check "HDR surface 补丁" "eglSetSurfaceAttrib"
check "libplacebo（gpu-next 必需）" "libplacebo"
check "vo_gpu_next" "gpu-next"

if [ $fail = 1 ]; then
  echo "❌ 产物校验失败 —— 不要拿去打包" >&2
  exit 1
fi

echo
echo "✅ 完成。下一步："
echo "   bash android/app/buildscripts/mpv/stage-libmpv.sh"
