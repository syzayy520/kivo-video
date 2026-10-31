#!/bin/sh
# P14-AA：at_time 档第一次有跨度和 DueStep —— 真机两轮。
#
# 这一档以前从不喂 DueStep：`due_step_supplied = 0`、四跨度全 0、`media_rate` 算不出，
# 而报告里那看起来像"没有位移"。这一卡补的就是它。
#
# 要看三格：
#   · `pace.span_frames` > 0 且 `pace.span_basis = scheduled`
#   · `timeline.media_rate_measured = true`（第一次在这一档上成立）
#   · `pace.clock_supplied_frames` 仍是 0（presented 的口径没被污染）
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14aa"
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
  # 不传 `kivo.present`：默认就是 surface_at_time（这一卡要的就是那一档）。
  # shellcheck disable=SC2086
  MSYS_NO_PATHCONV=1 "$A" $DEV shell "am start -n com.kivo.cinema/.ui.PlayerActivity --es kivo.media $MEDIA/$clip --ei kivo.pace 1 --ei kivo.drop 1 $extra --es kivo.evidence $name.json" >/dev/null 2>&1
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
  echo "[$label] $name"
  start_app "$name" "$clip" "$extra"
  used=$(wait_for_report "$name" 150)
  pull_evidence "$name"
  echo "      用时 ${used}s"
}

CLIP=p13af_4k30_60s_av.mp4
run_group "1/2 at_time" p14aa-4k30-r1 "$CLIP" "--ei kivo.clock_rate_ppm 5000"
run_group "2/2 at_time" p14aa-4k30-r2 "$CLIP" "--ei kivo.clock_rate_ppm 5000"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 摘要 ──"
for name in p14aa-4k30-r1 p14aa-4k30-r2; do
  f="$OUT/$name-sync.json"
  [ -s "$f" ] || { echo "$name: 没有文件"; continue; }
  python - "$f" "$name" <<'PY'
import json, sys
path, name = sys.argv[1], sys.argv[2]
try:
    d = json.load(open(path, encoding="utf-8"))
except Exception as error:
    print(f"{name}: 读不出（{error}）")
    raise SystemExit(0)
pace = d.get("pace", {})
tl = d.get("timeline", {})
print(f"{name}: /{str(d.get('schema','')).split('/')[-1]}  "
      f"present={pace.get('presented')}  "
      f"span_frames={pace.get('span_frames')}  "
      f"basis={pace.get('span_basis')}  "
      f"csf={pace.get('clock_supplied_frames')}  "
      f"due_supplied={pace.get('due_step_supplied')}  "
      f"pts_span_ms={(pace.get('pts_span_ns') or 0)/1e6:.1f}  "
      f"media_rate={tl.get('media_rate')}  measured={tl.get('media_rate_measured')}  "
      f"reason={tl.get('media_rate_reason')!r}")
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14aa/*-sync.json"
