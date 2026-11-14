#!/bin/bash
# 281 · 长稳探针：每 5 秒采一次 mpv 的欠载/缓冲/丢帧 + 上屏帧率
# 用法: bash tools/p281_soak_probe.sh <标签> <秒数>
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
LABEL="${1:-soak}"
SECS="${2:-180}"
OUT="docs/evidence/p281/live/$LABEL"
mkdir -p "$OUT"
L='SurfaceView[com.kivo.cinema/com.kivo.cinema.ui.compose.PlayerComposeActivity](BLAST)'
ID=$($ADB shell "dumpsys SurfaceFlinger --list 2>/dev/null" | grep -F "$L" | head -1 | sed 's/.*#//' | tr -d '\r')
echo "layer=$ID secs=$SECS"
: > "$OUT/latency.txt"
N=$((SECS/5))
for i in $(seq 1 $N); do
  $ADB shell "dumpsys SurfaceFlinger --latency '$L#$ID'" 2>/dev/null >> "$OUT/latency.txt"
  echo "" >> "$OUT/latency.txt"
  T=$(date +%s)
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" 2>/dev/null > "$OUT/kernel-$T.log"
  echo "t=$((i*5))s kernel_bytes=$(wc -c < "$OUT/kernel-$T.log")"
  sleep 5
done
$ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" 2>/dev/null > "$OUT/net.log"
echo "--- 汇总 ---"
node tools/p274_sf_latency.mjs "$OUT/latency.txt"
# 【为什么要用最后一份快照】每次快照都是**整份日志**的拷贝 ⇒
# `cat kernel-*.log | grep -c` 会把同一行重复计数 N 次
# （281 第一版就踩了：音频欠载被数成 36 次，实际只有 1 次）。
# 只看最后一份（它包含从起播到现在的全部内容）才是真值。
echo "--- 欠载/缓冲/丢帧（取最后一份快照，避免重复计数）---"
LAST=$(ls -t "$OUT"/kernel-*.log 2>/dev/null | head -1)
echo "snapshot=$LAST"
grep -ac "Audio device underrun" "$LAST" 2>/dev/null | sed 's/^/audio_underrun=/'
grep -ac "Enter buffering" "$LAST" 2>/dev/null | sed 's/^/enter_buffering=/'
grep -ac "Render queue underrun" "$LAST" 2>/dev/null | sed 's/^/render_q_underrun=/'
grep -acE "Video frame dropped|frame drop" "$LAST" 2>/dev/null | sed 's/^/video_drop=/'
grep -acE "osd_render \(slow" "$LAST" 2>/dev/null | sed 's/^/osd_slow=/'
