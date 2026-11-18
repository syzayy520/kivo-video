#!/bin/bash
# p358_armA.sh · 两臂复验 A 臂（keepalive=0，每块新建连接）20 分钟长跑采集
#
# 【与 B 臂严格对照】复刻 D358 B 臂（longB）的采集口径，唯一差异是开关：
#   A 臂 originKeepAlive=0（默认，每块新建） / B 臂 =1（复用）。
#   同一片源（bangumi《读书会》Videos/232935，1.02GB）、同一起点（seek 396s
#   = B 臂续播落点 159497100 字节）、同为硬解 + gpu-next、同为 20 分钟。
#
# 【判据】p358_keeplive_trend.py 三判据：chunk 速率 / dur / underrun，
#   外加内核 vo_drop 零累积（longA-health.ndjson）。
#   必须跑满 20 分钟覆盖 demuxer-cache 完整波动周期（D358 教训：中途 4min
#   单调下降是假衰减，第 16min 会跳回）。
#
# 【用法】先确保设备已解锁、在前台、stayon 常亮：
#   bash tools/p358_armA.sh
# 输出：docs/evidence/p403-design/v358/longA{,.txt,-netdiag.txt,-health.ndjson}
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
PKG=com.kivo.cinema
D="C:/kivo video/docs/evidence/p403-design/v358"
LABEL=longA
TOTAL_MIN=20

# ---- 片源（B 臂同源同落点）----
SRC_ID="c22b47bf46b9ec9f6f9f4736ae628922"
URL="https://emby.bangumi.ca/emby/Videos/232935/stream?static=true&MediaSourceId=${SRC_ID}&kivo.run=longA-arm"
SIZE=1097796531
SEEK_SEC=396
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt' 2>/dev/null)"

cd "C:/kivo video" || exit 1
mkdir -p "$D"

# ---- 测试属性退出必复位（覆盖 Ctrl-C / 中途失败）----
cleanup() { $ADB shell "setprop debug.kivo.originKeepAlive 0" >/dev/null 2>&1; }
trap cleanup EXIT INT TERM

# ---- 前置检查：设备必须解锁、非飞行模式 ----
LOCKED=$($ADB shell dumpsys trust 2>/dev/null | grep -c "deviceLocked=1" | tr -d '\r')
if [ "$LOCKED" != "0" ]; then
  echo "!! 设备处于安全锁屏，adb 注入会被拦——请先解锁再跑（不许静默降级）"
  exit 2
fi
AIR=$($ADB shell settings get global airplane_mode_on 2>/dev/null | tr -d '\r')
if [ "$AIR" = "1" ]; then
  echo "!! 飞行模式开着——请先关闭再跑"
  exit 2
fi

# ---- A 臂：keepalive=0 ----
$ADB shell "setprop debug.kivo.originKeepAlive 0"
$ADB shell svc power stayon true >/dev/null 2>&1
$ADB shell am force-stop "$PKG"
sleep 2
$ADB shell "run-as $PKG sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag; rm -f files/metrics/${LABEL}-health.ndjson'" 2>/dev/null
$ADB shell "am start -n $PKG/.ui.compose.PlayerComposeActivity --es kivo.playurl '$URL' --es kivo.embyToken '$TOKEN' --el kivo.playSize $SIZE --ei kivo.seekSec $SEEK_SEC --es kivo.label '$LABEL'" >/dev/null 2>&1
echo "T0=$(date '+%F %T') keepalive=0 seek=${SEEK_SEC}s url=$URL"

# ---- 起播健康检查：30s 内必须出 [cache] 行，否则早退（不产出半成品）----
sleep 30
CACHE=$($ADB shell run-as $PKG cat files/net-diag.log 2>/dev/null | grep -ac "\[cache\]" | tr -d '\r')
if [ "${CACHE:-0}" -eq 0 ]; then
  echo "!! 起播 30s 无 [cache] 行（内核未起）——早退，本臂作废"
  $ADB shell run-as $PKG cat files/net-diag.log 2>/dev/null > "$D/longA-abort-netdiag.txt"
  exit 1
fi
echo "  起播 OK：[cache] $CACHE 行；开始 ${TOTAL_MIN} 分钟采样"

# ---- 逐分钟采样（口径与 B 臂逐字一致）----
: > "$D/longA.txt"
for i in $(seq 1 "$TOTAL_MIN"); do
  sleep 60
  BAD=$($ADB shell run-as $PKG cat files/net-diag.log 2>/dev/null | grep -cE "origin open fail|eof unexpected|stall|degrada" | tr -d '\r')
  OK=$($ADB shell run-as $PKG cat files/net-diag.log 2>/dev/null | grep -cE "origin chunk" | tr -d '\r')
  C=$($ADB shell run-as $PKG cat files/net-diag.log 2>/dev/null | grep "\[cache\]" | tail -1 | grep -oE "fw=[0-9.]+M total=[0-9.]+M dur=[0-9.]+s underrun=[a-z]+ rate=[0-9.]+M/s" | tr -d '\r')
  echo "t=${i}min 异常=$BAD chunk=$OK $C" | tee -a "$D/longA.txt"
done

# ---- 收尾取证（采样结束后）----
$ADB shell run-as $PKG cat files/net-diag.log 2>/dev/null > "$D/longA-netdiag.txt"
$ADB shell run-as $PKG cat files/mpv-kernel.log 2>/dev/null > "$D/longA-kernel.log"
$ADB shell run-as $PKG cat "files/metrics/${LABEL}-health.ndjson" 2>/dev/null > "$D/longA-health.ndjson"
echo "===== A 臂完成 $(date '+%F %T') ====="
python tools/p358_keeplive_trend.py "$D/longA.txt" 2>&1
echo "LONG_A_DONE"
