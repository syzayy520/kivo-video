#!/usr/bin/env python3
"""给截图叠加 dp 标尺（每 20dp 一条横线 + 每 100dp 标注），用于直接读取元素位置。

用法: python tools/ruler.py <img> <px_per_dp> <out.png> [--step 20]
"""
import sys
from PIL import Image, ImageDraw, ImageFont

img_p, ppd, out = sys.argv[1], float(sys.argv[2]), sys.argv[3]
step = int(sys.argv[sys.argv.index("--step") + 1]) if "--step" in sys.argv else 20

im = Image.open(img_p).convert("RGB")
d = ImageDraw.Draw(im)
try:
    font = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", max(14, int(ppd * 7)))
except Exception:
    font = ImageFont.load_default()

H = im.height
y_dp = 0
while y_dp * ppd < H:
    y = int(y_dp * ppd)
    major = y_dp % 100 == 0
    d.line([(0, y), (im.width, y)], fill=(255, 0, 255) if major else (0, 255, 255), width=2 if major else 1)
    if y_dp % 100 == 0:
        d.text((6, y + 2), f"{y_dp}", fill=(255, 0, 255), font=font)
    y_dp += step

x_dp = 0
while x_dp * ppd < im.width:
    x = int(x_dp * ppd)
    if x_dp % 50 == 0:
        d.line([(x, 0), (x, im.height)], fill=(255, 255, 0), width=1)
        d.text((x + 2, 4), f"{x_dp}", fill=(255, 255, 0), font=font)
    x_dp += 50

im.save(out)
print(f"{out}  {im.size}  @{ppd}px/dp")
