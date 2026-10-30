#!/bin/sh
# P13-AF 真机取证：同一 ppm，三臂各两轮。
#
#   A = 1080p30 60s（基线）
#   B = 1080p60 60s（只看帧率）
#   C = 4K30    60s（只看分辨率）
#
# 全部 2000 ppm、门开、cpu_paced。判据见 119-p13af-design.md：
#   R1 次数 vs ppm×时长/band（复用 verify_p13ae_gate.py）
#   R2 三臂极差 ≤ 4
#   R5 同臂两轮之差 ≤ 3
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p13af"
mkdir -p "$OUT"

run_case() {
  name=$1
  file=$2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  # 先删设备上的同名证据：不删的话轮询会在上一轮的文件上立刻命中，
  # 拉回来的是旧数据（P14-A 踩过一次，见 116-p14a-status.md §四）。
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$file --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --ei kivo.clock_rate_ppm 2000 --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt 130 ]; do
    sleep 5
    waited=$((waited + 5))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      break
    fi
  done
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
  echo "[done] $name (${waited}s)"
}

run_case p13af-a1080p30-r1 p13af_1080p30_60s_av.mp4
run_case p13af-a1080p30-r2 p13af_1080p30_60s_av.mp4
run_case p13af-b1080p60-r1 p13af_1080p60_60s_av.mp4
run_case p13af-b1080p60-r2 p13af_1080p60_60s_av.mp4
run_case p13af-c4k30-r1    p13af_4k30_60s_av.mp4
run_case p13af-c4k30-r2    p13af_4k30_60s_av.mp4
echo "[all done]"
