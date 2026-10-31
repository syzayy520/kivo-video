#!/bin/sh
# P14-AG：丢帧现场（dropped_events）。
#
# 三轮，每一轮都要回答一个具体问题：
#   0 ms   A  对照：0 丢帧 ⇒ total=0 / sampled=0 / truncated=false，数组空
#   800 ms B  不还台阶：AD 量到 24 帧 ⇒ 看它是"一簇"还是"两簇"，以及 truncated 是否正确
#   999 ms B  不还台阶：31 帧 ⇒ 第二组形状，验证环不会被这一轮的形状带偏
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14ag"
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

run_group "1/3 对照"  p14ag-0ms-a1   "$CLIP" ""
run_group "2/3 800B" p14ag-800ms-b1 "$CLIP" "--ei kivo.clock_offset_ms 800  --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"
run_group "3/3 999B" p14ag-999ms-b1 "$CLIP" "--ei kivo.clock_offset_ms 999  --ei kivo.clock_offset_at_ms $AT --es kivo.step_repay off"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 丢帧现场 ──"
for name in p14ag-0ms-a1 p14ag-800ms-b1 p14ag-999ms-b1; do
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
ev = d["dropped_events"]
recent = ev["recent"]
print(f"{name:16s} /{str(d.get('schema','')).split('/')[-1]}  "
      f"frames_dropped={d['frames_dropped']:3d}  total={ev['total']:3d} "
      f"sampled={ev['sampled']:2d} truncated={ev['truncated']}")
if recent:
    spans = [r["media_ns"] for r in recent]
    lateness = [r["lateness_ns"] for r in recent]
    print(f"    media {spans[0]/1e9:.2f}s → {spans[-1]/1e9:.2f}s "
          f"(跨 {(spans[-1]-spans[0])/1e6:.0f} ms)；lateness "
          f"{min(lateness)/1e6:.1f}~{max(lateness)/1e6:.1f} ms")
    print("    前 4 笔:", [(round(r['media_ns']/1e9, 3), round(r['lateness_ns']/1e6, 1)) for r in recent[:4]])
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14ag/*-sync.json"
