#!/bin/bash
# 274 · 确定性源站实验：在**速率可控**的本地源上跑播放，测架构效率
#
# 【为什么需要】真实源站速度随机波动 100~1700 KB/s ⇒ 跨轮次不可比。
#   本装置用本地片源片段按精确速率供给（误差 < 0.3%）⇒ 条件完全确定。
#
# 【用法】bash tools/p274_lab.sh <标签> <mbps> [秒数]
set -u
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
LABEL="${1:-lab}"
MBPS="${2:-0.35}"
SECS="${3:-60}"
PORT=8123
URL_LOCAL="http://127.0.0.1:$PORT/v"
cd "C:/kivo video" || exit 1
OUT="docs/evidence/p274-local-origin/$LABEL"
mkdir -p "$OUT"

# 1) 重启装置（换速率）
taskkill //F //IM node.exe >/dev/null 2>&1
sleep 2
node tools/p274_local_origin.mjs --port $PORT --mbps "$MBPS" \
  --file docs/evidence/p274-local-origin/main.bin --file-start 20971520 --size 1935985234 \
  --log "docs/evidence/p274-local-origin/origin-$LABEL.log" --head-file docs/evidence/p274-local-origin/sample.mkv --head-start 0 --tail-file docs/evidence/p274-local-origin/tail.bin --tail-start 1931790930 >/dev/null 2>&1 &
sleep 3
$ADB reverse tcp:$PORT tcp:$PORT >/dev/null

# 2) 起播
$ADB shell am force-stop com.kivo.cinema
sleep 2
$ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
$ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "$URL_LOCAL" --es kivo.embyToken "$TOKEN" --el kivo.playSize 1935985234 \
  --es kivo.label "$LABEL" >/dev/null 2>&1
sleep "$SECS"

# 3) 采集
$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$OUT/kernel.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$OUT/diag.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$OUT/net.log" 2>/dev/null

# 4) 复算
echo "===== $LABEL @ ${MBPS} MB/s ====="
node tools/p273_gaps.mjs "$OUT/kernel.log" "$OUT/diag.log" 2>&1 \
  | grep -E "Enter buffering|实际帧率|渲染间隔|Opening done" | sed 's/^/  /'
python - "$OUT/net.log" <<'PY'
import re, sys
lines = open(sys.argv[1], encoding='utf-8', errors='ignore').read().split('\n')
ch = [l for l in lines if '[play] origin chunk' in l]
tot = sum(int(m.group(1)) for l in ch if (m := re.search(r'len=(\d+)', l)))
reqs = sum(1 for l in lines if 'PROXY REQ' in l)
print(f"  预读块 {len(ch)} 个 / {tot/1048576:.1f} MiB · 内核请求 {reqs} 次")
PY
grep "DONE" "docs/evidence/p274-local-origin/origin-$LABEL.log" 2>/dev/null | \
  awk '{for(i=1;i<=NF;i++) if($i ~ /^bytes=/) b=$i} END{}' >/dev/null
echo "  装置侧供出：$(grep -c 'DONE' "docs/evidence/p274-local-origin/origin-$LABEL.log" 2>/dev/null) 次请求"
