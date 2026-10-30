#!/bin/sh
# 生成 P14-D 的 30 分钟长片（720p30，30 分钟 = 54000 帧）。
#
# 为什么 720p：360p 的解码压力太小，测不出"长期满载"；1080p 的编码与推送都更慢。
# 为什么 30 分钟：真正的 2 小时要占设备两小时且中途不能被打断；
# 30 分钟已经足够回答"每秒一次的修正会不会累积成漂移"。
set -eu
FF="C:/kivo video/third_party/ffmpeg-sdk/bin/ffmpeg.exe"
OUT="C:/kivo video/tests/real/p14d_720p30_1800s_av.mp4"
"$FF" -hide_banner -loglevel error -stats \
  -f lavfi -i "testsrc2=size=1280x720:rate=30:duration=1800" \
  -f lavfi -i "sine=frequency=440:sample_rate=48000:duration=1800" \
  -c:v libx264 -preset ultrafast -crf 30 -pix_fmt yuv420p -g 60 \
  -c:a aac -b:a 128k -shortest -y "$OUT"
ls -l "$OUT" | awk '{print $5, "bytes"}'
