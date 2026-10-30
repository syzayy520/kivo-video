#!/bin/sh
# P14-C 判别实验：那 5 次/秒到底属于哪个子系统。
#
# 不做返回地址采样（那是另一个量级的工装），先用"关掉一个子系统看它还在不在"：
#   base     有音频、有节拍  —— 基线
#   noaudio  纯视频          —— 若 5 Hz 消失，说明是音频侧
#   nopace   不开节拍        —— 若消失，说明是节拍器
#
# 三例都用 1080p30 / 60 s / ppm 0（不需要注入，本卡只看分配）。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14c"
mkdir -p "$OUT"

run_case() {
  name=$1
  shift
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/p13af_1080p30_60s_av.mp4 --es kivo.present cpu_paced --es kivo.evidence $name.json $*" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt 130 ]; do
    sleep 5
    waited=$((waited + 5))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name.json" >/dev/null 2>&1; then
      break
    fi
  done
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  echo "[done] $name (${waited}s)"
}

run_case p14c-base    --ei kivo.pace 1 --ei kivo.drop 1
run_case p14c-noaudio --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.audio off
run_case p14c-nopace  --ei kivo.pace 0 --ei kivo.drop 1
echo "[all done]"
