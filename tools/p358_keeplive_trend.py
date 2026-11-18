#!/usr/bin/env python3
"""358 · 复用连接（keepAlive=1）长跑的趋势判定。

## 为什么单独一个脚本
274 记录过：**同一条连接用久了正文吞吐会塌**（226→150→100→50→48→47→42→67 KB/s，
表头始终 300–1300ms）。要判定"当前源站上这个衰减是否还在"，必须看**跨时间的趋势**，
而不是某一刻的瞬时值 —— 后者会得出相反结论（旧结论就是这么被误读的）。

## 判据（三项，任一成立即说明吞吐在衰减）
1. **chunk 速率下降**：后 1/3 时段的 chunk/分钟 < 前 1/3 的 60%
2. **前向缓存 dur 下降**：后段中位 `dur` < 前段中位的 60%
3. **出现 underrun**：`underrun=true` 的次数 ≥ 1（且不是在起播 30s 内）

用法：
    python tools/p358_keeplive_trend.py docs/evidence/p403-design/v358/longB.txt
"""
import re
import sys
import statistics


def parse(path):
    rows = []
    for line in open(path, encoding="utf-8"):
        m = re.match(r"t=(\d+)min\s+异常=(\d+)\s+chunk=(\d+).*?dur=([\d.]+)s.*?underrun=(\w+)", line)
        if not m:
            continue
        rows.append({
            "min": int(m.group(1)),
            "bad": int(m.group(2)),
            "chunk": int(m.group(3)),
            "dur": float(m.group(4)),
            "underrun": m.group(5) == "true",
        })
    return rows


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    rows = parse(sys.argv[1])
    if len(rows) < 6:
        print(f"样本不足（{len(rows)} 点），至少需要 6 点")
        return 1

    # chunk 的每分钟增量（第一点是累计含起播，跳过）
    rates, durations = [], []
    for a, b in zip(rows, rows[1:]):
        dm = b["min"] - a["min"]
        if dm <= 0:
            continue
        rates.append((b["min"], (b["chunk"] - a["chunk"]) / dm))
        durations.append((b["min"], b["dur"]))

    third = max(1, len(rates) // 3)
    head_rate = statistics.median(r for _, r in rates[:third])
    tail_rate = statistics.median(r for _, r in rates[-third:])
    head_dur = statistics.median(d for _, d in durations[:third])
    tail_dur = statistics.median(d for _, d in durations[-third:])

    print(f"样本 {len(rows)} 点，覆盖 {rows[0]['min']}–{rows[-1]['min']} 分钟")
    print(f"异常累计：{rows[-1]['bad']}")
    print()
    print(f"{'分钟':>4}  {'chunk/分':>9}  {'dur(s)':>7}  underrun")
    for (m, r), (_, d) in zip(rates, durations):
        flag = "  ←" if any(x["min"] == m and x["underrun"] for x in rows) else ""
        print(f"{m:>4}  {r:>9.1f}  {d:>7.1f}{flag}")
    print()
    print(f"chunk 速率：前 1/3 中位 {head_rate:.1f}/分 → 后 1/3 中位 {tail_rate:.1f}/分"
          f"（{tail_rate / head_rate * 100:.0f}%）")
    print(f"前向缓存 dur：前 1/3 中位 {head_dur:.1f}s → 后 1/3 中位 {tail_dur:.1f}s"
          f"（{tail_dur / head_dur * 100:.0f}%）")
    print()

    verdict = []
    if tail_rate < head_rate * 0.6:
        verdict.append("① chunk 速率下降到 60% 以下")
    if tail_dur < head_dur * 0.6:
        verdict.append("② 前向缓存 dur 下降到 60% 以下")
    late_underrun = [x for x in rows if x["underrun"] and x["min"] > 1]
    if late_underrun:
        verdict.append(f"③ 后段出现 underrun {len(late_underrun)} 次")

    if verdict:
        print("判定：**吞吐在衰减**")
        for v in verdict:
            print("  -", v)
    else:
        print("判定：**未观察到吞吐衰减**（三项判据都不成立）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
