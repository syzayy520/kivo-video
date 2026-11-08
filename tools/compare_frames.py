#!/usr/bin/env python3
"""像素级比对：把参照帧与真机截图归一到同一 dp 尺度，逐元素核对。

用法:
  python tools/compare_frames.py <ref.png> <ours.png> <out_prefix>
        [--ref-ppd 3.0] [--ours-ppd 2.75] [--thresh 9]

产出:
  <out_prefix>_side.png    同尺度并排（左=参照，右=实现）
  <out_prefix>_diff.png    逐像素差异热图
  stdout: 两张图的元素带表（dp）+ 行剖面（每 8dp 的均值色与亮度差）
"""
import sys
import numpy as np
from PIL import Image

DP_W = 390.0          # 参照系宽度（dp）
PX = 3                # 归一化后每 dp 像素数
H_DP = 844.0          # 参照系高度（dp）


def norm(path, ppd):
    """缩放到 3px/dp，**保持 dp 几何**（不强行统一高度——设备高度与参照不同）。"""
    im = Image.open(path).convert("RGB")
    k = PX / ppd
    return im.resize((int(round(im.width * k)), int(round(im.height * k))), Image.LANCZOS)


def bands(img, thresh=9.0, min_rows=2):
    a = np.asarray(img).astype(np.float32)
    h = a.shape[0]
    std = a.reshape(h, -1).std(axis=1)
    mask = std > thresh
    out, y = [], 0
    while y < h:
        if mask[y]:
            s = y
            while y < h and mask[y]:
                y += 1
            if y - s >= min_rows:
                out.append((s, y))
        else:
            y += 1
    return out


def band_table(name, img, thresh):
    print(f"\n=== {name} 元素带（dp）===")
    print(f"{'top':>8} {'bot':>8} {'h':>7}")
    bs = bands(img, thresh)
    for s, e in bs:
        print(f"{s/PX:8.1f} {e/PX:8.1f} {(e-s)/PX:7.1f}")
    return bs


def main():
    ref_p, ours_p, out = sys.argv[1], sys.argv[2], sys.argv[3]
    ref_ppd = float(sys.argv[sys.argv.index("--ref-ppd") + 1]) if "--ref-ppd" in sys.argv else 3.0
    our_ppd = float(sys.argv[sys.argv.index("--ours-ppd") + 1]) if "--ours-ppd" in sys.argv else 2.75
    thresh = float(sys.argv[sys.argv.index("--thresh") + 1]) if "--thresh" in sys.argv else 9.0

    ref = norm(ref_p, ref_ppd)
    ours = norm(ours_p, our_ppd)

    W = min(ref.width, ours.width)
    H = min(ref.height, ours.height)
    print(f"# 归一后 参照 {ref.size} / 实现 {ours.size} / 公共比对区 {W}x{H} (= {W/PX:.1f}x{H/PX:.1f}dp)")
    ref_c = ref.crop((0, 0, W, H))
    ours_c = ours.crop((0, 0, W, H))
    side = Image.new("RGB", (W * 2 + 24, H), (255, 255, 255))
    side.paste(ref_c, (0, 0))
    side.paste(ours_c, (W + 24, 0))
    side.save(f"{out}_side.png")

    a = np.asarray(ref_c).astype(np.int16)
    b = np.asarray(ours_c).astype(np.int16)
    d = np.abs(a - b).max(axis=2).astype(np.uint8)
    heat = np.zeros((H, W, 3), np.uint8)
    heat[..., 0] = np.clip(d * 3, 0, 255)
    heat[..., 1] = np.clip(255 - d * 2, 0, 255)
    Image.fromarray(heat).save(f"{out}_diff.png")

    band_table("参照", ref_c, thresh)
    band_table("实现", ours_c, thresh)

    print("\n=== 行剖面（每 8dp）===")
    print(f"{'y_dp':>7} {'ref_rgb':>16} {'ours_rgb':>16} {'dLum':>7}")
    for y in range(0, H, 8 * PX):
        ra = np.asarray(ref_c)[y:y + PX].reshape(-1, 3).mean(axis=0)
        rb = np.asarray(ours_c)[y:y + PX].reshape(-1, 3).mean(axis=0)
        la = 0.2126 * ra[0] + 0.7152 * ra[1] + 0.0722 * ra[2]
        lb = 0.2126 * rb[0] + 0.7152 * rb[1] + 0.0722 * rb[2]
        print(f"{y/PX:7.1f} ({ra[0]:5.0f},{ra[1]:4.0f},{ra[2]:4.0f}) "
              f"({rb[0]:5.0f},{rb[1]:4.0f},{rb[2]:4.0f}) {lb-la:+7.1f}")


if __name__ == "__main__":
    main()
