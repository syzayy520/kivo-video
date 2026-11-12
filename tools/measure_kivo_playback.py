#!/usr/bin/env python3
"""Kivo 播放页真机截图测量（dp = px / density，与 REX 的 pt 同纲）。

用法:
  python tools/measure_kivo_playback.py icons <img> [--density 2.75] [--y0 F] [--y1 F]
  python tools/measure_kivo_playback.py bottom <img> [--density 2.75]
  python tools/measure_kivo_playback.py panel <img> [--y0 F] [--y1 F]
  python tools/measure_kivo_playback.py text <img> --x0 A --x1 B --y0 C --y1 D

y0/y1 缺省按 REX 基线帧(1170 高)的比例换算到当前帧高；x 探针按帧宽比例取。
聚类/最长_run 复用 measure_rex_playback，保证两边算法逐位一致。
"""
import sys
import numpy as np
from PIL import Image
import measure_rex_playback as rex

D = 2.75  # px per dp（K40 density 440）


def load(path):
    return np.asarray(Image.open(path).convert("RGB")).astype(np.float32)


def fmt(s, e):
    return f"px[{s},{e}) w={e-s} | dp[{s/D:.1f},{e/D:.1f}) w={(e-s)/D:.1f} c={(s+e)/2/D:.1f}"


def icons(img, y0, y1):
    sub = img[y0:y1]
    white = (sub.min(axis=2) > 190).sum(axis=0)
    print(f"# icons band y[{y0},{y1}) = dp[{y0/D:.1f},{y1/D:.1f})")
    for s, e in rex.clusters(white > 2, gap=int(14 * D / 3)):
        if e - s < 8:
            continue
        band = (sub[:, s:e].min(axis=2) > 190).sum(axis=1)
        ys = [y for y in rex.clusters(band > 0, gap=4) if y[1] - y[0] > 6]
        yr = f"y[{y0+ys[0][0]},{y0+ys[-1][1]}) h={(ys[-1][1]-ys[0][0])/D:.1f}dp" if ys else "y?"
        print(f"  x {fmt(s,e)}  {yr}")


def bottom(img):
    h, w, _ = img.shape
    lum = img.mean(axis=2)
    ya, yb = int(h * 700 / 1170), int(h * 920 / 1170)
    print(f"# track scan y[{ya},{yb}): longest run of lum>45")
    track_y = None
    for y in range(ya, yb):
        n, s, e = rex.longest_run(lum[y] > 45)
        if n > w * 0.7:
            print(f"  y={y} ({y/D:.1f}dp) run x[{s},{e}) = dp[{s/D:.1f},{e/D:.1f})")
            track_y = y
    if track_y is None:
        print("  (no track run — 画面全黑时剩余段不可见，属预期)")
    else:
        row = lum[track_y]
        print("# played/buffered split on track row")
        for s, e in rex.clusters(row > 45, gap=2):
            if e - s > 20:
                m = img[track_y, s:e].mean(axis=0)
                print(f"  x[{s},{e}) = dp[{s/D:.1f},{e/D:.1f}) mean=({m[0]:.0f},{m[1]:.0f},{m[2]:.0f})")
    print(f"# button row clusters y[{int(h*930/1170)},{int(h*1120/1170)})")
    sub = img[int(h * 930 / 1170):int(h * 1120 / 1170)]
    white = (sub.min(axis=2) > 190).sum(axis=0)
    for s, e in rex.clusters(white > 2, gap=int(14 * D / 3)):
        if e - s < 8:
            continue
        band = (sub[:, s:e].min(axis=2) > 190).sum(axis=1)
        ys = [y for y in rex.clusters(band > 0, gap=4) if y[1] - y[0] > 6]
        y0 = int(h * 930 / 1170)
        yr = f"y[{y0+ys[0][0]},{y0+ys[-1][1]}) h={(ys[-1][1]-ys[0][0])/D:.1f}dp" if ys else "y?"
        print(f"  x {fmt(s,e)}  {yr}")


def panel(img, y0, y1):
    sub = img[y0:y1]
    lum = sub.mean(axis=2)
    cols = [c for c in rex.clusters(lum.mean(axis=0) > 70, gap=30) if c[1] - c[0] > 150]
    print(f"# panel scan y[{y0},{y1})")
    for s, e in cols:
        rows = lum[:, s:e].mean(axis=1)
        ys = [y for y in rex.clusters(rows > 70, gap=20) if y[1] - y[0] > 100]
        if not ys:
            continue
        t, b = y0 + ys[0][0], y0 + ys[-1][1]
        print(f"  rect x {fmt(s,e)}  y dp[{t/D:.1f},{b/D:.1f}) h={(b-t)/D:.1f}dp")
        inner = lum[ys[0][0]:ys[-1][1], s + 20:e - 20]
        bands = [bd for bd in rex.clusters(inner.std(axis=1) > 18, gap=6) if bd[1] - bd[0] > 8]
        prev = None
        for a, bb in bands:
            pitch = f" pitch={(a-prev)/D:.1f}dp" if prev is not None else ""
            print(f"    y[{t+a},{t+bb}) h={(bb-a)/D:.1f}dp{pitch}")
            prev = a


def text(img, x0, x1, y0, y1):
    sub = img[y0:y1, x0:x1]
    lum = sub.mean(axis=2)
    bg = np.median(lum)
    mask = np.abs(lum - bg) > 40
    ys = np.where(mask.sum(axis=1) > 2)[0]
    xs = np.where(mask.sum(axis=0) > 2)[0]
    if len(ys) == 0:
        print("  empty")
        return
    print(f"  bbox x[{x0+xs.min()},{x0+xs.max()}] y[{y0+ys.min()},{y0+ys.max()}] "
          f"w={(xs.max()-xs.min()+1)/D:.1f}dp h={(ys.max()-ys.min()+1)/D:.1f}dp")


if __name__ == "__main__":
    cmd = sys.argv[1]
    img = load(sys.argv[2])
    h, w, _ = img.shape

    def arg(name, default):
        return float(sys.argv[sys.argv.index(name) + 1]) if name in sys.argv else default

    D = arg("--density", 2.75)
    rex.P = D
    if cmd == "icons":
        icons(img, int(arg("--y0", 90 * h / 1170)), int(arg("--y1", 180 * h / 1170)))
    elif cmd == "bottom":
        bottom(img)
    elif cmd == "panel":
        panel(img, int(arg("--y0", 200 * h / 1170)), int(arg("--y1", 950 * h / 1170)))
    elif cmd == "text":
        text(img, int(arg("--x0", 0)), int(arg("--x1", w)), int(arg("--y0", 0)), int(arg("--y1", h)))