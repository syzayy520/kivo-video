#!/bin/sh
# P14-A 真机取证：三条以前没有的尺子。
#
# 三个用例各回答一件事：
#   1080p60   —— 常规路径的 CPU 占比与稳态分配
#   4k60      —— 分辨率上去之后 CPU 占比涨多少（G2 的第一个自变量）
#   long130   —— 130 s 长稳：稳态分配次数是否随时间累加（G3 真正想抓的东西）
#
# 期望（先写下来，跑完对）：
#   * `cpu.valid = true`、`process_ratio` 在 0.05–0.5 之间（硬解 + 上屏的下限/上限）
#   * `alloc.hook_installed = true`（钩子真的链进 APK 了）
#   * `buffers.in_flight = 0`、`leaked = 0`
#   * `calls_steady` 随片长线性增长 = 热路径在分配；保持常数 = 零分配
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14a"
mkdir -p "$OUT"

run_case() {
  name=$1
  file=$2
  ppm=$3
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  # **先删掉设备上的同名证据。**
  # 不删的话轮询会在上一轮留下的文件上立刻命中，拉回来的是旧数据 ——
  # 这个坑不报错，只会安静地给出上一次的数字（第一版就是这么骗过自己的）。
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$file --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --ei kivo.clock_rate_ppm $ppm --es kivo.evidence $name.json" >/dev/null 2>&1
  waited=0
  while [ "$waited" -lt 200 ]; do
    sleep 5
    waited=$((waited + 5))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name.json" >/dev/null 2>&1; then
      break
    fi
  done
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json" 2>/dev/null
  echo "[done] $name (${waited}s) $(wc -c < "$OUT/$name.json") bytes"
}

run_case p14a-1080p60-r1 p13c_1080p60_av.mp4 2000
run_case p14a-4k60-r1 p13c_4k60_av.mp4 2000
run_case p14a-long130-r1 p13h_360p30_130s_av.mp4 0
echo "[all done]"
