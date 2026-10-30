#!/bin/sh
# 生成 P13-AF 的三条 60 s 片源。
#
# 为什么是 60 s：2000 ppm 在 8 s 里只累积 16 ms，门限 45 ms —— 算术上不可能触发。
# 60 s 累积 116 ms，预期触发约 15 次。
#
# 三臂只有一处不同：A=1080p30（基线）、B=1080p60（只看帧率）、C=4K30（只看分辨率）。
# 不做 2×2 四臂：4K60 的 60 s 编码要十几分钟，而 A/B/C 已经能把两个自变量各自分离。
#
# 片源不入库：`tests/real/` 在 .gitignore 里。要重建就重跑本脚本。
set -eu
FF="C:/kivo video/third_party/ffmpeg-sdk/bin/ffmpeg.exe"
OUT="C:/kivo video/tests/real"
mkdir -p "$OUT"

enc() {
  name=$1
  size=$2
  rate=$3
  echo "[enc] $name  ${size}@${rate}"
  "$FF" -hide_banner -loglevel error \
    -f lavfi -i "testsrc2=size=${size}:rate=${rate}:duration=60" \
    -f lavfi -i "sine=frequency=440:sample_rate=48000:duration=60" \
    -c:v libx264 -preset ultrafast -crf 32 -pix_fmt yuv420p -g 60 \
    -c:a aac -b:a 128k -shortest -y "$OUT/$name"
  ls -l "$OUT/$name" | awk '{print "     ", $5, "bytes"}'
}

enc p13af_1080p30_60s_av.mp4 1920x1080 30
enc p13af_1080p60_60s_av.mp4 1920x1080 60
enc p13af_4k30_60s_av.mp4    3840x2160 30
echo "[done]"
