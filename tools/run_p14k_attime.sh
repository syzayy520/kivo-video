#!/bin/sh
# P14-K：surface_at_time 排程档。此前所有结论只在 cpu_paced 一档上。
#
# 三个用例对应三件事：
#   1080p30 60 s  —— 与 p14i-smoke（cpu_paced、同片源）直接 A/B；
#   4K60 8 s      —— 最吃紧的一档，看合成器定时在高负载下还成不成立；
#   720p30 1800 s —— 与 p14i-long1800 直接 A/B（长跑、稳态）。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14k"
mkdir -p "$OUT"

run_case() {
  name=$1; media=$2; limit=$3
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$media --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present surface_at_time --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt "$limit" ]; do
    sleep 10
    waited=$((waited + 10))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      sleep 4
      break
    fi
  done
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
  echo "[done] $name (${waited}s)"
}

run_case p14k-smoke-1080p30 p13af_1080p30_60s_av.mp4 180
run_case p14k-4k60 p13c_4k60_av.mp4 180
run_case p14k-long1800 p14d_720p30_1800s_av.mp4 2300
echo "[all done]"
