#!/usr/bin/env python3
"""P14-F 汇总：时钟异常的出现率与形状。

三件事，缺一件这张卡就白做：
  ① **分母**：这一轮跑了多少分钟（不是"几轮"，轮长度可变）；
  ② **分子**：`monotonic_rejects` / `jumps_clamped` / `trace.event_count` 三个数是否自洽；
  ③ **形状**：每个事件是"候选自己跳"还是"守卫把它压住"——用窗口里 pivot 那行判。

⚠️ `trace` 只留 3 格。一轮里超过 3 次事件时 `event_count` 会小于真实次数，
   所以分子以 `rejects + clamped` 为准，`event_count` 只用来判断"有没有窗口可看"。

用法：
    python tools/summarize_p14f_jumps.py docs/evidence/p14f/*-sync.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

TRACE_CAPACITY = 3


def classify(event: dict[str, Any]) -> str:
    """判读一个事件的形状。

    两类，判据是**候选与接受在 pivot 那一行是否相等**：
      `guard_clamped`   —— 候选往前跳了一大步，守卫只放行了一部分（差额被吞）
      `guard_rejected`  —— 候选**倒退**，守卫拒绝了它（接受值 = 上一次位置）
    两者都可能"看起来像同一件事"，但责任方相反：
    前者是设备跳、我们拦住；后者是设备报错、我们纠正。
    """
    points = event.get("points") or []
    pivot = int(event.get("pivot", -1))
    if pivot <= 0 or pivot >= len(points):
        return "unknown"
    current = points[pivot]
    previous = points[pivot - 1]
    candidate_step = int(current["candidate_ns"]) - int(previous["candidate_ns"])
    accepted_step = int(current["accepted_ns"]) - int(previous["accepted_ns"])
    suppressed = int(current["candidate_ns"]) - int(current["accepted_ns"])

    if candidate_step < 0:
        return f"guard_rejected(cand{candidate_step / 1e6:+.2f}ms acc{accepted_step / 1e6:+.2f}ms)"
    if suppressed > 0:
        return f"guard_clamped(swallowed {suppressed / 1e6:.2f}ms)"
    return f"passthrough(cand{candidate_step / 1e6:+.2f}ms)"


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2

    total_s = 0.0
    total_events = 0
    rows = []
    for raw in argv[1:]:
        path = Path(raw)
        data = json.loads(path.read_text(encoding="utf-8"))
        clock = data["audio"]["clock"]
        duration_s = int(data["source"]["duration_us"]) / 1e6
        rejects = int(clock["monotonic_rejects"])
        clamped = int(clock["jumps_clamped"])
        trace = clock.get("trace") or {}
        captured = int(trace.get("event_count", 0))
        events = trace.get("events") or []

        total_s += duration_s
        total_events += rejects + clamped

        shapes = [classify(event) for event in events]
        rows.append((path.name, duration_s, rejects, clamped, captured, shapes,
                     int(data["frames_dropped"]),
                     int(data["pace"]["lateness_ns"]["p99"]) / 1e6))

    print(f"{'轮':<16}{'时长s':>7}{'reject':>7}{'clamp':>7}{'窗口':>6}{'丢帧':>5}{'p99ms':>7}  形状")
    for name, duration_s, rejects, clamped, captured, shapes, dropped, p99 in rows:
        shape_text = " / ".join(shapes) if shapes else "—"
        print(f"{name:<16}{duration_s:>7.0f}{rejects:>7}{clamped:>7}{captured:>6}"
              f"{dropped:>5}{p99:>7.2f}  {shape_text}")

    minutes = total_s / 60.0
    print(f"\n合计 {minutes:.1f} 分钟 / {len(rows)} 轮，异常 {total_events} 次"
          f"（rejects + clamped）")
    if total_events:
        print(f"出现率 ≈ 每 {minutes / total_events:.1f} 分钟 1 次"
              f"（n = {total_events}，样本很小，只能给量级）")
    else:
        print("本轮样本里没有异常 —— 这本身是结论，但 30 分钟无异常不能说'不发生'")

    # H1 自洽性：窗口数不该超过真实事件数，也不该超过容量
    bad = [row for row in rows if row[4] > min(row[2] + row[3], TRACE_CAPACITY)]
    if bad:
        print(f"FAIL H1：{len(bad)} 轮的窗口数超过真实事件数或容量上限")
        return 1
    truncated = [row for row in rows if row[2] + row[3] > TRACE_CAPACITY]
    if truncated:
        print(f"注意：{len(truncated)} 轮的事件数超过窗口容量 {TRACE_CAPACITY}，"
              "形状只有前三个")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
