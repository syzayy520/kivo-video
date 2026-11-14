#!/bin/bash
# 274 · 起播/长稳真机采集（Emby 485 的 1080p 版本）
#
# 【为什么固定片源】485 有两份：2160p（54.5 Mbps）与 1080p（4.1 Mbps）。
# 两份都在同一个 item 下，只有 1080p 可能直传流畅（见 273 设计稿 §二十八）。
#
# 【用法】
#   bash tools/p274_soak.sh <标签> <秒数> [keepalive] [gpuNext]
#     标签      证据目录名（docs/evidence/p274-startup/<标签>）
#     秒数      起播后采集多久
#     keepalive 传 1 ⇒ 置 debug.kivo.originKeepAlive=1（旧行为/回退臂）
#     gpuNext   传 1 ⇒ 起播走 **vo=gpu-next** 出画（277 的新路径：完整 libplacebo
#                渲染器 + ES3 context + aimagereader 零拷贝硬解）
#                传 0 ⇒ 走现有 render API 路径（legacy RA + mediacodec）
#
# 【采集前必须清空日志】不清空 = 混入上一轮记录，对照表直接作废（§26.7 教训）。
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
# 485 的 1080p 版本（2160p 是 f8d2f0a0…，54.5 Mbps，不可能直传）
SRC_ID="aafdd622d6e1e15b27dfef460ffa95c8"
SIZE=4316184874
URL="https://emby.bangumi.ca/emby/Videos/485/stream?static=true&MediaSourceId=$SRC_ID"

LABEL="${1:-soak}"
SECS="${2:-300}"
KEEPALIVE="${3:-0}"
GPUNEXT="${4:-0}"
cd "C:/kivo video" || exit 1
OUT="docs/evidence/p274-startup/$LABEL"
mkdir -p "$OUT"

$ADB shell "setprop debug.kivo.originKeepAlive $KEEPALIVE"
$ADB shell am force-stop com.kivo.cinema
sleep 2
$ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'"
echo "T0=$(python -c 'import time;print(int(time.time()*1000))') keepalive=$KEEPALIVE gpuNext=$GPUNEXT"
GPFLAG=""
if [ "$GPUNEXT" = "1" ]; then GPFLAG="--ez kivo.gpuNext true"; fi
$ADB shell "am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl '$URL' --es kivo.embyToken '$TOKEN' \
  --el kivo.playSize $SIZE --es kivo.label '$LABEL' $GPFLAG" >/dev/null 2>&1
sleep "$SECS"

$ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$OUT/net.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$OUT/kernel.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$OUT/diag.log" 2>/dev/null

echo "===== $LABEL @ ${SECS}s keepalive=$KEEPALIVE gpuNext=$GPUNEXT ====="
node tools/p273_gaps.mjs "$OUT/kernel.log" "$OUT/diag.log" 2>&1 | sed 's/^/  /'
echo "  --- 首帧（绝对时刻）---"
grep "首帧就绪" "$OUT/net.log" | sed 's/^/  /'
echo "  --- 供给（每块表头耗时）---"
grep -E "origin chunk" "$OUT/net.log" | sed 's/^[0-9]* //' | tail -5 | sed 's/^/  /'

echo "  --- 出画模式 / VO / GL ---"
grep -iE "出画模式|vo/gpu-next|vo/libmpv|GL_VERSION|Detected GLES|Disabling HDR peak|Initialized libplacebo" "$OUT/kernel.log" | tail -6 | sed 's/^/  /'
grep -iE "出画模式" "$OUT/net.log" | tail -2 | sed 's/^/  /'
