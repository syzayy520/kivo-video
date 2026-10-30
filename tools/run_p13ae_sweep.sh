#!/bin/sh
# P13-AE 真机扫描：门在「接近正常」的速率误差下会不会误伤。
#
# 六轮，同一片源（130 s / 360p30 / AV），只有 ppm 与门开关变：
#   100 / 500 / 2000 / 5000 ppm（门开）+ 5000 ppm（门关，对照）+ 0 ppm（门开，基线）
#
# 为什么是 130 s 片源：8 s 里 45 ms 的门至少要 5625 ppm 才够触发，
# 100–2000 ppm 在 8 s 上**算术上不可能**触发 —— 用短片段测"误伤"等于没测。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media/p13h_360p30_130s_av.mp4
OUT="C:/kivo video/docs/evidence/p13ae"
mkdir -p "$OUT"

run_case() {
  name=$1
  ppm=$2
  gate=$3
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  sleep 1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --ei kivo.clock_rate_ppm $ppm --es kivo.drift_gate $gate --es kivo.evidence $name.json" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt 175 ]; do
    sleep 5
    waited=$((waited + 5))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      break
    fi
  done
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
  echo "[done] $name (${waited}s) $(wc -c < "$OUT/$name-sync.json") bytes"
}

run_case p13ae-r0-on-r1 0 on
run_case p13ae-r100-on-r1 100 on
run_case p13ae-r500-on-r1 500 on
run_case p13ae-r2000-on-r1 2000 on
run_case p13ae-r5000-on-r1 5000 on
run_case p13ae-r5000-off-r1 5000 off
echo "[all done]"
