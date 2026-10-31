#!/bin/sh
# P14-AF：出版 `jump_max_ns` + 跳变轮的 `sync_verdict` 改 unknown。
#
# 五轮，每一轮都有明确用途：
#   0 ms    A  对照：没有跳变、没有台阶 —— `jump_max_ns` 必须是 0，verdict 仍是 av_synced
#   800 ms  A  台阶轮：与 P14-AD 的 800 ms 同参数 —— 数值不许被这一卡改掉（只多一格）
#   999 ms  B  台阶轮（不还）：`jump_max_ns` 必须 0，verdict 仍是 clock_off
#   1500 A/B   跳变轮：`jump_max_ns` ≈ ±1500，verdict 必须是 unknown
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14af"
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
  echo "[$label] $name  ${extra:-（无注入）}"
  start_app "$name" "$clip" "$extra"
  used=$(wait_for_report "$name" 150)
  pull_evidence "$name"
  echo "      用时 ${used}s"
}

CLIP=p13af_4k30_60s_av.mp4
AT=20000

run_group "1/5 对照"   p14af-0ms-a1    "$CLIP" ""
run_group "2/5 台阶"   p14af-800ms-a1  "$CLIP" "--ei kivo.clock_offset_ms 800  --ei kivo.clock_offset_at_ms $AT"
run_group "3/5 台阶B"  p14af-999ms-b1  "$CLIP" "--ei kivo.clock_offset_ms 999  --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"
run_group "4/5 跳变A"  p14af-1500ms-a1 "$CLIP" "--ei kivo.clock_offset_ms 1500 --ei kivo.clock_offset_at_ms $AT"
run_group "5/5 跳变B"  p14af-1500ms-b1 "$CLIP" "--ei kivo.clock_offset_ms 1500 --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 结果 ──"
for name in p14af-0ms-a1 p14af-800ms-a1 p14af-999ms-b1 p14af-1500ms-a1 p14af-1500ms-b1; do
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
disc = d["drift_discontinuity"]
drift = d["av_drift"]
print(
    f"{name:16s} /{str(d.get('schema','')).split('/')[-1]}  "
    f"jump={health['jump_rebases']:2d}  jump_max={(disc.get('jump_max_ns') or 0)/1e6:9.2f}ms  "
    f"offset={(disc['offset_ns'] or 0)/1e6:8.2f}  lead={(pace['clock_lead_ns'] or 0)/1e6:9.2f}  "
    f"drop={d['frames_dropped']:3d}  verdict={drift.get('sync_verdict')}"
)
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14af/*-sync.json"
