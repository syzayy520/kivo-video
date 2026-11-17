#!/usr/bin/env python3
"""345 · soak 时间序列复算 + SurfaceFlinger 口径对照。

用法：
    python tools/p345_soak_report.py <label>-health.ndjson [sf-raw.txt]

输入 1：`files/metrics/<label>-health.ndjson`（由 MpvHealthSoak 产出，一行一个 5s 采样点）
输入 2：`adb shell dumpsys SurfaceFlinger --latency "<层名>"` 的原始输出，可多轮拼接

输出：mpv 侧趋势（mistimed / vo_drop / decoder_drop / avsync / vf_fps）
      + SF 侧帧间隔分位数与 >20ms 次数
      + 两者口径对照（**mistimed 不等于卡顿次数**，见 docs/design/playback/345 §5.4）

两个必须知道的坑（本脚本已处理，手工算容易错）：
  1. `--latency` 的未使用槽位是 `0 0 0`，另有 INT64_MAX 量级的哨兵值 ⇒ 必须过滤，
     否则 span 会算成几十亿秒。
  2. 环形缓冲只有 128 帧，多轮采样之间必然有 10s 级间隙 ⇒ **必须按轮分组**再算间隔，
     不能把合并后的时间戳直接排序求差分。
"""

import json
import sys

SENTINEL_NS = 2 ** 62


def load_soak(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            obj = json.loads(line)
            if "truncated" in obj:
                print(f"[warn] 序列被截断：{obj['truncated']}")
                continue
            h = obj.get("health") or {}
            rows.append((
                obj.get("t_ms", 0),
                h.get("mistimed_frame_count"),
                h.get("vo_drop_count"),
                h.get("decoder_drop_count"),
                h.get("avsync_s"),
                h.get("estimated_vf_fps"),
            ))
    return rows


def print_soak(rows):
    if not rows:
        print("soak: 没有采样点")
        return
    first, last = rows[0], rows[-1]
    span = (last[0] - first[0]) / 1000.0
    print(f"soak 采样点 {len(rows)}，覆盖 {first[0]/1000:.1f}s → {last[0]/1000:.1f}s（{span:.1f}s）")
    print(f"{'t_s':>7} {'mistimed':>9} {'vo_drop':>8} {'dec_drop':>9} {'avsync':>10} {'vf_fps':>7}")
    step = max(1, len(rows) // 10)
    picked = rows[::step]
    if picked[-1] != rows[-1]:
        picked.append(rows[-1])
    for r in picked:
        print(f"{r[0]/1000:7.1f} {str(r[1]):>9} {str(r[2]):>8} {str(r[3]):>9} "
              f"{str(r[4]):>10} {str(r[5]):>7}")


def load_sf(path):
    """按 '# round N' 分组；无分组标记时整份当一轮。"""
    rounds, cur = [], []
    with open(path, encoding="utf-8", errors="ignore") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            if s.startswith("# round"):
                if cur:
                    rounds.append(cur)
                cur = []
                continue
            parts = s.split()
            if len(parts) == 3:
                try:
                    actual = int(parts[1])
                except ValueError:
                    continue
                if 0 < actual < SENTINEL_NS:
                    cur.append(actual)
    if cur:
        rounds.append(cur)
    return rounds


def print_sf(path):
    rounds = load_sf(path)
    gaps, usable = [], 0
    for r in rounds:
        r = sorted(set(r))
        if len(r) < 10:
            continue
        usable += 1
        gaps += [(r[i + 1] - r[i]) / 1e6 for i in range(len(r) - 1)]
    if not gaps:
        print("sf: 没有可用轮次（每轮至少 10 帧）")
        return None
    gaps.sort()
    k = len(gaps)
    print(f"\nSF 可用轮 {usable}/{len(rounds)}，轮内间隔样本 {k}")
    for p in (0.50, 0.90, 0.99, 1.0):
        print(f"  gap p{int(p*100):>3} = {gaps[min(int(k * p), k - 1)]:.2f} ms")
    over = {thr: sum(1 for g in gaps if g > thr) for thr in (16.7, 20, 25, 33.4)}
    for thr, n in over.items():
        print(f"  >{thr}ms 次数 = {n}")
    print(f"  等效上屏 fps（按 p50）= {1000 / gaps[k // 2]:.2f}")
    return over


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    rows = load_soak(sys.argv[1])
    print_soak(rows)
    if len(sys.argv) >= 3:
        over = print_sf(sys.argv[2])
        if rows and over is not None and len(rows) > 1:
            m0, m1 = rows[0][1], rows[-1][1]
            if isinstance(m0, int) and isinstance(m1, int):
                print("\n口径对照：")
                print(f"  mistimed 增量 = +{m1 - m0}（{(m1 - m0) / ((rows[-1][0]-rows[0][0])/1000):.2f} 次/秒）")
                print(f"  SF >20ms 次数 = {over[20]}")
                print("  ⇒ mistimed 记的是「没落在理想 vsync」，不是可见卡顿；"
                      "判卡顿仍以 SF gap>20ms 为准。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
