#!/bin/bash
# 292 · 起播零缓冲爬坡 可控 A/B（本地精确限速装置，禁公网波动）
#
# 【为什么用本地装置】p291-night 两臂相隔 6 分钟、源站表头 1~8s 随机波动
#   （方法论 2/3：性能数字必须带源站基线）。起播 7.767s 冻结到底是
#   「源站整体慢」还是「零缓冲起步 + 串行表头空窗」的调度缺陷，公网判不了。
#
# 【装置变量】--mbps 0.6（614KB/s，片源 5916s/1.94GB=327KB/s，余量 88% ⇒
#   吞吐充足，排除 275 §6.2 物理缺口）+ --chunk-hdr-ms 2500（每个主块前
#   2.5s 表头空窗，模拟慢源站连接但完全确定）。不注入退化连接（那是 291 的题）。
#
# 【复现机理】首帧时 demuxer runway≈0（MpvFirstFrameWait 一确认上屏就返回）。
#   串行补块：一块 4MiB 读完 → 归还连接 → 下一块付一次表头空窗，期间供给=0。
#   mpv cache-pause-wait 默认 1s：underrun 后只攒 1s 就恢复播放 ⇒ 恢复后
#   又撞上下一块 2.5s 空窗 ⇒ 连环 Enter/End buffering。
#
# 【判据】对照臂（rebuffer=off，cache-pause-wait 默认 1s）应出现起播 0-40s
#   内 >=2 次 buffering（攒 1s 就恢复、又撞下一块空窗 ⇒ 连环）；修复臂
#   （rebuffer=on，压力期目标 6s）应次数更少、总时长更短且不连环。
#   代价（首帧时间变长）必须单独量，不许只报好的一面。
#
# 【★ 禁续播】watch.json 按 URL 存续播位 ⇒ 每臂独立端口 + run/arm query。
#
# 【用法】规格 label:rebuffer[:wait[:initial[:prefetch]]]，空格分隔多臂
#   ARMS="ctl:off" bash tools/p292_startup_ab.sh 70                  # 对照（旧行为）
#   ARMS="off:off w2:on:2.0 w4:on:4.0 w4n:on:4.0:no" \
#     MBPS=0.6 CHUNK_HDR_MS=3500 bash tools/p292_startup_ab.sh 70   # 292 方案 C 扫描
#   ARMS="ctl:off pf:off:::on" \
#     MBPS=0.6 CHUNK_HDR_MS=3500 bash tools/p292_startup_ab.sh 70   # 293 方案 D 对照
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
PKG=com.kivo.cinema
ACT="$PKG/.ui.compose.PlayerComposeActivity"
PORT_BASE=8140
RUN_ID="$(date +%s)"
PORT=$PORT_BASE
SECS="${1:-60}"
MBPS="${MBPS:-0.6}"
# 293 · 保守对照：>0 时所有响应共享总带宽（模拟源站总带宽固定，不叠加）。
GLOBAL_MBPS="${GLOBAL_MBPS:-0}"
CHUNK_HDR_MS="${CHUNK_HDR_MS:-2500}"
SIZE=1935985234
ARMS="${ARMS:-base}"
cd "C:/kivo video" || exit 1
# 293 · 可覆盖：不同场景要分开留档，否则同一目录会被下一次跑覆盖（证据不再可复算）。
OUTROOT="${OUTROOT:-docs/evidence/p274-startup/p292-startup}"
mkdir -p "$OUTROOT"
# 293 · 可覆盖：长稳需要更长的本地片段（main.bin 仅 48MiB 只够 12 块）。
MAIN="${MAIN:-docs/evidence/p274-local-origin/main.bin}"
HEAD="docs/evidence/p274-local-origin/sample.mkv"
TAIL="docs/evidence/p274-local-origin/tail.bin"
ORIGIN_PID=""

cleanup() {
  [ -n "$ORIGIN_PID" ] && kill "$ORIGIN_PID" 2>/dev/null
  $ADB shell "setprop debug.kivo.originKeepAlive 0" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.stallWatchdog on" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.supplyPressure on" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.rebuffer off" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.rebufferWait 4.0" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.rebufferInitial yes" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.chunkPrefetch off" >/dev/null 2>&1
}
trap cleanup EXIT INT TERM

start_origin() {
  local label="$1"
  node tools/p274_local_origin.mjs --port "$PORT" --mbps "$MBPS" \
    --global-mbps "$GLOBAL_MBPS" \
    --file "$MAIN" --file-start 20971520 --size "$SIZE" \
    --head-file "$HEAD" --head-start 0 \
    --tail-file "$TAIL" --tail-start 1931790930 \
    --chunk-hdr-ms "$CHUNK_HDR_MS" \
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
  # $1 label  $2 mode(off|on)  $3 idx  $4 wait秒(可空)  $5 initial(yes|no, 可空)
  # $6 prefetch(on|off, 可空) —— 293 方案 D 开关，缺省 off
  local label="$1" mode="$2" idx="$3" wait="${4:-}" initial="${5:-}" pf="${6:-off}"
  PORT=$((PORT_BASE + idx))
  local out="$OUTROOT/$label"
  mkdir -p "$out"
  start_origin "$label"
  $ADB shell am force-stop "$PKG" >/dev/null 2>&1
  sleep 2
  $ADB shell "setprop debug.kivo.originKeepAlive 0"
  $ADB shell "setprop debug.kivo.stallWatchdog on"
  $ADB shell "setprop debug.kivo.supplyPressure on"
  # 292 · 三个属性逐臂显式设定（缺省用默认值 ⇒ 每臂条件完全确定）
  $ADB shell "setprop debug.kivo.rebuffer $mode"
  if [ -n "$wait" ]; then
    $ADB shell "setprop debug.kivo.rebufferWait $wait"
  else
    $ADB shell "setprop debug.kivo.rebufferWait 4.0"
  fi
  if [ -n "$initial" ]; then
    $ADB shell "setprop debug.kivo.rebufferInitial $initial"
  else
    $ADB shell "setprop debug.kivo.rebufferInitial yes"
  fi
  # 293 · 方案 D：每臂显式设定（缺省 off ⇒ 对照臂完全走旧路径）
  $ADB shell "setprop debug.kivo.chunkPrefetch $pf"
  $ADB shell "run-as $PKG sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
  $ADB shell "am start -n $ACT --es kivo.playurl 'http://127.0.0.1:$PORT/v?run=$RUN_ID&arm=$label' \
    --el kivo.playSize $SIZE --es kivo.label '$label' --ez kivo.gpuNext true" >/dev/null 2>&1
  echo "  [$label] port=$PORT rebuffer=$mode wait=${wait:-4.0} initial=${initial:-yes} prefetch=$pf 采样 ${SECS}s"
  sleep "$SECS"
  $ADB exec-out "run-as $PKG cat files/net-diag.log" > "$out/net.log" 2>/dev/null
  $ADB exec-out "run-as $PKG cat files/mpv-kernel.log" > "$out/kernel.log" 2>/dev/null
  $ADB exec-out "run-as $PKG cat files/mpv-kernel.log.diag" > "$out/diag.log" 2>/dev/null
  $ADB shell screencap -p //sdcard/p292.png >/dev/null 2>&1
  $ADB pull //sdcard/p292.png "$out/playing.png" >/dev/null 2>&1
  stop_origin
}

echo "===== 292 起播爬坡 arms=[$ARMS] @ ${SECS}s/臂 mbps=$MBPS globalMbps=$GLOBAL_MBPS chunkHdr=${CHUNK_HDR_MS}ms run=$RUN_ID ====="
i=0
for spec in $ARMS; do
  # 规格： label:rebuffer[:wait[:initial[:prefetch]]]
  #   例如 ctl:off / w4:on:4.0 / pf:off:::on（rebuffer 关、只开 prefetch）
  IFS=: read -r label mode wait initial pf <<< "$spec"
  [ -z "$label" ] && continue
  mode="${mode:-off}"
  run_arm "$label" "$mode" "$i" "$wait" "$initial" "$pf"
  i=$((i+1))
done

echo "===== 判据汇总（起播 0-40s）====="
python3 - "$OUTROOT" "$ARMS" <<'PY'
import os, re, sys
root, specs = sys.argv[1], sys.argv[2].split()
print(f"{'臂':<8} {'首帧':>7} {'bf次':>5} {'bf总时长':>9} {'最长':>7} {'首次underrun':>13} {'主chunk':>7} {'末runway':>9}  FALLBACK")
for spec in specs:
    arm = spec.split(':')[0]
    p = os.path.join(root, arm)
    net = open(os.path.join(p, 'net.log'), encoding='utf-8', errors='ignore').read().splitlines()
    ker = open(os.path.join(p, 'kernel.log'), encoding='utf-8', errors='ignore').read()
    ol  = open(os.path.join(root, 'origin-%s.log' % arm), encoding='utf-8', errors='ignore').read().splitlines()
    fb  = sum(1 for l in ol if 'FALLBACK#' in l)
    # 首帧绝对时刻（相对首行）
    first = None
    for l in net:
        m = re.match(r'^(\d+)\s', l)
        if m and '首帧就绪' in l: first = int(m.group(1)); break
    t0 = first if first else (int(re.match(r'^(\d+)', net[0]).group(1)) if net else 0)
    # buffering 事件：取 mpv 时钟 <=40s 的（起播窗口），waited 时长
    waits = [float(x) for x in re.findall(r'End buffering \(waited ([\d.]+) secs\)', ker)]
    # 用 net 侧时间轴统计首帧后 40s 内的 buffering（kernel 无墙钟，用 Enter 计数近似）
    nbuf = len(re.findall(r'Enter buffering', ker))
    # 首次 underrun（首帧后 dur=0.0 underrun=true 的相对秒）
    first_under = None
    for l in net:
        m = re.match(r'^(\d+)\s+\[cache\].*dur=0\.0s underrun=true', l)
        if m:
            rel = (int(m.group(1)) - t0) / 1000.0
            if rel >= 0: first_under = rel; break
    # 主 chunk 数 + 末尾 runway
    chunks = sum(1 for l in net if re.search(r'\[play\] origin chunk', l))
    runway = None
    for l in net:
        m = re.search(r'dur=([\d.]+)s', l)
        if m: runway = float(m.group(1))
    # 首帧耗时：prep 开始 -> 首帧就绪
    prep = None
    for l in net:
        m = re.match(r'^(\d+)\s', l)
        if m and 'URL 预备开始' in l: prep = int(m.group(1)); break
    ttf = (first - prep) / 1000.0 if (first and prep) else None
    # 292：确认 [rebuffer] 选项确实下发成功（写入=true），否则该臂无效
    rb = [l for l in net if '[rebuffer]' in l]
    rb_ok = sum(1 for l in rb if '写入=true' in l)
    rb_fail = sum(1 for l in rb if '写入=false' in l or '⚠' in l)
    # 292 · 方案 C 决定性有效性信号：恢复必须是「runway达标」。
    # 出现「超时兜底」= armedAtMs 被清零 / 预缓冲没攒够 ⇒ 该臂无效（退化成方案 B）。
    reason = '-'
    for l in rb:
        if '恢复默认' in l:
            reason = '达标' if 'runway达标' in l else '超时兜底'
            break
    # 293 · prefetch 有效性：启动 / 就绪 / 复用次数（off 臂应全 0）
    # 注意日志形态是 `[play] prefetch 启动 @…`，**不含** `[prefetch]` 方括号。
    pf_start = sum(1 for l in net if 'prefetch 启动 @' in l)
    pf_ready = sum(1 for l in net if 'prefetch 就绪 @' in l)
    pf_reuse = sum(1 for l in net if '复用预开块' in l)
    print(f"{arm:<8} {(f'{ttf:.1f}s' if ttf is not None else 'n/a'):>7} {nbuf:>5} "
          f"{(f'{sum(waits):.1f}s' if waits else '0s'):>9} "
          f"{(f'{max(waits):.1f}s' if waits else '0s'):>7} "
          f"{(f'+{first_under:.1f}s' if first_under is not None else 'none'):>13} "
          f"{chunks:>7} {(f'{runway:.1f}s' if runway is not None else 'n/a'):>9}  {fb}"
          f"  rb(ok/fail)={rb_ok}/{rb_fail} 恢复={reason}"
          f"  pf={pf_start}/{pf_ready}/复用{pf_reuse}")
print("\n注：FALLBACK>0 / rb_fail>0 / 恢复=超时兜底 的臂数据无效；bf次/总时长/最长 是核心判据，")
print("    '首帧' 是代价指标（起播预缓冲若让首帧变慢必须如实标注，不许只报卡顿改善）。")
PY
