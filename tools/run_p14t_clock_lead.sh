#!/bin/sh
# P14-T：那 5.6% 是谁跑快了 —— 一个只动一个旋钮的对照实验。
#
# 三块屏、同一台机器、同一批片源，**只改一个东西：漂移门开不开**。
#
#   ① 1080p30 / 60 s / 门开  → 领先量应在微秒量级，`media_rate` 在 [0.98, 1.02]
#   ② 4K60    /  8 s / 门关  → 预期**复现**那 4 份越界：领先量数十到数百 ms，
#                              `media_rate` 出带（这一轮会进 `KNOWN_OUT_OF_BAND`）
#   ③ 4K60    /  8 s / 门开  → 同一片源、同一负载，只把门打开：领先量应被夹回
#                              门限（45 ms）附近，`media_rate` 回到带内
#
# ②③ 是关键。若两者一样，那"越界是门的账"这句话就是错的，本卡的结论作废。
# 结论不预设：测出来是什么就写什么。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14t"
mkdir -p "$OUT"

# 屏幕别再睡（P14-P 第一次取证栽在这里：屏幕 2.8 秒熄灭 → presented 停在 84）。
MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon usb" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "input keyevent KEYCODE_WAKEUP" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "wm dismiss-keyguard" >/dev/null 2>&1

# ── 锁屏前置检查 ──
#
# 锁屏下 App 照样启动、`decided` 照样涨，但窗口不可见 ⇒ `presented = 1`，
# 而**远程解不开**（MIUI 拦注入事件，重启也没用）。只能"发现并停下"。
locked=$(MSYS_NO_PATHCONV=1 "$A" $DEV shell "dumpsys window | grep -m1 mDreamingLockscreen" 2>/dev/null | tr -d '\r' | grep -o 'mDreamingLockscreen=[a-z]*')
if [ "$locked" = "mDreamingLockscreen=true" ]; then
  echo "[STOP] 设备锁屏（$locked）。请先在手机上解锁再运行。"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1
  exit 2
fi
echo "[ok] 未锁屏（$locked）"

start_app() {
  name=$1; clip=$2; gate=$3
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$clip --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate $gate --es kivo.evidence $name.json" >/dev/null 2>&1
}

pull_evidence() {
  name=$1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
}

wait_for_report() {
  name=$1; limit=$2
  waited=0
  while [ "$waited" -lt "$limit" ]; do
    sleep 6
    waited=$((waited + 6))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      sleep 4
      break
    fi
  done
  echo "$waited"
}

# ── ① 1080p30 / 60 s / 门开：基线 ──
echo "[1/3] 1080p30 60s，门开（基线：领先量应在微秒级）"
start_app p14t-1080p30-gate-on p13af_1080p30_60s_av.mp4 on
used=$(wait_for_report p14t-1080p30-gate-on 180)
pull_evidence p14t-1080p30-gate-on
echo "[done] p14t-1080p30-gate-on (${used}s)"

# ── ② 4K60 / 8 s / 门关：复现 ──
echo "[2/3] 4K60 8s，门关（预期复现越界）"
start_app p14t-4k60-gate-off p13c_4k60_av.mp4 off
used=$(wait_for_report p14t-4k60-gate-off 90)
pull_evidence p14t-4k60-gate-off
echo "[done] p14t-4k60-gate-off (${used}s)"

# ── ③ 4K60 / 8 s / 门开：只动门 ──
echo "[3/3] 4K60 8s，门开（同一片源，只把门打开）"
start_app p14t-4k60-gate-on p13c_4k60_av.mp4 on
used=$(wait_for_report p14t-4k60-gate-on 90)
pull_evidence p14t-4k60-gate-on
echo "[done] p14t-4k60-gate-on (${used}s)"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1
echo "[all done]"
