#!/bin/sh
# P14-AC：`av_drift` 自带使用条件 —— 两轮（一轮在门槛内、一轮在门槛外）。
#
# 判据 AC1/AC2 要的是一对反例：
#   · 不注入：领先量应在一帧以内	⇒ `sync_verdict = av_synced`
#   · 5000 ppm：60 s 上是 ~40 ms，超过 30 fps 的一帧	⇒ `clock_off`
# 两轮都跑 4K30 60 s，只差注入，别的完全一样。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14ac"
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
run_group "1/2 不注入" p14ac-control "$CLIP" ""
run_group "2/2 5000ppm" p14ac-5000ppm "$CLIP" "--ei kivo.clock_rate_ppm 5000"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 摘要 ──"
for name in p14ac-control p14ac-5000ppm; do
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
a = d.get("av_drift", {})
print(f"{name}: /{str(d.get('schema','')).split('/')[-1]}  "
      f"verdict={a.get('sync_verdict')}  lead_ms={(a.get('lead_ns') or 0)/1e6:.2f}  "
      f"bound_ms={(a.get('lead_bound_ns') or 0)/1e6:.2f}  "
      f"p99_ms={((((a.get('quantiles') or {}).get('media_time_decimated') or {}).get('p99')) or 0)/1e6:.3f}  "
      f"reason={a.get('sync_verdict_reason')!r}")
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14ac/*-sync.json"
