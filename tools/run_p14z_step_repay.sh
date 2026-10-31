#!/bin/sh
# P14-Z：台阶还掉之后，误差落在谁头上 —— 两臂对照。
#
# 同一个注入（+120 ms @20 s）、同一个片源、门都开着，只差一个开关：
#   A 臂 `step_repay = on` （今天的行为）：门把台阶还掉，画面平滑，A/V 差常驻；
#   B 臂 `step_repay = off`（探针）    ：台阶不进门，A/V 对齐，时间轴跳一下。
#
# 看四组数：`clock_lead_ns`（A/V 差）、`drift_max_correction_ns`（还的时候有多狠）、
# `frames_dropped`、`present_jitter.max`（画面有没有被打断）。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14z"
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
INJECT="--ei kivo.clock_rate_ppm 5000 --ei kivo.clock_offset_ms 120 --ei kivo.clock_offset_at_ms 20000"

run_group "1/4 A" p14z-a1 "$CLIP" "$INJECT"
run_group "2/4 A" p14z-a2 "$CLIP" "$INJECT"
run_group "3/4 B" p14z-b1 "$CLIP" "$INJECT --es kivo.step_repay off"
run_group "4/4 B" p14z-b2 "$CLIP" "$INJECT --es kivo.step_repay off"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 摘要 ──"
for name in p14z-a1 p14z-a2 p14z-b1 p14z-b2; do
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
jitter = d.get("present_jitter", {})
print(f"{name}: /{str(d.get('schema','')).split('/')[-1]}  "
      f"step_repay={'on' if gate.get('step_repay') else 'off'}  "
      f"rebases={health.get('drift_rebases')}  "
      f"max_corr_ms={(health.get('drift_max_correction_ns') or 0)/1e6:.2f}  "
      f"disc={disc.get('events')}  offset_ms={(disc.get('offset_ns') or 0)/1e6:.2f}  "
      f"lead_ms={(pace.get('clock_lead_ns') or 0)/1e6:.2f}  "
      f"jitter_max_ms={(jitter.get('max') or 0)/1e6:.2f}  "
      f"dropped={d.get('frames_dropped')}")
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14z/*-sync.json"
