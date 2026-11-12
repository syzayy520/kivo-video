#!/usr/bin/env python3
"""REX 播放页像素基线测量（2532x1170 @3x -> pt = px/3）。

用法:
  python tools/measure_rex_playback.py icons <img> [--y0 90 --y1 180]
      顶栏白色图标列聚类 -> 每个图标的 x 范围/中心/宽度 + 行范围
  python tools/measure_rex_playback.py bottom <img>
      底栏: 进度条 y/厚度/左右边界/已播分段 + 时间文字 bbox + 按钮行 bbox
  python tools/measure_rex_playback.py panel <img> [--y0 200 --y1 950]
      毛玻璃面板矩形(列/行亮度聚类) + 面板内文字行带(行高 pitch)
  python tools/measure_rex_playback.py text <img> --x0 A --x1 B --y0 C --y1 D
      指定窗口内非背景像素 bbox(量字高/字宽)

所有输出 px 与 pt 双列。背景为暗画面时阈值法有效; 亮画面需换帧。
"""
import sys
import numpy as np
from PIL import Image

P = 3.0  # px per pt


def load(path):
    return np.asarray(Image.open(path).convert("RGB")).astype(np.float32)


def clusters(mask_1d, gap):
    out = []
    i = 0
    n = len(mask_1d)
    while i < n:
        if mask_1d[i]:
            s = i
            while i < n and mask_1d[i]:
                i += 1
            out.append([s, i])
        else:
            i += 1
    merged = []
    for s, e in out:
        if merged and s - merged[-1][1] <= gap:
            merged[-1][1] = e
        else:
            merged.append([s, e])
    return merged


def fmt(s, e):
    return f"px[{s},{e}) w={e-s} | pt[{s/P:.1f},{e/P:.1f}) w={(e-s)/P:.1f} c={(s+e)/2/P:.1f}"


def icons(img, y0, y1):
    sub = img[y0:y1]
    white = (sub.min(axis=2) > 190).sum(axis=0)
    cols = clusters(white > 2, gap=14)
    print(f"# icons band y[{y0},{y1}) = pt[{y0/P:.1f},{y1/P:.1f})")
    for s, e in cols:
        if e - s < 8:
            continue
        band = (sub[:, s:e].min(axis=2) > 190).sum(axis=1)
        ys = clusters(band > 0, gap=4)
        ys = [y for y in ys if y[1] - y[0] > 6]
        yr = f"y[{y0+ys[0][0]},{y0+ys[-1][1]}) h={ys[-1][1]-ys[0][0]}px={(ys[-1][1]-ys[0][0])/P:.1f}pt" if ys else "y?"
        print(f"  x {fmt(s,e)}  {yr}")


def longest_run(mask):
    best = (0, 0, 0)
    s = None
    for i, v in enumerate(mask):
        if v and s is None:
            s = i
        elif not v and s is not None:
            if i - s > best[0]:
                best = (i - s, s, i)
            s = None
    if s is not None and len(mask) - s > best[0]:
        best = (len(mask) - s, s, len(mask))
    return best


def bottom(img):
    h, w, _ = img.shape
    lum = img.mean(axis=2)
    print("# track scan y[700,920): longest run of lum>45")
    track_y = None
    for y in range(700, 920):
        n, s, e = longest_run(lum[y] > 45)
        if n > w * 0.7:
            print(f"  y={y} ({y/P:.1f}pt) run x[{s},{e}) = pt[{s/P:.1f},{e/P:.1f})")
            track_y = y
    if track_y is None:
        return
    print(f"# column x=1200 around track y[{track_y-12},{track_y+12})")
    for y in range(track_y - 12, track_y + 12):
        r = img[y, 1200]
        r2 = img[y, 300]
        print(f"  y={y} ({y/P:.1f}pt) unplayed=({r[0]:.0f},{r[1]:.0f},{r[2]:.0f}) played_x300=({r2[0]:.0f},{r2[1]:.0f},{r2[2]:.0f})")
    print("# played/buffered split on track row")
    row = lum[track_y]
    seg = clusters(row > 45, gap=2)
    for s, e in seg:
        if e - s > 20:
            m = img[track_y, s:e].mean(axis=0)
            print(f"  x[{s},{e}) = pt[{s/P:.1f},{e/P:.1f}) mean=({m[0]:.0f},{m[1]:.0f},{m[2]:.0f})")
    print("# button row clusters y[930,1120)")
    sub = img[930:1120]
    white = (sub.min(axis=2) > 190).sum(axis=0)
    for s, e in clusters(white > 2, gap=14):
        if e - s < 8:
            continue
        band = (sub[:, s:e].min(axis=2) > 190).sum(axis=1)
        ys = clusters(band > 0, gap=4)
        ys = [y for y in ys if y[1] - y[0] > 6]
        yr = f"y[{930+ys[0][0]},{930+ys[-1][1]}) h={(ys[-1][1]-ys[0][0])/P:.1f}pt" if ys else "y?"
        print(f"  x {fmt(s,e)}  {yr}")


def panel(img, y0, y1):
    sub = img[y0:y1]
    lum = sub.mean(axis=2)
    colmean = lum.mean(axis=0)
    cols = clusters(colmean > 70, gap=30)
    cols = [c for c in cols if c[1] - c[0] > 150]
    print(f"# panel scan y[{y0},{y1})")
    for s, e in cols:
        rows = lum[:, s:e].mean(axis=1)
        ys = clusters(rows > 70, gap=20)
        ys = [y for y in ys if y[1] - y[0] > 100]
        if not ys:
            continue
        t, b = y0 + ys[0][0], y0 + ys[-1][1]
        print(f"  rect x {fmt(s,e)}  y px[{t},{b}) h={b-t}px={(b-t)/P:.1f}pt  y_pt[{t/P:.1f},{b/P:.1f})")
        # 面板内文字行带
        inner = lum[ys[0][0]:ys[-1][1], s+20:e-20]
        std = inner.std(axis=1)
        bands = clusters(std > 18, gap=6)
        bands = [bd for bd in bands if bd[1] - bd[0] > 8]
        print("  text bands (rel y):")
        prev = None
        for a, bb in bands:
            pitch = f" pitch={a-prev}px={(a-prev)/P:.1f}pt" if prev is not None else ""
            print(f"    y[{t+a},{t+bb}) h={bb-a}px={(bb-a)/P:.1f}pt{pitch}")
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
          f"w={xs.max()-xs.min()+1}px={(xs.max()-xs.min()+1)/P:.1f}pt "
          f"h={ys.max()-ys.min()+1}px={(ys.max()-ys.min()+1)/P:.1f}pt")


if __name__ == "__main__":
    cmd = sys.argv[1]
    img = load(sys.argv[2])

    def arg(name, default):
        return int(sys.argv[sys.argv.index(name) + 1]) if name in sys.argv else default

    if cmd == "icons":
        icons(img, arg("--y0", 90), arg("--y1", 180))
    elif cmd == "bottom":
        bottom(img)
    elif cmd == "panel":
        panel(img, arg("--y0", 200), arg("--y1", 950))
    elif cmd == "text":
        text(img, arg("--x0", 0), arg("--x1", img.shape[1]), arg("--y0", 0), arg("--y1", img.shape[0]))