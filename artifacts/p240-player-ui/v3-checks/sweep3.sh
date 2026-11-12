#!/usr/bin/env bash
# v3 sweep round 3 — ATOMIC protocol: taps + sleeps + screencap all inside ONE
# device-side shell (host latency can no longer race the 3s auto-hide), then pull.
# App is restarted first so audioOnly/lock state is guaranteed fresh.
set -u
# Git Bash 会把 /sdcard/... 转成 C:/Program Files/Git/sdcard/... ⇒ pull 全失败（首轮实测）
export MSYS_NO_PATHCONV=1
ADB="C:/Android/sdk/platform-tools/adb -s b6e950ee"
OUT="C:/kivo video/artifacts/p240-player-ui/v3-checks"
DEV=/sdcard/kivo-sweep.png

shot() { # name device-script
  $ADB shell "$2"
  $ADB pull $DEV "$OUT/s3-$1.png" >/dev/null 2>&1
  echo "done $1"
}

# fresh state
$ADB shell am force-stop com.kivo.cinema
MSYS_NO_PATHCONV=1 $ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.sourceId seed-smb-xiaomi --es kivo.smbShare XiaoMi --es kivo.smbPath "视频/kivo-test/shelter.mkv"
sleep 12
$ADB shell "sleep 4"   # converge to bars-hidden

shot bars "input tap 1200 540; sleep 0.8; screencap -p $DEV; input tap 1200 540; sleep 4"
shot speed "input tap 1200 540; sleep 0.5; input tap 2305 956; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot subtitle "input tap 1200 540; sleep 0.5; input tap 2179 142; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot audio "input tap 1200 540; sleep 0.5; input tap 2042 142; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot quality "input tap 1200 540; sleep 0.5; input tap 1904 142; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot skip "input tap 1200 540; sleep 0.5; input tap 1764 142; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot cast "input tap 1200 540; sleep 0.5; input tap 1352 142; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot resource "input tap 1200 540; sleep 0.5; input tap 1921 956; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot episodes "input tap 1200 540; sleep 0.5; input tap 2123 956; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"
shot danmaku-ops "input tap 1200 540; sleep 0.5; input swipe 1489 142 1489 142 900; sleep 1.4; screencap -p $DEV; input tap 1200 540; sleep 4"

# subtitle display drill: open subtitle panel, tap 显示设置 row.
# Row math (K40 ×2.75, s3-subtitle.png 实测): 搜索字幕带 reason 副题 ⇒ 双行 60dp。
# 显示设置 row ≈ y[930,1045]px，center ≈ 987 ⇒ tap 1000。(933 会落在「操作」头上。)
shot subtitle-display "input tap 1200 540; sleep 0.5; input tap 2179 142; sleep 1.4; input tap 1932 1000; sleep 1.0; screencap -p $DEV; input tap 1200 540; sleep 4"

# audio-only: enter, capture, exit — all atomic
shot audiomode "input tap 1200 540; sleep 0.5; input tap 2317 142; sleep 1.0; screencap -p $DEV; input tap 2317 142; sleep 4"

# paused: floating camera; resume after capture
shot paused "input tap 1200 540; sleep 0.5; input tap 166 956; sleep 0.8; screencap -p $DEV; input tap 166 956; sleep 4"

# locked: long-press play; capture; unlock = wake-tap re-shows glyph (2.5s window) then padlock.
# 旧版在 screencap 后直接点挂锁，常超 2.5s 窗口 ⇒ 设备留锁、后续全吞（首轮实测）。
shot locked "input tap 1200 540; sleep 0.5; input swipe 166 956 166 956 900; sleep 0.5; screencap -p $DEV; input tap 1200 540; sleep 0.3; input tap 162 956; sleep 4"

echo SWEEP3_COMPLETE