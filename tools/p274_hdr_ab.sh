#!/bin/bash
# 274-HDR · HDR10 → SDR（L2）A/B 取证
#
# 【为什么用它】G1 已判：当前 render API（legacy RA）**没有 compute shader**
#   ⇒ HDR 峰值计算被禁用 ⇒ 真实 HDR 输出（G2）不成立（见 274-hdr-rendering.md §10.0）。
#   ⇒ 本脚本只验 **R2 = L2（HDR→SDR tone-map）**：不改架构，只看
#   "同一段 HDR10 画面，在不同 mpv 目标参数下，输出到 SDR 屏上是什么样子"。
#
# 【素材】build/avtmp/hdr10_1080p.mp4
#   hevc Main10 / 1920x1080 / yuv420p10le / bt2020nc / smpte2084(PQ) / bt2020 / 6.0s
#
# 【装置】tools/p274_local_origin.mjs（本地文件 ⇒ 速率确定，不受源站波动污染）
#
# 【用法】bash tools/p274_hdr_ab.sh
set -u
export MSYS_NO_PATHCONV=1
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
cd "C:/kivo video" || exit 1

SRC="build/avtmp/hdr10_1080p_x4.mp4"   # 6s 原片循环 4 次 ⇒ 24s（6s 太短，冻结截图会撞 EOF）
SIZE=$(wc -c < "$SRC" | tr -d ' ')
PORT=8123
OUT="docs/evidence/p274-hdr/l2-ab"
mkdir -p "$OUT"

# 装置：本地文件、限速 5 MB/s（6 秒片源，够快又不至于瞬间取完）
#
# 【为什么按端口杀进程，而不是 taskkill //IM node.exe】
#   · Git Bash 会把 `/F` 改写成 `F:/` ⇒ taskkill 静默失败 ⇒ 旧装置还占着端口，
#     新装置 EADDRINUSE 退出 ⇒ **测到的还是旧片源**（本轮真踩过：装置宣称总长
#     1935985234 = 上一轮的 Shelter，而我们要的是 349077 的 HDR10 片）。
#   · `taskkill /IM node.exe` 还会杀掉 WorkBuddy 自己的 node 进程 ⇒ 不能用。
# ⇒ 改成：先按端口找 PID 再杀。
for pid in $(netstat -ano | grep ":$PORT " | grep LISTENING | awk '{print $5}' | sort -u); do
  powershell -NoProfile -Command "Stop-Process -Id $pid -Force" >/dev/null 2>&1
done
sleep 1
node tools/p274_local_origin.mjs --port $PORT --mbps 5 \
  --file "$SRC" --size "$SIZE" --log "$OUT/origin.log" > "$OUT/origin.stderr" 2>&1 &
sleep 2
# 【自检】装置必须宣称我们这个片源的总长，否则说明端口被别人占着
DECL=$(curl -s -D - -o /dev/null -r 0-9 "http://127.0.0.1:$PORT/v" 2>/dev/null | grep -i content-range | grep -oE "/[0-9]+" | tr -d '/')
if [ "$DECL" != "$SIZE" ]; then
  echo "❌ 装置总长 $DECL ≠ 期望 $SIZE（端口被占？）"; exit 1
fi
echo "装置就绪：总长 $DECL 字节"
$ADB reverse tcp:$PORT tcp:$PORT >/dev/null

run_arm() {
  local label="$1"; local opt="$2"
  $ADB shell am force-stop com.kivo.cinema
  sleep 2
  $ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'"
  if [ -n "$opt" ]; then
    $ADB shell "am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
      --es kivo.playurl 'http://127.0.0.1:$PORT/v' --el kivo.playSize $SIZE \
      --es kivo.mpvOpt '$opt' --ei kivo.seekMs 3000 --ez kivo.freeze true \
      --es kivo.shot '$label.png' --es kivo.label '$label'" >/dev/null 2>&1
  else
    $ADB shell "am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
      --es kivo.playurl 'http://127.0.0.1:$PORT/v' --el kivo.playSize $SIZE \
      --ei kivo.seekMs 3000 --ez kivo.freeze true \
      --es kivo.shot '$label.png' --es kivo.label '$label'" >/dev/null 2>&1
  fi
  sleep 14
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$OUT/$label.kernel.log" 2>/dev/null
  # 【为什么截设备而不是让内核截图】
  #   mpv 的 `screenshot-to-file ... window` 在 render API（vo=libmpv）下**不支持**
  #   ⇒ 日志恒为 "Taking screenshot failed."（本轮三臂全踩）。
  #   设备截屏走的是 SurfaceFlinger 合成结果 ⇒ **就是用户眼睛看到的那一帧**，
  #   端到端更可信（连合成器与色彩空间一起算进去）。
  $ADB exec-out screencap -p > "$OUT/$label.png" 2>/dev/null
  echo "===== $label（opt=${opt:-无}）====="
  grep -E "Video  --vid|Audio  --aid|Subs  --sid" "$OUT/$label.kernel.log" | head -3
  grep -E "Disabling HDR peak|Set property: (target-|tone-mapping)" "$OUT/$label.kernel.log" | head -8
  if [ -s "$OUT/$label.png" ]; then
    echo "  截图 $(wc -c < "$OUT/$label.png") 字节"
  grep -E "Taking screenshot failed|screenshot-to-file" "$OUT/$label.kernel.log" | tail -2 | sed "s/^/    /"
  else
    echo "  ⚠ 没拿到截图"
  fi
}

# 臂 A：现状（不改任何 mpv 参数）
run_arm "a-baseline" ""
# 臂 B：L2 候选（SDR 目标 + bt.2390 色调映射 + 峰值按 SDR 100 nit 截）
run_arm "b-l2" "target-trc=bt.1886,target-prim=bt.709,target-peak=100,tone-mapping=bt.2390,target-colorspace-hint=yes"
# 臂 C：可视化判据（tone-mapping-visualize 会画出色调映射曲线）
run_arm "c-visualize" "tone-mapping-visualize=yes"

echo "===== 亮度统计（Rec.709 系数 / sRGB 编码值）====="
FF=$(cygpath -w "$(command -v ffmpeg)" 2>/dev/null || command -v ffmpeg)
for f in a-baseline b-l2 c-visualize; do
  [ -s "$OUT/$f.png" ] || continue
  python tools/analyze_frame_luma.py --ffmpeg "$FF" --out "$OUT/$f.luma.json" "$f=$OUT/$f.png" 2>&1 | tail -3
done
