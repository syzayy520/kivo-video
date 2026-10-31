#!/bin/sh
# P14-H：三个用例验"守卫知道过了多久"。
#
# 1) inject300：注入 300 ms 台阶 —— **回归**，真跳变必须仍被钳制；
# 2) smoke：60 s 无注入 —— 启动期那 284 ms 停顿以前钳 1 次，现在应当 0 次；
# 3) long1800：30 分钟 —— 稳态与片尾都不再误伤。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14h"
mkdir -p "$OUT"

pull() {
  name=$1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
  echo "[done] $name"
}

wait_for() {
  name=$1
  limit=$2
  waited=0
  while [ "$waited" -lt "$limit" ]; do
    sleep 10
    waited=$((waited + 10))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      sleep 4
      return 0
    fi
  done
  echo "[timeout] $name after ${waited}s"
}

launch() {
  name=$1
  media=$2
  shift 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$media --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate on --es kivo.evidence $name.json $*" >/dev/null 2>&1
}

# ── 1. 注入回归：真跳变必须仍被钳制 ──
launch p14h-inject300 p13af_1080p30_60s_av.mp4 --ei kivo.clock_offset_ms 300 --ei kivo.clock_offset_at_ms 3000
wait_for p14h-inject300 180
pull p14h-inject300

# ── 2. 无注入 60 s：启动停顿不再钳制 ──
launch p14h-smoke p13af_1080p30_60s_av.mp4
wait_for p14h-smoke 180
pull p14h-smoke

# ── 3. 30 分钟：稳态与片尾 ──
launch p14h-long1800 p14d_720p30_1800s_av.mp4
wait_for p14h-long1800 2300
pull p14h-long1800

echo "[all done]"
