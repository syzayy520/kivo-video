#!/bin/sh
# P14-Q：节拍侧的账要能加（`frames_decided = presented + dropped + 未交付`）。
#
# 要证的两件事，**必须两轮，缺一不可**：
#
#   ① 完整播放一轮 → `undelivered_frames = 0`
#      （`decided` 的每一帧要么呈现、要么丢弃、要么没有面可上）
#   ② 播到一半切后台 → `undelivered_frames > 0`
#      这正是 P14-P 那次事故的形状：`decided = 1800 / presented = 84 / dropped = 0`。
#      解码器把整片走完，而交付停在切走那一刻。
#
# 只有 ① 而没有 ②，这一格永远是 0 —— 而"永远为 0 的字段"和"没有这一格"
# 在证据上分不出来（P14-L 的 `over_budget_frames` 就是这个病）。
#
# ② 的结论**不预设**：`undelivered` 是否等于 `frames_discarded_no_surface`
# 决定 S2 绿还是红。绿说明切后台走的是换面路径（账早有解释）；
# 红说明存在第三种出口。两种都写进状态卡。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14q"
mkdir -p "$OUT"

# 屏幕别再睡（P14-P 第一次取证栽在这里：屏幕 2.8 秒熄灭 → presented 停在 84）。
# `svc power stayon usb` 可逆，不动系统的 `screen_off_timeout`。
MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon usb" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "input keyevent KEYCODE_WAKEUP" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "wm dismiss-keyguard" >/dev/null 2>&1

# ── 锁屏前置检查（P14-Q 加）──
#
# 锁屏和"屏幕熄灭"是两件事：锁屏时 App 照样启动、`decided` 照样走到 1800，
# 但窗口不可见 ⇒ `presented = 1`、`frames_discarded_no_surface = 1799`。
# 报告看起来像"节拍崩了"，实际只是没有面可上 —— 本卡为此浪费了整整两轮。
#
# 而且**远程解不开**：`input swipe` / `wm dismiss-keyguard` / `input keyevent 82` /
# `locksettings set-disabled` / 重启，全部被 MIUI 拦掉。唯一出路是人工解锁。
# 所以这里只能"发现并停下"，不能"自动解决" —— 但提前失败比产出一批假证据强。
locked=$(MSYS_NO_PATHCONV=1 "$A" $DEV shell "dumpsys window | grep -m1 mDreamingLockscreen" 2>/dev/null | tr -d '\r' | grep -o 'mDreamingLockscreen=[a-z]*')
if [ "$locked" = "mDreamingLockscreen=true" ]; then
  echo "[STOP] 设备锁屏（$locked）。请先在手机上解锁再运行。"
  echo "       锁屏下跑出来的会是 presented=1 / dns=1799 的假证据，不是节拍有问题。"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1
  exit 2
fi
echo "[ok] 未锁屏（$locked）"

start_app() {
  name=$1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/p13af_1080p30_60s_av.mp4 --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
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
echo "[1/2] 完整播放"
start_app p14q-full-1080p30
used=$(wait_for_report p14q-full-1080p30 180)
pull_evidence p14q-full-1080p30
echo "[done] p14q-full-1080p30 (${used}s)"

# ── ② 播到 8 秒切后台 ──
#
# 8 秒 ≈ 240 帧 @30fps，与 P14-P 那次 84 帧同一量级（那次是 2.8 秒）。
# 切走之后**给足 25 秒**再取：会话收尾要写报告，而 `presented` 已经不会再涨 ——
# 这正是"解码继续、交付停止"那个窗口。
echo "[2/2] 播 8 秒切后台"
start_app p14q-background-1080p30
sleep 8
MSYS_NO_PATHCONV=1 "$A" $DEV shell "input keyevent KEYCODE_HOME" >/dev/null 2>&1
echo "[home sent]"
sleep 25
pull_evidence p14q-background-1080p30
echo "[done] p14q-background-1080p30"

# 还原：不要让设备停在"插着 USB 就不睡"的状态上。
MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1
echo "[all done]"
