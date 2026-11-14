#!/bin/bash
# 273 v44 · 归一化配对实验（修复前 / 修复后）
#
# 【为什么必须配对】源站是公共的、会波动的。单跑一轮无法区分
#   "代码改好了" 与 "源站当时快"。⇒ 两个版本交替跑，各 2 轮，
#   每轮前后各测一次源站基线（真机直连 8 MiB）。
#
# 【用法】bash /tmp/pair_test.sh

ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
URL="https://emby.bangumi.ca/emby/Videos/233908/stream?static=true"
ROOT="C:/kivo video"
EV="$ROOT/docs/evidence/p273-emby-stutter"
BUILD=/c/Android/jdk-17.0.13+11

cd "$ROOT" || exit 1

baseline() {
  $ADB shell "run-as com.kivo.cinema curl -s -o /dev/null -w 'time=%{time_total}s speed=%{speed_download}\n' -H 'X-Emby-Token: $TOKEN' -r 0-8388608 '$URL'" 2>&1 | tail -1
}

run_one() {
  local label=$1
  $ADB shell am force-stop com.kivo.cinema
  sleep 1
  $ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
  $ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
    --es kivo.playurl "$URL" --es kivo.embyToken "$TOKEN" --el kivo.playSize 1935985234 \
    --es kivo.landSeekSecs "4932" --ei kivo.landSeekDelayMs 9000 --es kivo.label "$label" >/dev/null 2>&1
  sleep 32
  mkdir -p "$EV/$label"
  $ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$EV/$label/net.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$EV/$label/kernel.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$EV/$label/diag.log" 2>/dev/null
  echo "--- $label ---"
  node tools/p273_drag.mjs "$EV/$label" 2>&1 | grep -E "seek→首帧|落点窗口"
  node tools/p273_gaps.mjs "$EV/$label/kernel.log" "$EV/$label/diag.log" 2>&1 | grep -E "实际帧率|Enter buffering"
}

install_and_run() {
  local apk=$1 label=$2
  $ADB install -r "$apk" >/dev/null 2>&1
  run_one "$label"
}

for round in 1 2; do
  echo "########## 轮次 $round ##########"
  echo -n "  [修复前] 基线 "; baseline
  install_and_run /tmp/apk-before.apk "pair-before-$round"
  echo -n "  [修复后] 基线 "; baseline
  install_and_run /tmp/apk-after.apk "pair-after-$round"
done
