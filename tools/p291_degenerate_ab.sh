#!/bin/bash
# 291 · 退化连接可控 A/B（一次装包、两个开关、四臂归因）
#
# 【为什么不用公网】p290-live 实测公网吞吐在 253~566 KB/s 之间漂移，
#   跨臂对照会被源站波动污染（方法论 2/3）。本脚本用本地装置按精确速率供片，
#   并**确定性注入**一条退化连接（每 N 个主 chunk 降到 0.04 MB/s）复现
#   127.8s 冻结 ⇒ 两臂除了开关之外条件完全相同。
#
# 【判据】看门狗臂应在 ~6-12s 内留下 `origin 连接退化` 日志并重开一块；
#   对照臂会卡在那一块 4MiB/0.04MB/s ≈ 105s（soTimeout=30s 永不触发，
#   因为装置每 <2s 就吐一个 64KB chunk）。卡在最后一块时没有下一块 ⇒
#   相邻间隔算不出断层，必须补「末尾悬停」判据。
#
# 【★ 禁续播】watch.json 按 URL 存续播位。同一 URL 重跑会从上一臂进度开始
#   （290 实测 start=93/188/231s 一路漂移，后两臂跑出本地片段范围 ⇒
#   FALLBACK 到上游只回 35 字节，数据作废）。
#   ⇒ 每臂用**不同端口 + run/arm query**，URL 全新；装置不解析 path，供片相同。
#
# 【短窗口边界】本脚本验证的是**确定性机制是否被触发**，不是整体流畅度结论；
#   整体长稳仍须公网 ≥240s A/B（279 §九）。本地片段仅 48 MiB，
#   @0.6MB/s 从头播约 100s 内不越界（续播被禁后实测成立）。
#
# 【用法】bash tools/p291_degenerate_ab.sh [每臂秒数=100]
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
PKG=com.kivo.cinema
ACT="$PKG/.ui.compose.PlayerComposeActivity"
PORT_BASE=8134
RUN_ID="$(date +%s)"
PORT=$PORT_BASE
SECS="${1:-100}"
MBPS=0.6
DEG_EVERY=3
DEG_MBPS=0.04
DEG_AFTER=12
SIZE=1935985234
cd "C:/kivo video" || exit 1
OUTROOT="docs/evidence/p274-startup/p291-degenerate"
mkdir -p "$OUTROOT"
MAIN="docs/evidence/p274-local-origin/main.bin"
HEAD="docs/evidence/p274-local-origin/sample.mkv"
TAIL="docs/evidence/p274-local-origin/tail.bin"

cleanup() {
  [ -n "${ORIGIN_PID:-}" ] && kill "$ORIGIN_PID" 2>/dev/null
  $ADB shell "setprop debug.kivo.originKeepAlive 0" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.stallWatchdog on" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.supplyPressure on" >/dev/null 2>&1
}
trap cleanup EXIT INT TERM

start_origin() {
  local label="$1"
  node tools/p274_local_origin.mjs --port "$PORT" --mbps "$MBPS" \
    --file "$MAIN" --file-start 20971520 --size "$SIZE" \
    --head-file "$HEAD" --head-start 0 \
    --tail-file "$TAIL" --tail-start 1931790930 \
    --degenerate-every "$DEG_EVERY" --degenerate-mbps "$DEG_MBPS" \
    --degenerate-after-sec "$DEG_AFTER" \
    --log "$OUTROOT/origin-$label.log" > "$OUTROOT/node-$label.out" 2>&1 &
  ORIGIN_PID=$!
  sleep 3
  $ADB reverse "tcp:$PORT" "tcp:$PORT" >/dev/null
}

stop_origin() {
  kill "$ORIGIN_PID" 2>/dev/null
  wait "$ORIGIN_PID" 2>/dev/null
  ORIGIN_PID=""
}

run_arm() {
  local label="$1" stall="$2" supply="$3" idx="$4"
  PORT=$((PORT_BASE + idx))
  local out="$OUTROOT/$label"
  mkdir -p "$out"
  start_origin "$label"
  $ADB shell am force-stop "$PKG" >/dev/null 2>&1
  sleep 2
  $ADB shell "setprop debug.kivo.originKeepAlive 0"
  $ADB shell "setprop debug.kivo.stallWatchdog $stall"
  $ADB shell "setprop debug.kivo.supplyPressure $supply"
  $ADB shell "run-as $PKG sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
  # run/arm query 只用于让 watch.json 不命中续播；装置不解析 path，供片完全相同。
  $ADB shell "am start -n $ACT --es kivo.playurl 'http://127.0.0.1:$PORT/v?run=$RUN_ID&arm=$label' \
    --el kivo.playSize $SIZE --es kivo.label '$label' --ez kivo.gpuNext true" >/dev/null 2>&1
  echo "  [$label] port=$PORT stall=$stall supply=$supply 采样 ${SECS}s"
  sleep "$SECS"
  $ADB exec-out "run-as $PKG cat files/net-diag.log" > "$out/net.log" 2>/dev/null
  $ADB exec-out "run-as $PKG cat files/mpv-kernel.log" > "$out/kernel.log" 2>/dev/null
  $ADB exec-out "run-as $PKG cat files/mpv-kernel.log.diag" > "$out/diag.log" 2>/dev/null
  stop_origin
}

echo "===== 291 退化连接 4 臂 @ ${SECS}s/臂 run=$RUN_ID ====="
run_arm base   off off 0
run_arm stall  on  off 1
run_arm supply off on  2
run_arm both   on  on  3

echo "===== 判据汇总 ====="
python3 - "$OUTROOT" <<'PY'
import os, re, sys
root = sys.argv[1]
print(f"{'臂':<8} {'注入':>4} {'换连接':>6} {'主chunk':>7} {'最大断层':>9} {'末尾悬停':>9} {'buffering':>10} {'warmup':>7}  FALLBACK")
for arm in ['base', 'stall', 'supply', 'both']:
    p = os.path.join(root, arm)
    net = open(os.path.join(p, 'net.log'), encoding='utf-8', errors='ignore').read().splitlines()
    ker = open(os.path.join(p, 'kernel.log'), encoding='utf-8', errors='ignore').read()
    swaps = sum(1 for l in net if 'origin 连接退化' in l)
    warm = sum(1 for l in net if 'origin warmup hdrMs' in l)
    ol = open(os.path.join(root, 'origin-%s.log' % arm), encoding='utf-8', errors='ignore').read().splitlines()
    deg = sum(1 for l in ol if 'DEGENERATE#' in l)
    fb = sum(1 for l in ol if 'FALLBACK#' in l)
    rows, last_t = [], 0
    for l in net:
        m0 = re.match(r'^(\d+)\s', l)
        if m0: last_t = int(m0.group(1))
        m = re.search(r'^(\d+)\s+\[play\] origin chunk', l)
        if m: rows.append(int(m.group(1)))
    gaps = [(rows[i] - rows[i-1]) / 1000 for i in range(1, len(rows))]
    maxgap = max(gaps) if gaps else 0.0
    # ★ 卡在最后一块时没有下一块 ⇒ 相邻间隔算不出「断层」；必须补「末尾悬停」。
    tail = (last_t - rows[-1]) / 1000 if rows else 0.0
    waits = re.findall(r'End buffering \(waited ([\d.]+) secs\)', ker)
    print(f"{arm:<8} {deg:>4} {swaps:>6} {len(rows):>7} {maxgap:>8.1f}s {tail:>8.1f}s "
          f"{len(waits):>10} {warm:>7}  {fb}")
print("\n注：注入>0 且换连接>0、末尾悬停 << 对照臂 = 看门狗救回；")
print("    对照臂应卡在退化块直到窗口结束（末尾悬停 ≈ 剩余时长）。")
print("    FALLBACK>0 的臂数据无效（跑出本地片段范围），须排除。")
PY
