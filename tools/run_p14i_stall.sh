#!/bin/sh
# P14-I：接收圈停顿的构成。两个用例：60 s 烟测 + 30 分钟长跑。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14i"
mkdir -p "$OUT"

run_case() {
  name=$1; media=$2; limit=$3
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$media --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
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

run_case p14i-smoke p13af_1080p30_60s_av.mp4 180
run_case p14i-long1800 p14d_720p30_1800s_av.mp4 2300
echo "[all done]"
