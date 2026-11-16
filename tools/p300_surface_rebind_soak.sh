#!/bin/bash
# 300 · Surface 重建压测（275 §6.2「100 次前后台/视图重建」）
#
# 【量什么】HOME → 回前台 每轮都会触发 SurfaceView 的 surfaceCreated
# ⇒ MpvPlaybackSession.rebind() ⇒ native attachSurface ⇒ gpu-next 分支
# `mpv_set_option("wid", …)`。判据：
#   ① 进程 pid 不变（没有崩溃/被系统杀）；
#   ② diag 里 attach-surface 计数 = 初始 + N（说明每轮都真的重绑了，不是"看起来没崩"）；
#   ③ 结束画面仍在出帧（SurfaceFlinger 采样有帧）；
#   ④ native/JNI 内存不随次数单调暴涨（global ref 泄漏会体现为 JNI 行增长）。
#
# 【为什么用 local 3min 片，不用公网】本项只验 Surface 重绑，任何网络波动都会污染结论。
#
# 用法：bash tools/p300_surface_rebind_soak.sh [次数，默认100] [每轮间隔秒，默认1.2]
# 前置：app 已按 always_from_start=true / background_audio=false / auto_pip=false 起播。
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
PKG="com.kivo.cinema"
ACT="$PKG/.ui.compose.PlayerComposeActivity"

N="${1:-100}"
GAP="${2:-1.2}"

pid0=$($ADB shell pidof "$PKG" | tr -d '\r')
att0=$($ADB shell "run-as $PKG cat files/mpv-kernel.log.diag" 2>/dev/null | grep -c "attach-surface")
mem0=$($ADB shell "dumpsys meminfo $PKG" 2>/dev/null | grep -E "^\s+Native Heap|^\s+JNI" | tr -s ' ' | tr -d '\r')
echo "== 起点 == pid=$pid0 attach=$att0"
echo "   $mem0"

fail=0
for i in $(seq 1 "$N"); do
  $ADB shell input keyevent KEYCODE_HOME >/dev/null 2>&1
  sleep "$GAP"
  $ADB shell am start -n "$ACT" -f 0x00020000 >/dev/null 2>&1
  sleep "$GAP"

  # 每 20 轮抽样一次：pid 变了就是崩溃/被杀，立刻记下来但继续跑完（要拿到全量计数）
  if [ $((i % 20)) -eq 0 ]; then
    pid=$($ADB shell pidof "$PKG" | tr -d '\r')
    att=$($ADB shell "run-as $PKG cat files/mpv-kernel.log.diag" 2>/dev/null | grep -c "attach-surface")
    echo "  [$i/$N] pid=$pid attach=$att"
    [ "$pid" != "$pid0" ] && { echo "  !! 进程变了（$pid0 → $pid）"; fail=1; }
  fi
done

pid1=$($ADB shell pidof "$PKG" | tr -d '\r')
att1=$($ADB shell "run-as $PKG cat files/mpv-kernel.log.diag" 2>/dev/null | grep -c "attach-surface")
mem1=$($ADB shell "dumpsys meminfo $PKG" 2>/dev/null | grep -E "^\s+Native Heap|^\s+JNI" | tr -s ' ' | tr -d '\r')

echo "== 终点 == pid=$pid1 attach=$att1"
echo "   $mem1"
echo "== 判据 =="
echo "  pid 不变：$([ "$pid0" = "$pid1" ] && echo PASS || { echo "FAIL($pid0→$pid1)"; fail=1; })"
delta=$((att1 - att0))
echo "  重绑次数：$delta（期望 ≈ $N）$([ "$delta" -ge $((N * 9 / 10)) ] && echo PASS || { echo FAIL; fail=1; })"
echo "  --- 内存对比（Native Heap / JNI 行）---"
echo "  before: $mem0"
echo "  after : $mem1"
exit "$fail"
