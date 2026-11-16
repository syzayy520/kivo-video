#!/bin/bash
# p274_soak.sh · 288 修订（289 竣工）· 公网两臂长稳「终判」采集器（硬解口径 / gpu-next）
#
# 【职责】一臂一次调用：起播 → 等上屏层就绪 → 全程 SurfaceFlinger 上屏帧采样
#   → 收尾抓 kernel/net/diag + 前台校验 + 截图 → 一次性出全部判据。
#   两臂 = 本脚本调两次（不同标签 + keepalive），见 279 §九。
#
# 【为什么不再用 p273_gaps.mjs】它靠 mpv render 回调数渲染帧，
#   vo=gpu-next 没有 render 回调 ⇒ 恒为 0（278 B 臂已踩）。且旧版 `sleep SECS`
#   中间完全不采样、末尾才 dump 一次 ⇒ 覆盖不到长窗口中的卡顿。
#   本版用 p274_sf_latency.mjs 读**真正上屏**帧，全程反复采样拼接去重。
#   （p273_gaps.mjs 仍被 8 个 legacy/实验脚本引用，保留文件，只是本脚本不再调用。）
#
# 【为什么全程零截图】287 教训：长采样窗口中途 screencap（4K）会抢占
#   SurfaceFlinger 合成，造成 ~1.6s 假卡顿。截图一律放到采样结束之后。
#
# 【用法】
#   bash tools/p274_soak.sh <标签> <秒数> [keepalive] [gpuNext]
#     标签       证据目录名（docs/evidence/p274-startup/<标签>）
#     秒数       **纯净采样窗口**时长（≥240 才能下卡顿结论，铁律）
#     keepalive  1 ⇒ 置 debug.kivo.originKeepAlive=1（复用连接/回退臂）；0=每块新连接
#     gpuNext    1 ⇒ 显式 --ez kivo.gpuNext true；默认本就是 gpu-next，0 仅用于 legacy 对照
#
# 【本地装置彩排 / 自定义片源】用环境变量覆盖（默认公网 Emby 485 1080p）：
#   KIVO_URL=http://127.0.0.1:8131/v KIVO_SIZE=22494506514 KIVO_TOKEN= \
#     bash tools/p274_soak.sh <标签> <秒数> 0 1
#
# 【291/292 · 开关】默认三个修复都开；置 off 即为对照臂：
#   KIVO_STALL=off   关单块吞吐看门狗
#   KIVO_SUPPLY=off  关供数压力闸门
#   KIVO_REBUFFER=off 关自适应 rebuffer 闸门（292 起播爬坡）
#   例：KIVO_STALL=off KIVO_SUPPLY=off KIVO_REBUFFER=off bash tools/p274_soak.sh base-300 300 0 1
#
# 【采集前必须清空日志】不清空 = 混入上一轮，对照表作废（§26.7）。
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
PKG=com.kivo.cinema
ACT="$PKG/.ui.compose.PlayerComposeActivity"

# ---- 291：测试臂属性必须在脚本退出时复位（含 Ctrl-C / 中途失败）----
# 【为什么】290 B 臂把 debug.kivo.originKeepAlive 置 1 后没复位，属性留在设备上
#   污染了用户的**真实播放会话**（表现为「卡得一动不动」）。系统属性是进程级
#   全局状态，force-stop / 重装都不会清 ⇒ 必须显式复位。用 trap 覆盖所有退出路径。
cleanup_props() {
  $ADB shell "setprop debug.kivo.originKeepAlive 0" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.stallWatchdog on" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.supplyPressure on" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.rebuffer off" >/dev/null 2>&1
}
trap cleanup_props EXIT INT TERM

# ---- 片源（默认公网 485 的 1080p；可用环境变量覆盖做本地稳定源彩排）----
SRC_ID="aafdd622d6e1e15b27dfef460ffa95c8"
URL="${KIVO_URL:-https://emby.bangumi.ca/emby/Videos/485/stream?static=true&MediaSourceId=$SRC_ID}"
SIZE="${KIVO_SIZE:-4316184874}"
TOKEN_DEFAULT="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt' 2>/dev/null)"
TOKEN="${KIVO_TOKEN-$TOKEN_DEFAULT}"

LABEL="${1:-soak}"
SECS="${2:-300}"
KEEPALIVE="${3:-0}"
GPUNEXT="${4:-1}"
cd "C:/kivo video" || exit 1
OUT="docs/evidence/p274-startup/$LABEL"
mkdir -p "$OUT"

# ---- 起播 ----
# 291/292：三个修复开关默认 on；置 off 即为 A/B 对照臂（一次装包跑多臂归因）。
$ADB shell "setprop debug.kivo.originKeepAlive $KEEPALIVE"
$ADB shell "setprop debug.kivo.stallWatchdog ${KIVO_STALL:-on}"
$ADB shell "setprop debug.kivo.supplyPressure ${KIVO_SUPPLY:-on}"
$ADB shell "setprop debug.kivo.rebuffer ${KIVO_REBUFFER:-off}"
$ADB shell am force-stop "$PKG"
sleep 2
$ADB shell "run-as $PKG sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'"
GPFLAG=""
if [ "$GPUNEXT" = "1" ]; then GPFLAG="--ez kivo.gpuNext true"; fi
TOKENFLAG=""
if [ -n "$TOKEN" ]; then TOKENFLAG="--es kivo.embyToken '$TOKEN'"; fi
$ADB shell "am start -n $ACT --es kivo.playurl '$URL' $TOKENFLAG \
  --el kivo.playSize $SIZE --es kivo.label '$LABEL' $GPFLAG" >/dev/null 2>&1
echo "T0=$(date '+%Y-%m-%d %H:%M:%S') keepalive=$KEEPALIVE gpuNext=$GPUNEXT secs=$SECS url=$URL"

# ---- 等 (BLAST) 上屏层就绪（每次启动 id 都变，带 30s 重试）----
LAYER_BASE="SurfaceView[$PKG/$PKG.ui.compose.PlayerComposeActivity](BLAST)"
LAYER=""
for w in $(seq 1 30); do
  LID=$($ADB shell "dumpsys SurfaceFlinger --list 2>/dev/null" | grep -F "$LAYER_BASE" | head -1 | sed 's/.*#//' | tr -d '\r')
  if [ -n "$LID" ]; then LAYER="$LAYER_BASE#$LID"; break; fi
  sleep 1
done
if [ -z "$LAYER" ]; then
  echo "!! 30s 内找不到 (BLAST) 上屏层：App 是否在前台？片源是否打开失败？"
  echo "!! 本臂作废，不产出半成品 latency（不许静默降级）。"
  exit 1
fi
echo "  层就绪：$LAYER"

# ---- 纯净采样窗口：wall-clock 覆盖 SECS，每 1.2s dump 一次，全程零截图 ----
# latency 是 128 帧环形缓冲（60fps 约 2.1s）⇒ 采样间隔必须 **< 2.1s**，
# 否则两次 dump 之间漏采的帧会被旧量具算成「大 gap」假卡顿（289 彩排实证：
# 1.8s 标称 + adb 抖动 ⇒ 实测间隔最大 2.97s，B 臂 13/18 次超 2.1s）。
# 收紧到 1.2s 留足余量；★ 采样期间**绝不可并发其它 adb 命令**（会争抢致间隔抖动）。
# 中途绝不截图（287 教训：4K screencap 抢占合成造成 ~1.6s 假卡顿）。
: > "$OUT/latency.txt"
T_SAMPLE_START=$(date +%s)
T_END=$(( T_SAMPLE_START + SECS ))
N=0
while [ "$(date +%s)" -lt "$T_END" ]; do
  $ADB shell "dumpsys SurfaceFlinger --latency '$LAYER'" 2>/dev/null >> "$OUT/latency.txt"
  echo "" >> "$OUT/latency.txt"
  N=$((N+1))
  sleep 1.2
done
echo "  采样 $N 次 / 实际 $(( $(date +%s) - T_SAMPLE_START ))s / 目标 ${SECS}s（全程零截图、不并发 adb）"

# ---- 收尾取证（采样已结束；截图放最后，绝不干扰上屏帧统计）----
echo "  前台："
$ADB shell "dumpsys activity activities 2>/dev/null | grep -i 'topResumedActivity' | head -1" | sed 's/^/    /'
$ADB exec-out screencap -p > "$OUT/playing.png" 2>/dev/null
$ADB exec-out "run-as $PKG cat files/net-diag.log" > "$OUT/net.log" 2>/dev/null
$ADB exec-out "run-as $PKG cat files/mpv-kernel.log" > "$OUT/kernel.log" 2>/dev/null
$ADB exec-out "run-as $PKG cat files/mpv-kernel.log.diag" > "$OUT/diag.log" 2>/dev/null

echo "===== $LABEL @ ${SECS}s keepalive=$KEEPALIVE gpuNext=$GPUNEXT ====="
echo "--- ① 上屏帧率/卡顿（sf_latency，权威判据）---"
node tools/p274_sf_latency.mjs "$OUT/latency.txt" 2>&1 | sed 's/^/  /'
echo "--- ② 口径确认：必须硬解 + gpu-next（否则本臂不算硬解口径）---"
grep -iE "Using hardware decoding|software decoding|Unsupported hwdec|Loading hwdec driver|VO: \[gpu-next\]|MediaCodec started|Decoder format|Initialized libplacebo" "$OUT/kernel.log" \
  | grep -viE "GL_|EGL_|extension" | head -8 | sed 's/^/  /'
echo "--- ③ 内核全程事件（连续日志，不受采样窗口限制）---"
for P in "Enter buffering" "End buffering" "Audio device underrun" "Render queue underrun" "acquireLatestImage" "30001" "Waiting for frame" "Video frame dropped"; do
  printf "  %-28s : %s\n" "$P" "$(grep -ci "$P" "$OUT/kernel.log")"
done
echo "--- ④ 首帧（绝对时刻）---"
grep "首帧就绪" "$OUT/net.log" | sed 's/^/  /'
echo "--- ⑤ 出画模式（net-diag）---"
grep -iE "出画模式|renderMode" "$OUT/net.log" | tail -2 | sed 's/^/  /'
echo "--- ⑥ 供给（单次请求字节数+耗时，因果判据，方法论 6）---"
grep -iE "origin chunk|REQ#|rate=|chunk|单次" "$OUT/net.log" | tail -8 | sed 's/^/  /'
echo "  证据：$OUT/{latency.txt,kernel.log,net.log,diag.log,playing.png}"
echo "  注：脚本退出时自动复位 originKeepAlive=0 / stallWatchdog=on / supplyPressure=on"
