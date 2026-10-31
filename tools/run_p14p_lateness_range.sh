#!/bin/sh
# P14-P：`pace.lateness_ns` 说出自己的分辨力。同设备、同片源、同参数，**只换排程档**。
#
# 要证的那件事（这一块是 `quantiles.per_frame` 的第二次印刷）：
#
#   cpu_paced      → samples = 1800 → 七格有内容，且与 `quantiles.per_frame`
#                                    的 11 个数**逐位相等**（R5）
#   surface_at_time → samples = 0   → 七格全 0，measured = false（R4）
#
# 第二档同时是 R5 的边界：那一档 `av_drift.quantiles` 是空对象，没有孪生块，
# R5 必须判"不可判"而不是"通过" —— 所以这一轮特意不能再只跑一档。
#
# 只跑两轮：这一卡验的是"精度能不能被读出来"，不是性能数字
# （P14-K 已量过 4K60 与 1080p30 54000 帧 0 丢帧）。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14p"
mkdir -p "$OUT"

# ── 先让屏幕别再睡（P14-P 第一次取证就栽在这里）──
#
# 第一轮取证时屏幕在第 2.8 秒熄灭，于是 `presented` 停在 84 帧，而解码器
# 照样把 1800 帧走完 —— 报告写出来的是 `decided=1800 / presented=84 / dropped=0`。
# 三个数加不起来（`presented + dropped != decided`），而**没有任何判据看这一条**，
# `drop_ratio` 还照样算出 0.0（完美）。那两份证据已作废重跑。
#
# `svc power stayon usb` 是"插着 USB 就别熄屏"，可逆；不动系统的
# `screen_off_timeout`（那是用户的设置，取证脚本没有资格改）。
MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon usb" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "input keyevent KEYCODE_WAKEUP" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "wm dismiss-keyguard" >/dev/null 2>&1

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

# 同一片源是刻意的：R5 比的是"两处印刷同一个直方图"，片源不同就失去了同源前提。
run_case p14p-cpupaced-1080p30 p13af_1080p30_60s_av.mp4 cpu_paced 180
run_case p14p-attime-1080p30   p13af_1080p30_60s_av.mp4 surface_at_time 180
echo "[all done]"
