#!/bin/sh
# P14-O：`p99` 自带范围。同设备、同分辨率、同片长，**只差帧率**。
#
# 要证的那件事（D2 判据的第一个可复算前提）：
#
#   30fps  p99 = 125000  落桶 512  那桶里 1196/1200 个样本  → 范围 [min, 250000)
#   60fps  p99 = 625000  落桶 514  那桶里    3/1200 个样本   → 范围 [500000, 750000)
#
# 于是 D2 的商不再是"5.00x"一个点估计，而是一个区间：
#   下界 = 500000 / 250000 = 2.00x（正好等于判据门槛）
#   上界 = 750000 / 15625  ≈ 48.00x
#
# 这两个数都是**桶界之比**，所以它们是确定的，不是这一轮碰巧。
# 跑了才知道的是：`p99_bucket_samples` 到底几个样本 —— 那是"这个尾部由谁投票"。
#
# 只跑两轮：这一卡验的是"精度能不能被读出来"，不是性能数字
# （P14-K 已量过 4K60 与 1080p30）。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14o"
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

# 配对必须是同片长、同分辨率、**只差帧率** —— 否则 D2 的分组键会把它们分开，
# 而分开之后"没有可比的一对"看上去和"跑了但没结论"一模一样。
run_case p14o-1080p30 p13af_1080p30_60s_av.mp4 180
run_case p14o-1080p60 p13af_1080p60_60s_av.mp4 180
echo "[all done]"
