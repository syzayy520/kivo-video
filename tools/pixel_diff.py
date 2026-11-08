#!/usr/bin/env python3
"""像素级 diff（硬约束工具）：对"输出物"与"参照物"逐像素对比，输出可量化证据。

用法:
  python tools/pixel_diff.py <ref> <out_img> <diff_out.png>
        [--ref-ppd 2.0628] [--out-ppd 2.75] [--norm-dp 393] [--px-per-dp 3]

输出（stdout，客观生成，不得修改或选择性呈现）:
  REF_SIZE / OUT_SIZE / NORM_SIZE / COMMON_REGION
  不一致像素总数 (Δ>0) 及其占全图比例
  最大单像素通道偏差 (0-255)
  补充：Δ>8、Δ>24 的像素数与比例（仅作可读性参考，不替代严格口径）
  差异图路径

口径说明：Δ = 两图同位置像素在三通道上的最大绝对差。
严格口径取 Δ>0 —— 只要有一丝差异即计为不一致（JPEG 压缩、缩放重采样都会造成差异，
这是客观事实，不做豁免）。因此本工具**从不**输出 PASS；判定权在审计方。
"""
import sys
import numpy as np
from PIL import Image

args = sys.argv[1:]
ref_p, out_p, diff_p = args[0], args[1], args[2]


def opt(name, default):
    return float(args[args.index(name) + 1]) if name in args else default


ref_ppd = opt("--ref-ppd", 2.0628)
out_ppd = opt("--out-ppd", 2.75)
norm_dp = opt("--norm-dp", 393)
px_per_dp = opt("--px-per-dp", 3)

ref = Image.open(ref_p).convert("RGB")
out = Image.open(out_p).convert("RGB")
print(f"REF_SIZE = {ref.size} ({ref.width/ref_ppd:.1f}x{ref.height/ref_ppd:.1f}dp @{ref_ppd}px/dp)")
print(f"OUT_SIZE = {out.size} ({out.width/out_ppd:.1f}x{out.height/out_ppd:.1f}dp @{out_ppd}px/dp)")

# 归一到同一 dp 尺度（等比，不强行统一高度）
tw = int(round(norm_dp * px_per_dp))
def norm(im, ppd):
    k = tw / (im.width / ppd * px_per_dp) * 1.0
    return im.resize((tw, int(round(im.height * tw / im.width))), Image.LANCZOS)

r = norm(ref, ref_ppd)
o = norm(out, out_ppd)
W = min(r.width, o.width)
H = min(r.height, o.height)
print(f"NORM_SIZE = ref {r.size} / out {o.size}")
print(f"COMMON_REGION = {W}x{H}px ({W/px_per_dp:.1f}x{H/px_per_dp:.1f}dp)")

a = np.asarray(r.crop((0, 0, W, H))).astype(np.int16)
b = np.asarray(o.crop((0, 0, W, H))).astype(np.int16)
d = np.abs(a - b)
dmax = d.max(axis=2)
total = W * H

strict = int((dmax > 0).sum())
p8 = int((dmax > 8).sum())
p24 = int((dmax > 24).sum())
mx = int(dmax.max())
mean = float(dmax.mean())

print()
print(f"不一致像素总数 (Δ>0) = {strict} / {total} = {strict/total*100:.2f}%")
print(f"最大单像素通道偏差 = {mx} / 255")
print(f"平均通道偏差 = {mean:.2f}")
print(f"补充 Δ>8  = {p8} ({p8/total*100:.2f}%)")
print(f"补充 Δ>24 = {p24} ({p24/total*100:.2f}%)")

# 高亮差异图：红=差异强度，绿=一致
heat = np.zeros((H, W, 3), np.uint8)
heat[..., 0] = np.clip(dmax * 2, 0, 255)
heat[..., 1] = np.clip(160 - dmax * 2, 0, 160)
Image.fromarray(heat).save(diff_p)
print(f"DIFF_IMAGE = {diff_p}")

# 分区拆解（补充证据，不替代严格口径）：用于定位差异来源
print()
print("分区拆解（补充，仅供定位；不改变上面的严格口径结论）:")
bands = [(0, 510, "幕布(艺术图)"), (510, 600, "meta+主操作行"),
         (600, 660, "简介+徽章"), (660, 850, "各节")]
for y0, y1, name in bands:
    r0, r1 = int(y0 * px_per_dp), min(int(y1 * px_per_dp), H)
    if r0 >= H:
        continue
    seg = dmax[r0:r1]
    n = seg.size
    print(f"  {name:14s} y[{y0},{y1})dp  Δ>0 {int((seg>0).sum())/n*100:6.2f}%  "
          f"Δ>24 {int((seg>24).sum())/n*100:6.2f}%  平均 {seg.mean():5.1f}  最大 {int(seg.max())}")

print()
print("STATUS = GAP" if strict > 0 else "STATUS = NO_DIFF")
