#!/bin/sh
# P14-AD：台阶幅度扫描 —— "不还"的代价从哪一段开始付。
#
# 自变量只有幅度：0 / 120 / 300 / 800 ms 正台阶，速率注入关掉（rate_ppm = 0），
# 否则 5000 ppm 在 60 s 里自己攒出的 300 ms 会和幅度混在一起。
#
# 两臂：`step_repay` on（今天的行为）/ off（P14-Z 的探针）。
# 对照（幅度 0）每臂 1 轮，其余每格 2 轮 —— 零点必须真实存在。
#
# 不测 ≥ 1 s：`jump_limit_ns = 1 s`，跨过去会触发 `jump_rebased`，那是另一种机制。
# 不测负台阶：倒退钳制会吞掉它（P14-Y 的 c2 已量过）。
set -u
A="C:/Android/sdk/platform-tools/adb.exe"
DEV="-s b6e950ee"
MEDIA=/storage/emulated/0/Android/data/com.kivo.cinema/files/media
OUT="C:/kivo video/docs/evidence/p14ad"
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
STEP_AT=20000

run_group " 1/14 对照 A" p14ad-0ms-a1   "$CLIP" ""
run_group " 2/14 对照 B" p14ad-0ms-b1   "$CLIP" "--es kivo.step_repay off"

for amp in 120 300 800; do
  for r in 1 2; do
    run_group " A ${amp}ms r${r}" "p14ad-${amp}ms-a${r}" "$CLIP" \
      "--ei kivo.clock_offset_ms ${amp} --ei kivo.clock_offset_at_ms ${STEP_AT}"
    run_group " B ${amp}ms r${r}" "p14ad-${amp}ms-b${r}" "$CLIP" \
      "--ei kivo.clock_offset_ms ${amp} --ei kivo.clock_offset_at_ms ${STEP_AT} --es kivo.step_repay off"
  done
done

MSYS_NO_PATHCONV=1 "$A" $DEV shell "svc power stayon false" >/dev/null 2>&1

echo ""
echo "── 曲线 ──"
for name in p14ad-0ms-a1 p14ad-0ms-b1 \
            p14ad-120ms-a1 p14ad-120ms-a2 p14ad-120ms-b1 p14ad-120ms-b2 \
            p14ad-300ms-a1 p14ad-300ms-a2 p14ad-300ms-b1 p14ad-300ms-b2 \
            p14ad-800ms-a1 p14ad-800ms-a2 p14ad-800ms-b1 p14ad-800ms-b2; do
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
jitter = d["present_jitter"]
trace = pace["clock_lead_trace"]
inj = d.get("clock_injection") or {}
print(
    f"{name:20s} /{str(d.get('schema','')).split('/')[-1]} "
    f"repay={'on ' if gate.get('step_repay') else 'off'} "
    f"inj={int(inj.get('offset_ns', 0))//1000000:4d}ms  "
    f"drop={d['frames_dropped']:3d} undeliv={pace['undelivered_frames']:3d} "
    f"due_step={pace['due_step_frames']:3d}  "
    f"lead={(pace['clock_lead_ns'] or 0)/1e6:8.2f} lead_max={(trace['max_ns'] or 0)/1e6:8.2f}  "
    f"corr={(health['drift_max_correction_ns'] or 0)/1e6:8.2f} rebases={health['drift_rebases']:3d}  "
    f"offset={(disc['offset_ns'] or 0)/1e6:8.2f} ev={disc['events']:2d}  "
    f"jit_max={(jitter['max'] or 0)/1e6:6.2f} over={pace.get('over_budget_frames')}"
)
PY
done
echo ""
echo "下一步：python tools/verify_p12c_sync.py docs/evidence/p14ad/*-sync.json"
