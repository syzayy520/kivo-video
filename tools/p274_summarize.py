#!/usr/bin/env python3
"""274 · 汇总装置上的全部实验臂（预览上限 A/B 的结果表）。

用法：python tools/p274_summarize.py
读 docs/evidence/p274-local-origin/*/ 下的 net.log / kernel.log / diag.log。
"""
import os, re, subprocess, sys, glob

ROOT = "C:/kivo video"
EV = os.path.join(ROOT, "docs/evidence/p274-local-origin")


def read(p):
    try:
        with open(p, encoding='utf-8', errors='ignore') as f:
            return f.read().split('\n')
    except OSError:
        return []


def gaps(d):
    k = os.path.join(d, "kernel.log")
    g = os.path.join(d, "diag.log")
    if not (os.path.exists(k) and os.path.exists(g)):
        return None, None
    try:
        out = subprocess.run(
            ["node", "tools/p273_gaps.mjs", k, g],
            cwd=ROOT, capture_output=True, text=True, timeout=60).stdout
    except Exception:
        return None, None
    fps = buf = None
    for line in out.split('\n'):
        if '实际帧率' in line:
            fps = line.split(':')[-1].strip().replace('fps', '').strip()
        if 'Enter buffering' in line:
            m = re.search(r'(\d+)', line)
            if m:
                buf = m.group(1)
    return fps, buf


rows = []
for d in sorted(glob.glob(os.path.join(EV, "*"))):
    if not os.path.isdir(d):
        continue
    name = os.path.basename(d.rstrip('/\\'))
    net = read(os.path.join(d, "net.log"))
    if not net:
        continue
    resp = cap = None
    for l in net:
        m = re.search(r'GET range=bytes=1905- bytes=(\d+) cap=(\d+) ms=(\d+)', l)
        if m and resp is None:
            cap, resp = int(m.group(1)), int(m.group(3))
        if resp is None:
            m2 = re.search(r'GET range=bytes=0- bytes=(\d+) cap=(\d+) ms=(\d+)', l)
            if m2:
                cap, resp = int(m2.group(1)), int(m2.group(3))
    thumb_bytes = thumb_n = 0
    for l in net:
        m = re.search(r'\[thumb\] origin chunk from=(\d+).*len=(\d+)', l)
        if m:
            thumb_n += 1
            thumb_bytes += int(m.group(2))
    total = 0
    for l in net:
        m = re.search(r'\[([a-z-]+)\] origin chunk from=\d+.*len=(\d+)', l)
        if m:
            total += int(m.group(2))
    yld = sum(1 for l in net if 'thumb] origin 让路' in l)
    play_yld = sum(1 for l in net if 'play] origin 让路' in l)
    fps, buf = gaps(d)
    rows.append((name, cap, resp, fps, buf, thumb_n, thumb_bytes, total, yld, play_yld))

print(f"{'臂':<24}{'上限':>9}{'预览响应':>12}{'帧率':>8}{'缓冲':>6}"
      f"{'thumb':>12}{'合计':>10}{'让路(缩/播)':>14}")
print("-" * 95)
for n, cap, resp, fps, buf, tn, tb, tot, y, py in rows:
    cap_s = f"{cap//1024}K" if cap else "-"
    resp_s = f"{resp}ms" if resp else "未完成"
    fps_s = fps or "-"
    buf_s = buf or "-"
    print(f"{n:<24}{cap_s:>9}{resp_s:>12}{fps_s:>8}{buf_s:>6}"
          f"{f'{tn}次/{tb//1048576}M':>12}{f'{tot//1048576}M':>10}{f'{y}/{py}':>14}")
