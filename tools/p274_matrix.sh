#!/bin/bash
# 274 · 参数矩阵实验（在确定性本地源上）
#
# 【为什么能一次跑多组】两个旋钮都是 intent 参数 ⇒ 一次编译安装即可，
#   每组只改 intent ⇒ 不必反复编译。
#
# 【用法】bash tools/p274_matrix.sh <mbps> [秒数]
set -u
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
MBPS="${1:-0.25}"
SECS="${2:-55}"
PORT=8123
cd "C:/kivo video" || exit 1
OUTROOT="docs/evidence/p274-local-origin"

# 装置：固定速率
taskkill //F //IM node.exe >/dev/null 2>&1
sleep 2
node tools/p274_local_origin.mjs --port $PORT --mbps "$MBPS" \
  --file "$OUTROOT/main.bin" --file-start 20971520 --size 1935985234 \
  --head-file "$OUTROOT/sample.mkv" --head-start 0 --tail-file "$OUTROOT/tail.bin" --tail-start 1931790930 \
  --log "$OUTROOT/origin-matrix-$MBPS.log" >/dev/null 2>&1 &
sleep 3
$ADB reverse tcp:$PORT tcp:$PORT >/dev/null

run_one() {
  local label="$1"; local demux="$2"; local range="$3"; local ring="$5"
  local out="$OUTROOT/$label"
  mkdir -p "$out"
  $ADB shell am force-stop com.kivo.cinema
  sleep 2
  $ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
  $ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
    --es kivo.playurl "http://127.0.0.1:$PORT/v" --es kivo.embyToken "$TOKEN" \
    --el kivo.playSize 1935985234 --es kivo.label "$label" \
    --es kivo.landSeekSecs "10,30,50" --ei kivo.landSeekDelayMs 9000 \
    --ei kivo.demuxerMb "$demux" --ei kivo.maxRangeMb "$range" --ei kivo.noWaitLanding "${4:-0}" --ei kivo.ringMb "${5:-0}" >/dev/null 2>&1
  sleep "$SECS"
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$out/kernel.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$out/diag.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$out/net.log" 2>/dev/null
  local fps buf dl
  fps=$(node tools/p273_gaps.mjs "$out/kernel.log" "$out/diag.log" 2>&1 | grep "实际帧率" | grep -oE "[0-9.]+")
  buf=$(node tools/p273_gaps.mjs "$out/kernel.log" "$out/diag.log" 2>&1 | grep "Enter buffering" | grep -oE "[0-9]+")
  dl=$(python - "$out/net.log" <<'PY'
import re, sys
lines = open(sys.argv[1], encoding='utf-8', errors='ignore').read().split('\n')
ch = [l for l in lines if '[play] origin chunk' in l]
tot = sum(int(m.group(1)) for l in ch if (m := re.search(r'len=(\d+)', l)))
print(f"{tot/1048576:.1f}")
PY
)
  printf "  %-22s demux=%-3s range=%-3s ⇒ fps=%-6s 缓冲=%-3s 下载=%s MiB\n" \
    "$label" "$demux" "$range" "${fps:-n/a}" "${buf:-n/a}" "$dl"
}

echo "===== 矩阵 @ ${MBPS} MB/s ====="
run_one "mx-${MBPS}-ring8"  0 0 0 0
run_one "mx-${MBPS}-ring4"  0 0 0 4
run_one "mx-${MBPS}-ring2"  0 0 0 2
run_one "mx-${MBPS}-ring1"  0 0 0 1
