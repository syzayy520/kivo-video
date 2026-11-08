#!/usr/bin/env python3
"""逐行/逐列像素测量：把截图与参照帧换算到 dp 后对齐比较。

用法:
  python tools/measure_frames.py bands <img> <px_per_dp> [--x0 A --x1 B]
      -> 输出内容带（连续的高方差行），单位 px 与 dp
  python tools/measure_frames.py rows <img> <px_per_dp> [--x0 A --x1 B] [--step N]
      -> 每 N 行输出 mean RGB + stddev，用于找色带/亮度跳变
  python tools/measure_frames.py col <img> <px_per_dp> --y A [--y2 B]
      -> 指定行的列剖面，找左右边界（文字起始 x）
"""
import sys
import numpy as np
from PIL import Image


def load(path):
    return np.asarray(Image.open(path).convert("RGB")).astype(np.float32)


def bands(img, px_per_dp, x0=None, x1=None, thresh=6.0, min_rows=2):
    h, w, _ = img.shape
    x0 = 0 if x0 is None else x0
    x1 = w if x1 is None else x1
    sub = img[:, x0:x1, :]
    # 每行的局部方差：文字行/边缘行方差高，纯色行方差低
    std = sub.reshape(h, -1).std(axis=1)
    mask = std > thresh
    out = []
    y = 0
    while y < h:
        if mask[y]:
            s = y
            while y < h and mask[y]:
                y += 1
            if y - s >= min_rows:
                out.append((s, y))
        else:
            y += 1
    print(f"# {h}x{w} @ {px_per_dp}px/dp  x[{x0},{x1}) thresh={thresh}")
    print(f"{'top_px':>7} {'bot_px':>7} {'h_px':>5} | {'top_dp':>7} {'bot_dp':>7} {'h_dp':>6}")
    for s, e in out:
        print(f"{s:7d} {e:7d} {e-s:5d} | {s/px_per_dp:7.1f} {e/px_per_dp:7.1f} {(e-s)/px_per_dp:6.1f}")


def rows(img, px_per_dp, x0=None, x1=None, step=10):
    h, w, _ = img.shape
    x0 = 0 if x0 is None else x0
    x1 = w if x1 is None else x1
    sub = img[:, x0:x1, :]
    print(f"# {h}x{w} @ {px_per_dp}px/dp  x[{x0},{x1}) step={step}")
    print(f"{'y_px':>6} {'y_dp':>7} {'mean_rgb':>22} {'std':>6} {'lum':>6}")
    for y in range(0, h, step):
        r = sub[y]
        m = r.mean(axis=0)
        sd = r.reshape(-1).std()
        lum = 0.2126 * m[0] + 0.7152 * m[1] + 0.0722 * m[2]
        print(f"{y:6d} {y/px_per_dp:7.1f} ({m[0]:5.0f},{m[1]:5.0f},{m[2]:5.0f}) {sd:6.1f} {lum:6.1f}")


def col(img, px_per_dp, y, y2=None):
    h, w, _ = img.shape
    y2 = y + 1 if y2 is None else y2
    band = img[y:y2, :, :]
    ref = np.median(band.reshape(-1, 3), axis=0)
    # 与中位色差异超过阈值的列视为"有内容"
    d = np.abs(band - ref).max(axis=2).max(axis=0)
    idx = np.where(d > 18)[0]
    print(f"# y[{y},{y2}) @ {px_per_dp}px/dp  median=({ref[0]:.0f},{ref[1]:.0f},{ref[2]:.0f})")
    if len(idx) == 0:
        print("no content")
        return
    # 连续段
    segs = []
    s = idx[0]
    p = idx[0]
    for i in idx[1:]:
        if i - p > 8:
            segs.append((s, p))
            s = i
        p = i
    segs.append((s, p))
    for a, b in segs:
        print(f"  x[{a:5d},{b:5d}]  dp[{a/px_per_dp:7.1f},{b/px_per_dp:7.1f}]  w={(b-a)/px_per_dp:6.1f}")


if __name__ == "__main__":
    cmd = sys.argv[1]
    path = sys.argv[2]
    ppd = float(sys.argv[3])
    kw = {}
    it = iter(sys.argv[4:])
    for k in it:
        if k == "--x0":
            kw["x0"] = int(next(it))
        elif k == "--x1":
            kw["x1"] = int(next(it))
        elif k == "--step":
            kw["step"] = int(next(it))
        elif k == "--y":
            kw["y"] = int(next(it))
        elif k == "--y2":
            kw["y2"] = int(next(it))
        elif k == "--thresh":
            kw["thresh"] = float(next(it))
    img = load(path)
    {"bands": bands, "rows": rows, "col": col}[cmd](img, ppd, **kw)
