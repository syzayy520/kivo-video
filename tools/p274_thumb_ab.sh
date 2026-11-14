#!/bin/bash
# 274 · 预览 chunk 大小 A/B（装置上确定性实验）
#
# 【要验证的假设】thumb 标签下了 8 MiB 只为取 1 帧。
#   上一轮证伪了"缩环"（carry 恒等于 chunkBytes，缩环无效）。
#   ⇒ 缩 chunk 会同步缩 carry ⇒ 每条连接少下差额字节。
#
# 【用法】bash tools/p274_thumb_ab.sh <标签> <thumbChunkKb> [落点秒数列表]
set -u
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
LABEL="${1:-thumb-ab}"
CHUNK_KB="${2:-0}"
SECS_LIST="${3:-10,30,50}"
PORT=8123
URL_LOCAL="http://127.0.0.1:$PORT/v"
cd "C:/kivo video" || exit 1
OUT="docs/evidence/p274-local-origin/$LABEL"
mkdir -p "$OUT"

taskkill //F //IM node.exe >/dev/null 2>&1
sleep 2
node tools/p274_local_origin.mjs --port $PORT --mbps 0.35 \
  --file docs/evidence/p274-local-origin/main.bin --file-start 20971520 --size 1935985234 \
  --log "docs/evidence/p274-local-origin/origin-$LABEL.log" \
  --head-file docs/evidence/p274-local-origin/sample.mkv --head-start 0 \
  --tail-file docs/evidence/p274-local-origin/tail.bin --tail-start 1931790930 >/dev/null 2>&1 &
sleep 3
$ADB reverse tcp:$PORT tcp:$PORT >/dev/null

$ADB shell am force-stop com.kivo.cinema
sleep 2
$ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
$ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "$URL_LOCAL" --es kivo.embyToken "$TOKEN" --el kivo.playSize 1935985234 \
  --es kivo.landSeekSecs "$SECS_LIST" --ei kivo.landSeekDelayMs 9000 \
  --ei kivo.thumbChunkKb "$CHUNK_KB" --ei kivo.thumbMaxRangeKb "${MAXRANGE_KB:-0}" --es kivo.label "$LABEL" >/dev/null 2>&1
sleep 45

$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$OUT/kernel.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$OUT/diag.log" 2>/dev/null
$ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$OUT/net.log" 2>/dev/null

echo "===== $LABEL · thumbChunkKb=$CHUNK_KB ====="
node tools/p273_gaps.mjs "$OUT/kernel.log" "$OUT/diag.log" 2>&1 \
  | grep -E "Enter buffering|实际帧率|渲染间隔 max|渲染间隔 p99" | sed 's/^/  /'
python - "$OUT/net.log" <<'PY'
import re, sys, collections
lines = open(sys.argv[1], encoding='utf-8', errors='ignore').read().split('\n')
by = collections.defaultdict(lambda: [0,0])
for l in lines:
    m = re.search(r'\[([a-z-]+)\] origin chunk from=(\d+).*len=(\d+)', l)
    if m:
        by[m.group(1)][0] += 1; by[m.group(1)][1] += int(m.group(3))
tot = sum(b for _,b in by.values())
for k,(n,b) in sorted(by.items(), key=lambda x:-x[1][1]):
    print(f"  {k:11s} {n:3d} 次 {b/1048576:7.2f} MiB")
print(f"  合计 {tot/1048576:.2f} MiB")
PY
