#!/bin/sh
# P14-S：「播放速率」的分子到底是不是媒体时间。
#
# 两轮，回答两个不同的问题：
#
#   ① 1080p30 完整播放（`cpu_paced`）→ 报告里同时有**两个**跨度：
#         nominal_span_ns   = (frames − 1) × 名义帧时长   ← 旧的那个"分子"
#         media_pts_span_ns = 首帧到末帧的实测 pts 差     ← 真正的那个
#      预期两者相差不到一帧（60 s 上差在微秒量级），`media_rate` 落在 [0.98, 1.02]，
#      且 `media_span_frames == presented`（实测跨度覆盖的帧就是上过屏的帧）。
#
#   ② 同一片源、`surface_at_time` → 这一档**不提供钟读数**，`media_rate_measured`
#      必须是 `false`，并且必须带理由。这一轮要证明的不是"跑得准"，
#      而是"判不动的时候它会说出来" —— `/16` 起的 6 份 attime 证据全是这个形状
#      （`clock_supplied_frames = 0`），而 `/29` 在那之前只能读到一堆 0。
#
# 结论不预设：差多少、落不落带内，测出来是什么就写什么。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14s"
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
  name=$1; present=$2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/p13af_1080p30_60s_av.mp4 --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present $present --es kivo.drift_gate on --es kivo.evidence $name.json" >/dev/null 2>&1
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

# ── ① cpu_paced：两个跨度并排 ──
echo "[1/2] cpu_paced（两个跨度并排，看差多少）"
start_app p14s-media-1080p30 cpu_paced
used=$(wait_for_report p14s-media-1080p30 180)
pull_evidence p14s-media-1080p30
echo "[done] p14s-media-1080p30 (${used}s)"

# ── ② surface_at_time：判不动的时候会不会说出来 ──
echo "[2/2] surface_at_time（预期 media_rate_measured=false + 理由）"
start_app p14s-media-attime-1080p30 surface_at_time
used=$(wait_for_report p14s-media-attime-1080p30 180)
pull_evidence p14s-media-attime-1080p30
echo "[done] p14s-media-attime-1080p30 (${used}s)"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1
echo "[all done]"
