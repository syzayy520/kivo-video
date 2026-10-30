#!/bin/sh
# P14-F：6 轮 × 300 s，统计时钟异常的出现率与形状。
#
# ppm = 0、门开、cpu_paced —— 不注入速率：本卡要的是**设备自己**的异常。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14f"
mkdir -p "$OUT"

run_case() {
  name=$1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/p14f_720p30_300s_av.mp4 --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt 380 ]; do
    sleep 10
    waited=$((waited + 10))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      break
    fi
  done
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
  echo "[done] $name (${waited}s)"
}

for round in 1 2 3 4 5 6; do
  run_case "p14f-r${round}"
done
echo "[all done]"
