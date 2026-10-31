#!/bin/sh
# P14-W：8 s 片源的量程不足 —— 换 60 s 片源，看门会不会在第 9 秒附近动手。
#
# P14-V 算过：5 000 ppm 在 8 s 上只能累积 40 ms，而门限是 45 ms ⇒ `limit_reach_s`
# 比整段还长（12.07 s > 8 s）。**门在 8 s 上一声不响并不能证明它做对了。**
# 这一张卡把时长拉到 60 s，让 `reach` 落进量程内（45 ms / 5000 ppm = 9 s）。
#
# 四组，只改一个东西（门开/门关、注不注入）：
#   ① 门开 + 5000 ppm（60 s）：预期 `drift_rebases > 0`，领先量被夹在 40–45 ms
#   ② 门关 + 5000 ppm（60 s）：预期 `drift_rebases = 0`，领先量涨到 ≈ 290 ms
#   ③ 门开 + 不注入（60 s）   ：预期 `drift_rebases = 0`，领先量 ≈ 0
#   ④ 门开 + 5000 ppm（4K30 60 s）：换分辨率/帧率再验一次，排除"只是 1080p60 的巧合"
#
# 为什么主用 1080p60 而不是 4K60：帧时长与 4K60 相同（16.67 ms），而 60 s 的 4K60
# 编码要十几分钟、解码负载还会和"门有没有动作"混在一起。分辨率不是这一卡的自变量，
# ④ 用 4K30 交叉检查一次就够。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14w"
mkdir -p "$OUT"

# 屏幕别再睡（P14-P 栽过：2.8 秒熄灭 → presented 停在 84）
MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon usb" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "input keyevent KEYCODE_WAKEUP" >/dev/null 2>&1
MSYS_NO_PATHCONV=1 "$A" $DEV shell "wm dismiss-keyguard" >/dev/null 2>&1

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
  # shellcheck disable=SC2086
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$clip --ei kivo.pace 1 --ei kivo.drop 1 --es kivo.present cpu_paced $extra --es kivo.evidence $name.json" >/dev/null 2>&1
}

pull_evidence() {
  name=$1
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name.json" > "$OUT/$name.json"
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema cat files/evidence/$name-sync.json" > "$OUT/$name-sync.json"
}

# 60 s 片源 + 收尾：给 150 s 上限（P14-W 第一次跑就是按 8 s 的节奏等 30 s，
# 结果文件还没写完就取，拿到空文件）。
wait_for_report() {
  name=$1; limit=$2
  waited=0
  while [ "$waited" -lt "$limit" ]; do
    sleep 5
    waited=$((waited + 5))
    if MSYS_NO_PATHCONV=1 "$A" $DEV shell "run-as com.kivo.cinema ls files/evidence/$name-sync.json" >/dev/null 2>&1; then
      sleep 4
      break
    fi
  done
  echo "$waited"
}

run_group() {
  label=$1; name=$2; clip=$3; extra=$4
  echo "[$label] $clip  $extra"
  start_app "$name" "$clip" "$extra"
  used=$(wait_for_report "$name" 150)
  pull_evidence "$name"
  echo "      用时 ${used}s"
}

run_group "1/4" p14w-60s-gate-on-5000ppm p13af_1080p60_60s_av.mp4 "--ei kivo.clock_rate_ppm 5000"
run_group "2/4" p14w-60s-gate-off-5000ppm p13af_1080p60_60s_av.mp4 "--ei kivo.clock_rate_ppm 5000 --es kivo.drift_gate off"
run_group "3/4" p14w-60s-gate-on-control p13af_1080p60_60s_av.mp4 ""
run_group "4/4" p14w-60s-4k30-gate-on-5000ppm p13af_4k30_60s_av.mp4 "--ei kivo.clock_rate_ppm 5000"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 摘要 ──"
for name in p14w-60s-gate-on-5000ppm p14w-60s-gate-off-5000ppm p14w-60s-gate-on-control p14w-60s-4k30-gate-on-5000ppm; do
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
health = pace.get("clock_health", {})
gate = d.get("drift_gate", {})
trace = pace.get("clock_lead_trace", {})
print(f"{name}: /{str(d.get('schema','')).split('/')[-1]}  "
      f"gate={'on' if gate.get('enabled') else 'off'}  "
      f"rebases={health.get('drift_rebases')}  "
      f"lead={trace.get('last_ns')}  max={trace.get('max_ns')}  "
      f"reach={gate.get('limit_reach_s')}  dropped={d.get('frames_dropped')}")
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14w/*-sync.json"
