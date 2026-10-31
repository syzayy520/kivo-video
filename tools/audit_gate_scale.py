#!/usr/bin/env python3
"""门限量纲普查（P14-V）。

回答一个问题：**`drift_limit_ns` 恒为 45 ms，但它在不同长度的会话里意味着什么？**

门守的是累积漂移，而累积漂移 = 速率误差 × 时长。于是：

    「45 ms 门限」≡ 「这次会话允许的速率误差 = 45 ms / 时长」

同一句话在 8 s 上宽松，在 30 分钟上等于晶振精度。这个换算从来没有人做过 ——
`limit_ns` 与 `elapsed_span_ns` 一直并排印在同一份 JSON 里，却从不互相比。

用法：

    python tools/audit_gate_scale.py            # 摘要
    python tools/audit_gate_scale.py --detail   # 逐份列出

退出码：0 正常；1 发现内部不一致（等价速率与独立复算不符）。
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import statistics
import sys
from typing import Any

#: 门守的是「累积偏差不超过人耳阈值」，所以判据线本身不是错的 ——
#: 错的是没人算出它在长会话上收紧到什么程度。这两句要分开看。
AUDIBLE_LIMIT_NS = 45_000_000

#: 消费级晶振（含 SoC 音频域与 CLOCK_MONOTONIC 所用那两颗）的频差量级。
#: 与 `pace_wall_offset.hpp` 的 `kWallReferencePpm` 同一个假设，同一条来路。
CRYSTAL_PPM = 50.0

#: 以 50 ppm 走多久会攒到 45 ms。**与窗口无关的常量** ——
#: 45000 µs / (50 µs/s) = 900 s = 15 分钟。
CRYSTAL_REACH_S = AUDIBLE_LIMIT_NS / (CRYSTAL_PPM * 1e-6 * 1e9)


def gate_limit(gate: dict[str, Any], pace: dict[str, Any]) -> int:
    """门限。`drift_gate.limit_ns` 从 `/20` 起有；更早的版本没有门这一说。"""
    return int(gate.get("limit_ns") or pace.get("drift_limit_ns") or 0)


def collect(root: str) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for path in sorted(glob.glob(os.path.join(root, "**", "*-sync.json"), recursive=True)):
        try:
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
        except (OSError, json.JSONDecodeError):
            continue
        schema = str(data.get("schema", ""))
        if not schema.startswith("kivo.p12c.sync/"):
            continue
        pace = data.get("pace") or {}
        gate = data.get("drift_gate") or {}
        span = int(pace.get("elapsed_span_ns") or 0)
        limit = gate_limit(gate, pace)
        if limit <= 0 or span <= 0:
            continue
        # 报告自己算的那一格（`/33` 起有），没有就按定义式补。
        reported = gate.get("limit_ppm_equivalent")
        derived = limit * 1e6 / span
        rows.append(
            {
                "path": os.path.relpath(path, root),
                "version": int(schema.rsplit("/", 1)[-1]),
                "span_s": span / 1e9,
                "limit_ns": limit,
                "ppm": derived,
                "ppm_reported": None if reported is None else float(reported),
                "enabled": gate.get("enabled"),
                "reach_s": gate.get("limit_reach_s"),
                "reach_measured": gate.get("limit_reach_s_measured"),
            }
        )
    return rows


def buckets(rows: list[dict[str, Any]]) -> dict[float, list[float]]:
    grouped: dict[float, list[float]] = {}
    for row in rows:
        grouped.setdefault(round(row["span_s"], 1), []).append(row["ppm"])
    return grouped


def main() -> int:
    parser = argparse.ArgumentParser(description="门限量纲普查（P14-V）")
    parser.add_argument("--root", default="docs/evidence")
    parser.add_argument("--detail", action="store_true", help="逐份列出")
    args = parser.parse_args()

    rows = collect(args.root)
    if not rows:
        print("没有可算的证据（需要 drift_gate.limit_ns 与 pace.elapsed_span_ns）")
        return 0

    # ── 反向判据：报告自己算的那一格必须与独立复算一致 ──
    #
    # 这是本脚本存在的另一半理由。只印表不校验，表本身可能是错的；
    # 而错了的表比没有表更糟 —— 它看起来像结论。
    mismatched = [
        row
        for row in rows
        if row["ppm_reported"] is not None
        and abs(row["ppm_reported"] - row["ppm"]) > abs(row["ppm"]) * 1e-6 + 1e-9
    ]

    print(f"可算份数：{len(rows)} 份（有 limit_ns 与 elapsed_span_ns）")
    print(f"45 ms 在 50 ppm 的晶振上要 {CRYSTAL_REACH_S:.0f} s 才攒到 —— 与窗口无关的常量")
    print()

    grouped = buckets(rows)
    print(f"{'窗口(s)':>9} {'份数':>5} {'等价速率(ppm)':>14} {'超出晶振区':>10}  读作")
    for span in sorted(grouped):
        values = grouped[span]
        median = statistics.median(values)
        in_crystal = "是" if (AUDIBLE_LIMIT_NS / (CRYSTAL_PPM * 1e-6 * 1e9)) <= span else "否"
        if median > 2000:
            reading = "很宽松（门几乎管不到速率）"
        elif median > 200:
            reading = "收紧"
        elif median > 60:
            reading = "接近晶振"
        else:
            reading = "晶振精度区（正常钟也会撞）"
        print(f"{span:9.1f} {len(values):5d} {median:14.0f} {in_crystal:>10}  {reading}")

    wide = [row for row in rows if row["span_s"] >= CRYSTAL_REACH_S]
    print()
    print(
        f"窗口 >= {CRYSTAL_REACH_S:.0f} s 的轮次：{len(wide)} 份 —— "
        f"这些轮次上，一个 50 ppm 的正常时钟**足够撞到 45 ms 门**"
    )
    if wide:
        enabled = sum(1 for row in wide if row["enabled"] is True)
        print(f"  其中门开着的：{enabled} 份（门关着的轮次不会动作，不能算作'门判断沉默'）")

    if args.detail:
        print()
        print(f"{'窗口(s)':>9} {'ppm':>9} {'v':>4} {'门':>4}  path")
        for row in sorted(rows, key=lambda item: item["span_s"]):
            gate_state = {True: "开", False: "关", None: "?"}[row["enabled"]]
            print(
                f"{row['span_s']:9.1f} {row['ppm']:9.0f} /{row['version']:<3} "
                f"{gate_state:>4}  {row['path']}"
            )

    print()
    if mismatched:
        print(f"FAIL：{len(mismatched)} 份的 limit_ppm_equivalent 与独立复算不符")
        for row in mismatched[:10]:
            print(
                f"  {row['path']}: 报告 {row['ppm_reported']:.6f}，"
                f"复算 {row['ppm']:.6f}（差 {row['ppm_reported'] - row['ppm']:.6f}）"
            )
        return 1

    reported = [row for row in rows if row["ppm_reported"] is not None]
    print(
        f"反向判据：{len(reported)} 份带 limit_ppm_equivalent，全部与独立复算一致"
        f"（其余 {len(rows) - len(reported)} 份是 /33 之前的，按定义式补算）"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
