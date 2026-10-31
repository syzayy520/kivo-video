#!/bin/sh
# P14-AE：≥ 1 s 的台阶 —— 跨过 jump_limit 之后，门把它当 seek 处理。
#
# 变量只有幅度：999 / 1000 / 1500 / 3000 ms 正台阶，rate_ppm = 0。
#   999 与 1000 是悬崖两侧（判定是严格 `>`）：前者该走台阶 + 门还它，后者可能走跳变重锚。
#   1500 / 3000 一定走跳变分支：位置按墙推进，台阶被丢掉，只留一个 jump_rebases 计数。
#
# 两臂都跑：H2 说 `step_repay` 在跳变分支上无效（跳变在门之前就把台阶扔了）—— 要证伪它。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14ae"
mkdir -p "$OUT"

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
  echo "[$label] $name  $extra"
  start_app "$name" "$clip" "$extra"
  used=$(wait_for_report "$name" 150)
  pull_evidence "$name"
  echo "      用时 ${used}s"
}

CLIP=p13af_4k30_60s_av.mp4
AT=20000

run_group " 1/10 999 A"  p14ae-999ms-a1  "$CLIP" "--ei kivo.clock_offset_ms 999  --ei kivo.clock_offset_at_ms $AT"
run_group " 2/10 999 B"  p14ae-999ms-b1  "$CLIP" "--ei kivo.clock_offset_ms 999  --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"
run_group " 3/10 1000 A" p14ae-1000ms-a1 "$CLIP" "--ei kivo.clock_offset_ms 1000 --ei kivo.clock_offset_at_ms $AT"
run_group " 4/10 1000 B" p14ae-1000ms-b1 "$CLIP" "--ei kivo.clock_offset_ms 1000 --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"
run_group " 5/10 1500 A" p14ae-1500ms-a1 "$CLIP" "--ei kivo.clock_offset_ms 1500 --ei kivo.clock_offset_at_ms $AT"
run_group " 6/10 1500 B" p14ae-1500ms-b1 "$CLIP" "--ei kivo.clock_offset_ms 1500 --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"
run_group " 7/10 1500 A" p14ae-1500ms-a2 "$CLIP" "--ei kivo.clock_offset_ms 1500 --ei kivo.clock_offset_at_ms $AT"
run_group " 8/10 1500 B" p14ae-1500ms-b2 "$CLIP" "--ei kivo.clock_offset_ms 1500 --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"
run_group " 9/10 3000 A" p14ae-3000ms-a1 "$CLIP" "--ei kivo.clock_offset_ms 3000 --ei kivo.clock_offset_at_ms $AT"
run_group "10/10 3000 B" p14ae-3000ms-b1 "$CLIP" "--ei kivo.clock_offset_ms 3000 --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 悬崖两侧 ──"
for name in p14ae-999ms-a1 p14ae-999ms-b1 p14ae-1000ms-a1 p14ae-1000ms-b1 \
            p14ae-1500ms-a1 p14ae-1500ms-b1 p14ae-1500ms-a2 p14ae-1500ms-b2 \
            p14ae-3000ms-a1 p14ae-3000ms-b1; do
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
pace = d["pace"]
health = pace["clock_health"]
gate = d["drift_gate"]
disc = d["drift_discontinuity"]
drift = d["av_drift"]
inj = d.get("clock_injection") or {}
print(
    f"{name:18s} /{str(d.get('schema','')).split('/')[-1]} "
    f"repay={'on ' if gate.get('step_repay') else 'off'} "
    f"inj={int(inj.get('offset_ns', 0))//1000000:5d}ms  "
    f"jump={health['jump_rebases']:2d} reb={health['drift_rebases']:2d} "
    f"corr={(health['drift_max_correction_ns'] or 0)/1e6:8.2f}  "
    f"lead={(pace['clock_lead_ns'] or 0)/1e6:9.2f}  "
    f"drop={d['frames_dropped']:3d}  "
    f"账 ev={disc['events']:2d} jumped={disc['jumped']:2d} stepped={disc['stepped']:2d} "
    f"offset={(disc['offset_ns'] or 0)/1e6:8.2f}  "
    f"verdict={drift.get('sync_verdict')}"
)
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14ae/*-sync.json"
