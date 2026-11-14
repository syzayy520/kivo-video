#!/bin/bash
# 274 · 长稳取证 + reader 生命周期复算
# 用法：bash tools/p273_reader_life.sh <标签>
set -u
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
URL="https://emby.bangumi.ca/emby/Videos/233908/stream?static=true"
LABEL="${1:-soak}"
SECONDS_TOTAL="${2:-240}"
cd "C:/kivo video" || exit 1

$ADB shell am force-stop com.kivo.cinema
sleep 2
$ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
$ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "$URL" --es kivo.embyToken "$TOKEN" --el kivo.playSize 1935985234 \
  --es kivo.label "$LABEL" >/dev/null 2>&1

echo "=== 源站基线（起播前）==="
$ADB shell "run-as com.kivo.cinema curl -s -o /dev/null -w 'speed=%{speed_download}B/s time=%{time_total}s\n' -H 'X-Emby-Token: $TOKEN' -r 0-8388608 '$URL'" 2>&1 | tail -1

# 每 10 秒采一次 meminfo + 缓存状态
OUT="docs/evidence/p273-emby-stutter/$LABEL"
mkdir -p "$OUT"
for t in $(seq 10 10 "$SECONDS_TOTAL"); do
  sleep 10
  $ADB shell "dumpsys meminfo com.kivo.cinema" > "$OUT/mi-$t.txt" 2>/dev/null
  $ADB shell "run-as com.kivo.cinema sh -c 'cat files/net-diag.log'" > "$OUT/net-$t.log" 2>/dev/null
done

echo "=== 源站基线（结束）==="
$ADB shell "run-as com.kivo.cinema curl -s -o /dev/null -w 'speed=%{speed_download}B/s time=%{time_total}s\n' -H 'X-Emby-Token: $TOKEN' -r 0-8388608 '$URL'" 2>&1 | tail -1

$ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$OUT/net.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$OUT/kernel.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$OUT/diag.log" 2>/dev/null
echo "采集完成 → $OUT"
