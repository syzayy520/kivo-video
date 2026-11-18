#!/bin/bash
# 294 · 方案 D（chunkPrefetch）公网两臂验证 —— 闭合 279 §14 "公网未验 not_measured"
#
# 【目的】方案 D 在本地精确限速装置上已充分验证（279 §14：240s 长稳 pf 臂零 buffering、
#   bf 2→0、underrun none）。唯一缺口是"公网真实源站表头 1.5–8 s 随机 + 可能限并发"下
#   是否仍改善。本脚本用真实 Emby 源站跑 chunkPrefetch=off/on 两臂对照。
#
# 【为什么不能用 p292_startup_ab.sh】那是本地装置脚本（p274_local_origin.mjs + adb reverse），
#   刻意"禁公网波动"，无法验证"表头随机 + 限并发"这一公网特有变量。
#
# 【判据】（起播窗口 0-40 s，同 279 §14.3 口径）
#   - 首帧时间（net-diag "首帧就绪" 相对起点）
#   - buffering 次数 / 总时长 / 最长（mpv-kernel.log Enter/End buffering）
#   - 首次 underrun 相对时刻、末 runway（[cache] underrun/runway）
#   - [prefetch] 动作日志（预开 / 提升 / 作废）与 conn 数（红线：总连接 ≤2）
#   pf 臂应：首帧代价在噪声内、buffering 显著减少、underrun 推迟或消失。
#
# 【硬前置】设备必须解锁（deviceLocked=0）——锁屏下 adb input/intent/curl 全被拦。
#   用法：先请用户解锁，再 `bash tools/p294_chunkprefetch_public_ab.sh 150`
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
PKG=com.kivo.cinema
ACT="$PKG/.ui.compose.PlayerComposeActivity"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
# 与 D357/D358 同源同起点（bangumi《读书会》1.02GB，已验证稳定）
URL="https://emby.bangumi.ca/emby/Videos/232935/stream?static=true&MediaSourceId=c22b47bf46b9ec9f6f9f4736ae628922"
SIZE=1097796531
SEEK=396
SECS="${1:-150}"
OUTROOT="docs/evidence/p403-design/v364"
RUN_ID="$(date +%s)"
cd "C:/kivo video" || exit 1
mkdir -p "$OUTROOT"

cleanup() {
  $ADB shell "setprop debug.kivo.chunkPrefetch off" >/dev/null 2>&1
  $ADB shell "setprop debug.kivo.originKeepAlive 0" >/dev/null 2>&1
  $ADB shell am force-stop "$PKG" >/dev/null 2>&1
}
trap cleanup EXIT INT TERM

echo "===== 前置校验 ====="
LOCKED=$($ADB shell dumpsys trust 2>/dev/null | grep -o 'deviceLocked=[01]' | head -1)
AIR=$($ADB shell settings get global airplane_mode_on 2>/dev/null | tr -d '\r')
echo "  $LOCKED  airplane=$AIR  token_len=${#TOKEN}"
if [ "$LOCKED" != "deviceLocked=0" ]; then
  echo "  ❌ 设备仍锁屏 —— 请先请用户解锁，脚本拒绝在锁屏态起播（不静默降级）"
  exit 1
fi
if [ "$AIR" != "0" ]; then
  echo "  ❌ 飞行模式开着 —— 请先关闭，脚本拒绝起播"
  exit 1
fi
echo "  ✅ 前置通过"

run_arm() {
  local label="$1" pf="$2"
  local out="$OUTROOT/$label"
  mkdir -p "$out"
  $ADB shell am force-stop "$PKG" >/dev/null 2>&1
  sleep 2
  $ADB shell "setprop debug.kivo.originKeepAlive 0"
  $ADB shell "setprop debug.kivo.chunkPrefetch $pf"
  $ADB shell "run-as $PKG sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
  $ADB shell "am start -n $ACT --es kivo.playurl '$URL&run=$RUN_ID&arm=$label' \
    --es kivo.embyToken '$TOKEN' --el kivo.playSize $SIZE --ei kivo.seekSec $SEEK \
    --es kivo.label '$label' --ez kivo.gpuNext true" >/dev/null 2>&1
  echo "  [$label] chunkPrefetch=$pf  采样 ${SECS}s"
  sleep "$SECS"
  $ADB exec-out "run-as $PKG cat files/net-diag.log" > "$out/net.log" 2>/dev/null
  $ADB exec-out "run-as $PKG cat files/mpv-kernel.log" > "$out/kernel.log" 2>/dev/null
  $ADB exec-out "run-as $PKG cat files/mpv-kernel.log.diag" > "$out/diag.log" 2>/dev/null
  $ADB shell screencap -p //sdcard/p294.png >/dev/null 2>&1
  $ADB pull //sdcard/p294.png "$out/playing.png" >/dev/null 2>&1
  echo "    net=$(wc -l < "$out/net.log")行 kernel=$(wc -l < "$out/kernel.log")行"
}

echo "===== 跑两臂 @ ${SECS}s/臂 run=$RUN_ID ====="
run_arm ctl off
run_arm pf  on

echo "===== 判据汇总（起播窗口 0-40s）====="
python3 - "$OUTROOT" <<'PY'
import os, re, sys
root = sys.argv[1]
print(f"{'臂':<6} {'空窗s':>7} {'真实首帧':>9} {'EnterBuf':>9} {'bf总s':>7} {'最长s':>7} {'首次underrun':>13} {'预取动作':>9} {'conn.fail':>10}")
for arm in ('ctl', 'pf'):
    p = os.path.join(root, arm)
    net = open(os.path.join(p, 'net.log'), encoding='utf-8', errors='ignore').read().splitlines()
    ker = open(os.path.join(p, 'kernel.log'), encoding='utf-8', errors='ignore').read()
    def ts(l):
        m = re.match(r'^(\d+)', l)
        return int(m.group(1)) if m else None
    # 【口径修正 1】首帧基准必须是"URL 起播开始"，不能用 net[0]（"预备开始"）。
    #   预备完成→起播开始之间有 Activity 冷启动调度空窗（ctl 臂实测 95s），
    #   算进首帧会得到 98.5s 的假值，真实内核起播→首帧只有 3.3s。
    prep   = next((ts(l) for l in net if '[prep] URL 预备开始' in l), None)
    start  = next((ts(l) for l in net if '[prep] URL 起播开始' in l), None)
    first  = next((ts(l) for l in net if '首帧就绪' in l), None)
    gap_s  = (start - prep) / 1000.0 if prep and start else 0.0
    ff_s   = (first - start) / 1000.0 if start and first else None
    ff     = f'{ff_s:.1f}s' if ff_s is not None else 'NA'
    ent = len(re.findall(r'Enter buffering', ker))
    waits = [float(x) for x in re.findall(r'End buffering \(waited ([\d.]+) secs\)', ker)]
    wsum = sum(waits); wmax = max(waits) if waits else 0.0
    # 首帧后 40s 内首次 [cache] underrun=true
    runder = 'none'
    for l in net:
        t = ts(l)
        if t and 'underrun=true' in l and first and first <= t <= first + 40000:
            runder = f'+{(t-first)/1000.0:.1f}s'; break
    # 【口径修正 2】实际 prefetch 标记是 [play-seek] origin warmup /
    #   [tail] prefetch window / [play-tail] origin chunk，不是 [prefetch]。
    pfact = len([l for l in net if '[play-seek] origin warmup' in l
                 or '[tail] prefetch window' in l or '[play-tail] origin chunk' in l])
    cfail = len([l for l in net if 'conn fail' in l or 'ConnectionReset' in l])
    print(f"{arm:<6} {gap_s:>7.1f} {ff:>9} {ent:>9} {wsum:>7.1f} {wmax:>7.1f} {runder:>13} {pfact:>9} {cfail:>10}")
PY
echo "===== DONE (证据在 $OUTROOT/) ====="
