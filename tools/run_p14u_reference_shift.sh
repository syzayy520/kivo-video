#!/bin/sh
# P14-U：报告说差 125 µs，耳朵听到的是多少 —— 一个用注入器造出"钟撒谎"的对照实验。
#
# 三块屏、同一台机器、同一片源（4K60 / 8 s），**只改一个东西：钟被注入了什么**。
#
#   ① 阶跃 400 ms  → 预期 `verdict = stepped`、`max_step_ns` ≈ 400 ms、
#                    `vs_wall.last_ns` ≈ −400 ms，而 `av_drift.p50` 仍是 125 µs
#   ② 速率 5000 ppm → 预期 `verdict = ramped`、`step_frames = 0`、`last_ns` ≈ 40 ms
#   ③ 不注入       → 预期 `verdict = steady`、`|vs_wall.last_ns| ≤ 一帧`
#
# ① 是本卡的关键：**同一份证据里同时说"相对钟 125 µs"和"相对墙 −400 ms"**。
# 两句话出自同一批帧，这就是"`av_drift` 等于音画偏差"这句话在什么条件下不成立。
#
# 三组**全部把漂移门关掉**：门会把领先量夹在 45 ms 以内（P14-T 的 A2 判据），
# 那正好是本卡要绕开的那层遮蔽。门开着测到的是门，不是钟。
#
# 结论不预设：测出来是什么就写什么。②若也出 `stepped`，说明注入器的速率档
# 是"分段跳"而不是"连续偏"，那本卡的 verdict 二分法就得改。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14u"
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
  name=$1; clip=$2; extra=$3
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am force-stop com.kivo.cinema" >/dev/null 2>&1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema rm -f files/evidence/$name.json files/evidence/$name-sync.json" >/dev/null 2>&1
  sleep 2
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$clip --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced --es kivo.drift_gate off $extra --es kivo.evidence $name.json" >/dev/null 2>&1
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

# ── ① 钟阶跃 400 ms（媒体时间第 2 s 处一次加上去）──
echo "[1/3] 4K60，钟阶跃 +400 ms @2 s，门关（预期 stepped / vs_wall ≈ −400 ms）"
start_app p14u-step-400ms p13c_4k60_av.mp4 "--ei kivo.clock_offset_ms 400 --ei kivo.clock_offset_at_ms 2000"
used=$(wait_for_report p14u-step-400ms 180)
pull_evidence p14u-step-400ms
echo "      用时 ${used}s"

# ── ② 钟速率 5000 ppm（8 s 上累积约 40 ms）──
echo "[2/3] 4K60，钟速率 +5000 ppm，门关（预期 ramped / step_frames=0 / last ≈ 40 ms）"
start_app p14u-ramp-5000ppm p13c_4k60_av.mp4 "--ei kivo.clock_rate_ppm 5000"
used=$(wait_for_report p14u-ramp-5000ppm 180)
pull_evidence p14u-ramp-5000ppm
echo "      用时 ${used}s"

# ── ③ 对照：什么都不注入 ──
echo "[3/3] 4K60，不注入，门关（预期 steady / |vs_wall.last| ≤ 一帧）"
start_app p14u-control p13c_4k60_av.mp4 ""
used=$(wait_for_report p14u-control 180)
pull_evidence p14u-control
echo "      用时 ${used}s"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

# ── 结果摘要 ──
#
# 只印本卡关心的六格。判据在 `verify_p12c_sync.py` 的 U 组，这里只是让人
# 一眼看到"三组的形状对不对"。
echo ""
echo "── 摘要 ──"
for name in p14u-step-400ms p14u-ramp-5000ppm p14u-control; do
  f="$OUT/$name-sync.json"
  if [ ! -s "$f" ]; then
    echo "$name: 没有文件"
    continue
  fi
  python - "$f" "$name" <<'PY'
import json, sys
path, name = sys.argv[1], sys.argv[2]
try:
    d = json.load(open(path, encoding="utf-8"))
except Exception as error:
    print(f"{name}: 读不出（{error}）")
    raise SystemExit(0)
pace = d.get("pace", {})
trace = pace.get("clock_lead_trace", {})
wall = d.get("av_drift", {}).get("vs_wall", {})
drift = d.get("av_drift", {}).get("quantiles", {}).get("per_frame", {})
print(f"{name}: /{str(d.get('schema','')).split('/')[-1]}  "
      f"verdict={trace.get('verdict')}  max_step={trace.get('max_step_ns')}  "
      f"lead={trace.get('last_ns')}  av_drift.p50={drift.get('p50')}  "
      f"vs_wall.last={wall.get('last_ns')}")
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14u/*-sync.json"
