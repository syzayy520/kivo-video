#!/bin/sh
# P13-AE 复现轮：r100 与 r2000 各再跑一轮，回答"那一次 58 ms 是速率还是时钟事件"。
#
# 判别逻辑：
#   同一 ppm 再跑两次，若仍各自触发 1 次且修正量相近 → 与 ppm 相关，是速率路径的问题；
#   若第二次 0 次、第三次 0 次 → 与那一次的 `monotonic_rejects = 1` 同时出现，
#   是**时钟事件**被门如实吸收，不是门误伤。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media/p13h_360p30_130s_av.mp4
OUT="C:/kivo video/docs/evidence/p13ae"

run_case() {
  name=$1
  ppm=$2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  sleep 1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --ei kivo.clock_rate_ppm $ppm --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
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
  echo "[done] $name (${waited}s)"
}

run_case p13ae-r100-on-r2 100
run_case p13ae-r100-on-r3 100
run_case p13ae-r2000-on-r2 2000
echo "[repeat done]"
