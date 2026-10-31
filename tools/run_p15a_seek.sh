#!/bin/sh
# P15-A（G8）：seek 的证据。三轮，覆盖 assess_seek() 的三条分支。
#
#   1 无 seek   → requests=0、pass=false、reason 非空（"没 seek 不能判通过"）
#   2 向前 seek → 5 s 时跳到 20 s（PREVIOUS_SYNC 应落在 ≤ 20 s 的关键帧上）
#   3 向后 seek → 20 s 时跳回 5 s（同上，但要跨过已解码的一段）
#
# 判据 AS1 只看"事实与判定自洽"，阈值（150 ms）由 assess_seek() 自己判。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p15a"
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

run_group "1/3 无 seek"  p15a-noseek-a1   "$CLIP" ""
run_group "2/3 向前 seek" p15a-fwd20-a1    "$CLIP" "--ei kivo.seek.ms 20000 --ei kivo.seek.at.ms 5000"
run_group "3/3 向后 seek" p15a-back5-a1    "$CLIP" "--ei kivo.seek.ms 5000  --ei kivo.seek.at.ms 20000"

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── seek 事实 ──"
for name in p15a-noseek-a1 p15a-fwd20-a1 p15a-back5-a1; do
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
sk = (d.get("video") or {}).get("seek") or {}
a = sk.get("assessment") or {}
print(f"{name:16s} /{str(d.get('schema','')).split('/')[-1]}  "
      f"req={sk.get('requests')} target={sk.get('target_us')} landing={sk.get('landing_us')} "
      f"prepare={sk.get('prepare_us')} first={sk.get('first_frame_us')} "
      f"total={a.get('total_us')} pass={a.get('pass')}")
if a.get("reason"):
    print("    reason:", a["reason"][:90])
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p15a/*-sync.json"
