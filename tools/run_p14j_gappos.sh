#!/bin/sh
# P14-J：长间隔发生在会话的第几秒。60 s 片源包含两端的边界，跑两轮看是否稳定。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14j"
mkdir -p "$OUT"

for round in 1 2; do
  name="p14j-smoke-r${round}"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/p13af_1080p30_60s_av.mp4 --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt 150 ]; do
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
done
echo "[all done]"
