#!/bin/bash
# 278 · 上屏帧率量具采集（SurfaceFlinger latency）
#
# 【为什么不用 p273_gaps.mjs】它靠 mpv render 回调计数 ⇒ 只对 render API 路径有效；
# vo=gpu-next 没有 render 回调 ⇒ 统计恒为 0（278 的 B 臂就踩到了）。
# 本脚本量的是**真正上屏**的帧（SurfaceFlinger 合成时刻）⇒ 两条出画通路同一把尺子。
#
# 【为什么不能一次 dump 就够】
#   ① 层名里的 id 每次启动都变（SurfaceView[...]#NNNN）⇒ 必须先查。
#   ② latency 是 **128 帧的环形缓冲**（60 fps 下只覆盖约 2.1 秒）⇒ 必须反复采样并合并。
#
# 【用法】
#   bash tools/p274_sf_latency.sh <标签> [秒数] [gpuNext]
#     标签     证据目录名（docs/evidence/p274-startup/<标签>）
#     秒数     播放多久后开始采样（默认 40）
#     gpuNext  传 1 ⇒ 起播走 vo=gpu-next
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
SRC_ID="aafdd622d6e1e15b27dfef460ffa95c8"
SIZE=4316184874
URL="https://emby.bangumi.ca/emby/Videos/485/stream?static=true&MediaSourceId=$SRC_ID"

LABEL="${1:-sflat}"
SECS="${2:-40}"
GPUNEXT="${3:-0}"
cd "C:/kivo video" || exit 1
OUT="docs/evidence/p274-startup/$LABEL"
mkdir -p "$OUT"

GPFLAG=""
if [ "$GPUNEXT" = "1" ]; then GPFLAG="--ez kivo.gpuNext true"; fi

$ADB shell am force-stop com.kivo.cinema
sleep 2
$ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log'"
$ADB shell "am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl '$URL' --es kivo.embyToken '$TOKEN' \
  --el kivo.playSize $SIZE --es kivo.label '$LABEL' $GPFLAG" >/dev/null 2>&1

sleep "$SECS"

# ---- 查 SurfaceView 的 (BLAST) 层 id（每次启动都变）----
LAYER_BASE='SurfaceView[com.kivo.cinema/com.kivo.cinema.ui.compose.PlayerComposeActivity](BLAST)'
LAYER_ID=$($ADB shell "dumpsys SurfaceFlinger --list 2>/dev/null" | grep -F "$LAYER_BASE" | head -1 | sed 's/.*#//' | tr -d '\r')
if [ -z "$LAYER_ID" ]; then
  echo "!! 找不到 SurfaceView (BLAST) 层（App 是否在前台？）"
  exit 1
fi
LAYER="$LAYER_BASE#$LAYER_ID"
echo "  层：$LAYER"

# ---- 反复采样并合并（环形缓冲 ⇒ 每次只覆盖最近 ~2 秒）----
: > "$OUT/latency.txt"
for i in $(seq 1 60); do
  $ADB shell "dumpsys SurfaceFlinger --latency '$LAYER'" 2>/dev/null >> "$OUT/latency.txt"
  echo "" >> "$OUT/latency.txt"
  sleep 0.5
done

$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$OUT/kernel.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$OUT/net.log" 2>/dev/null

echo "===== $LABEL @ ${SECS}s gpuNext=$GPUNEXT ====="
node tools/p274_sf_latency.mjs "$OUT/latency.txt" | sed 's/^/  /'
echo "  --- 出画路径 ---"
grep -iE "vo/gpu-next/opengl\] (Initializing|EGL_VERSION)|vo/libmpv\]|GL_VERSION|Disabling HDR peak|Initialized libplacebo" "$OUT/kernel.log" | head -5 | sed 's/^/  /'
grep -iE "出画模式" "$OUT/net.log" | tail -1 | sed 's/^/  /'
