#!/bin/sh
# P14-G：4 轮 × 1800 s，采残差尾部 + 钳制类的轮间一致性。
#
# 与 P14-D 完全同参数（ppm 0、门开、cpu_paced、同一条 1800 s 片源），
# 这样 D 的那一轮可以直接与本卡的 4 轮比较，不用另做归一化。
#
# 为什么仍然是 4 轮而不是 1 轮 2 小时：尾部形状 1 轮就有 54000 个样本，
# 但"每轮几次钳制"这个问题只有多轮才能看出稳定性。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14g"
LIMIT=2300
mkdir -p "$OUT"

run_case() {
  name=$1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/p14d_720p30_1800s_av.mp4 --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt "$LIMIT" ]; do
    sleep 20
    waited=$((waited + 20))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      sleep 4
      break
    fi
  done
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
  echo "[done] $name (waited ${waited}s, $(wc -c < "$OUT/$name-sync.json") bytes)"
}

for round in 1 2 3 4; do
  run_case "p14g-long1800-r${round}"
done
echo "[all done]"
