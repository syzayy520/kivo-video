#!/bin/sh
# P14-R：一个没有播完的会话，能不能留下一份说得清的账。
#
# 两轮，回答两个不同的问题：
#
#   ① 完整播放 → 数值必须与 P14-Q 那轮**逐位相同**（1800 / 1800 / 0 / undelivered=0）。
#      这一轮不是新结论，是**回归**：本卡把收尾顺序从「写 → stop → 写」改成
#      「stop → 写 → 写」，改了顺序就必须证明数值没动。
#
#   ② 播到 6 秒切后台 → 必须落盘，且 `end_reason = interrupted`。
#      P14-Q 那轮就是在这里什么都没写出来（`cat: No such file or directory`），
#      而当时的解释"进程被回收"只说对了最后一环 —— 真正的堵点是
#      「停了、没报错、也没跑完」在层 3 被明确判为静默失败，于是这个状态**没有形状**。
#
# ② 的结论**不预设**：`end_reason` 是否真为 interrupted、账是否仍然闭合（S2 绿/红），
# 测出来是什么就写什么。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14r"
mkdir -p "$OUT"

# 屏幕别再睡（P14-P 第一次取证栽在这里：屏幕 2.8 秒熄灭 → presented 停在 84）。
# `svc power stayon usb` 可逆，不动系统的 `screen_off_timeout`。
MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon usb" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "input keyevent KEYCODE_WAKEUP" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "wm dismiss-keyguard" >/dev/null 2>&1

# ── 锁屏前置检查 ──
#
# 锁屏下 App 照样启动、`decided` 照样涨，但窗口不可见 ⇒ `presented = 1`。
# 而**远程解不开**（MIUI 拦注入事件，重启也没用），所以这里只能"发现并停下"。
locked=$(MSYS_NO_PATHCONV=1 "$A" $DEV shell "dumpsys window | grep -m1 mDreamingLockscreen" 2>/dev/null | tr -d '\r' | grep -o 'mDreamingLockscreen=[a-z]*')
if [ "$locked" = "mDreamingLockscreen=true" ]; then
  echo "[STOP] 设备锁屏（$locked）。请先在手机上解锁再运行。"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1
  exit 2
fi
echo "[ok] 未锁屏（$locked）"

start_app() {
  name=$1; extra=$2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/p13af_1080p30_60s_av.mp4 --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate on $extra --es kivo.evidence $name.json" >/dev/null 2>&1
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
    sleep 10
    waited=$((waited + 10))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      sleep 4
      break
    fi
  done
  echo "$waited"
}

# ── ① 完整播放（60 s 片源，留 180 s 余量）──
echo "[1/2] 完整播放（回归：收尾顺序改了，数值不许动）"
start_app p14r-complete-1080p30 ""
used=$(wait_for_report p14r-complete-1080p30 180)
pull_evidence p14r-complete-1080p30
echo "[done] p14r-complete-1080p30 (${used}s)"

# ── ② 播到 6 秒切后台（`kivo.finalize on_stop`）──
#
# 6 秒 ≈ 180 帧 @30fps。比 P14-Q 那次的 8 秒更早一点，是为了让
# "没走完"这件事没有疑问（180 << 1800）。
echo "[2/2] 播 6 秒切后台"
start_app p14r-interrupt-1080p30 "--es kivo.finalize on_stop"
sleep 6
MSYS_NO_PATHCONV=1 "$A" $DEV shell "input keyevent KEYCODE_HOME" >/dev/null 2>&1
echo "[home sent]"
sleep 12
pull_evidence p14r-interrupt-1080p30
echo "[done] p14r-interrupt-1080p30"

# 还原：不要让设备停在"插着 USB 就不睡"的状态上。
MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1
echo "[all done]"
