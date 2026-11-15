#!/bin/bash
# 282 · 帧插值 A/B 实测（同一片源、同一台设备、同一时段）
#
# 【为什么要 A/B 而不是"开了看卡不卡"】
# 插值的收益是**帧数变多**，代价是**GPU 开销变高**。只看"卡不卡"会漏掉
# "帧数根本没涨"这种情况（开启后 video-sync 生效但 tscale 跑了，上屏还是 25fps
# ⇒ 等于白白多花 GPU）。所以判据是两条：
#   ① 上屏帧率 **明显高于片源帧率**（说明插值真的在做）
#   ② 上屏帧率 **不低于关闭时的 98%**（说明 GPU 扛得住）
#
# 【用量具】SurfaceFlinger --latency（真正上屏的帧），层名必须带 (BLAST)#id。
# 该 id 每次启动都变 ⇒ 每次重新查。
#
# 【用法】
#   bash tools/p282_interp_ab.sh [片源URL] [token] [秒数]
# 默认片源 = Emby 上那部 4K HDR 25fps 剧集（252137）。
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
PKG=com.kivo.cinema
ACT="$PKG/.ui.compose.PlayerComposeActivity"

URL="${1:-https://emby.bangumi.ca/emby/Videos/252137/stream?static=true}"
TOKEN="${2:-$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt' 2>/dev/null)}"
SECS="${3:-40}"

cd "C:/kivo video" || exit 1
OUT="docs/evidence/p282-interp"
mkdir -p "$OUT"

run_arm() {
  local label="$1" interp="$2"
  echo "=== 臂 $label （interpolation=$interp） ==="
  $ADB shell am force-stop $PKG
  sleep 2
  $ADB shell "run-as $PKG sh -c ': > files/net-diag.log; : > files/mpv-kernel.log'" 2>/dev/null
  $ADB shell "am start -n $ACT --es kivo.playurl '$URL' --es kivo.embyToken '$TOKEN' \
    --el kivo.playSize 3000000000 --ei kivo.interpolation $interp" >/dev/null 2>&1
  sleep 18
  local layer
  layer=$($ADB shell "dumpsys SurfaceFlinger --list" | grep -F "SurfaceView[$PKG" \
    | grep -F "(BLAST)" | head -1 | tr -d '\r')
  if [ -z "$layer" ]; then echo "!! 找不到上屏层（App 在前台吗？）"; return 1; fi
  echo "  层：$layer"
  : > "$OUT/$label.raw"
  for _ in $(seq 1 "$SECS"); do
    $ADB shell "dumpsys SurfaceFlinger --latency '$layer'" >> "$OUT/$label.raw" 2>/dev/null
    echo "" >> "$OUT/$label.raw"
    sleep 1
  done
  node tools/p274_sf_latency.mjs "docs/evidence/p282-interp/$label.raw"
  echo "  内核侧回显："
  $ADB shell "run-as $PKG sh -c 'grep -m1 \"\\[interp\\]\" files/net-diag.log'" 2>/dev/null
}

run_arm "off" 0
run_arm "on"  1

echo
echo "判据：on 臂帧率应明显 > 片源帧率（证明插值生效），且不低于 off 臂的 98%（证明不卡）。"
