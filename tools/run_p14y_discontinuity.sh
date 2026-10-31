#!/bin/sh
# P14-Y：钟的不连续那本账 —— 真机五轮。
#
# 与 P14-X 同参数（4K30 60 s / 5000 ppm / 门开）四轮，外加一轮门关对照。
# 要看的是同一个问题在两个层面上分别怎么说：
#   · `drift_discontinuity` 有没有把"停更—补回来"那一刻记下来（events / offset / max.kind）；
#   · 那笔大修正是否与它同轮（Y3），以及门照旧把它还掉之后残差落在哪。
#
# 门关那一轮不是"对照组好看"：记账若被门的状态影响，那些账就不可比。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14y"
mkdir -p "$OUT"

# 屏幕别睡（P14-P 栽过：2.8 秒熄灭 → presented 停在 84）
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

# 60 s 片源 + 收尾：给 150 s 上限（P14-W 第一次就是按 8 s 的节奏等 30 s，取到空文件）
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

CLIP=p13af_4k30_60s_av.mp4
INJECT="--ei kivo.clock_rate_ppm 5000"

run_group "1/7" p14y-4k30-r1 "$CLIP" "$INJECT"
run_group "2/7" p14y-4k30-r2 "$CLIP" "$INJECT"
run_group "3/7" p14y-4k30-r3 "$CLIP" "$INJECT"
run_group "4/7" p14y-4k30-r4 "$CLIP" "$INJECT"
run_group "5/7" p14y-4k30-r5-gate-off "$CLIP" "$INJECT --es kivo.drift_gate off"

# 正向对照（6/7、7/7）：**没有对照，0 事件与"仪器从不响"分不开**。
# P14-K 记过一笔"沉默的零"的账，这里不再欠第二次。
# 注入的是一次数台阶：`clock_offset_at_ms` 之后，位置一次加上 `clock_offset_ms`。
STEPS="--ei kivo.clock_offset_ms 120 --ei kivo.clock_offset_at_ms 20000"
run_group "6/7" p14y-4k30-c1-step-ahead "$CLIP" "$INJECT $STEPS"
run_group "7/7" p14y-4k30-c2-step-back "$CLIP" "$INJECT --ei kivo.clock_offset_ms -120 --ei kivo.clock_offset_at_ms 20000"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 摘要 ──"
for name in p14y-4k30-r1 p14y-4k30-r2 p14y-4k30-r3 p14y-4k30-r4 p14y-4k30-r5-gate-off p14y-4k30-c1-step-ahead p14y-4k30-c2-step-back; do
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
disc = d.get("drift_discontinuity", {})
print(f"{name}: /{str(d.get('schema','')).split('/')[-1]}  "
      f"gate={'on' if gate.get('enabled') else 'off'}  "
      f"rebases={health.get('drift_rebases')}  "
      f"max_correction_ms={(health.get('drift_max_correction_ns') or 0)/1e6:.2f}  "
      f"disc_events={disc.get('events')}  "
      f"offset_ms={(disc.get('offset_ns') or 0)/1e6:.2f}  "
      f"max_kind={disc.get('max',{}).get('kind')}  "
      f"dropped={d.get('frames_dropped')}")
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14y/*-sync.json"
