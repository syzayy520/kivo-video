#!/bin/sh
# P14-L：同一台设备、同一片源、同一参数，**只换排程档**，看可测性翻转。
#
# 要证的那件事：`over_budget_frames` 的 0 在两个档里含义完全不同。
#   cpu_paced      → lateness 有样本 → over_budget_frames_measured = true  → 那个 0 是真读数
#   surface_at_time → lateness 零样本 → over_budget_frames_measured = false → 那个 0 是空的
#
# 只跑两轮就够：这一卡验的是"可测性标注对不对"，不是性能数字。
# 性能数字在 P14-K 已经量过（56288 个时钟样本、0 丢帧）。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14l"
mkdir -p "$OUT"

run_case() {
  name=$1; media=$2; present=$3; limit=$4
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$media --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present $present --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
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
  echo "[done] $name ($present, ${waited}s)"
}

run_case p14l-cpupaced-1080p30 p13af_1080p30_60s_av.mp4 cpu_paced 180
run_case p14l-attime-1080p30   p13af_1080p30_60s_av.mp4 surface_at_time 180
echo "[all done]"
