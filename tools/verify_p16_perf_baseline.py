#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B24：从仓库里的证据算性能基线，超预算即红。

单一职责：守"数字有没有变差"。**不跑真机、不改证据、不替代 `verify_p12_all.py`**
（那个守结构：schema、字段、豁免）。

为什么需要它：`verify_p12_all.py` 只检查证据格式对不对，不检查数字。
如果有人把 `publish()` 开销翻倍、丢帧率从 0.09% 涨到 0.4%，结构门禁全绿，
性能已经退化了。本脚本补这一层。

判据：
    Q1  `/19+` 的冷启动 > 300 ms 份数 == 已登记豁免数
    Q2  全部 seek `total_us` > 150 ms 份数 == 已登记豁免数
    Q3  全局丢帧率 `Σdropped / Σrendered` < 0.5%
    Q4  每条豁免都必须真实存在（防止"豁免了但证据删了"）

退出码：0 = 基线守住；1 = 有超预算未登记；2 = 校验器自身失效。
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EVIDENCE = ROOT / "docs" / "evidence"

COLD_BUDGET_US = 300_000   # G8a
SEEK_BUDGET_US = 150_000   # G8b
DROP_RATIO_LIMIT = 0.005   # G6

# `/13` 时期的三份超预算轮次：4K 或 130 s 长片源，属历史证据。
# **每条都要带理由**，不许用通配符。
KNOWN_OVER_BUDGET: dict[str, str] = {
    "p13r-4k60-r3.json": "P13-R 4K60 轮，冷启动 548 ms；旧 schema /13，4K 片源",
    "p13s-4k60-r1.json": "P13-S 4K60 轮，冷启动 379 ms；旧 schema /13，4K 片源",
    "p13k-130s-r1.json": "P13-K 130 s 长片源，冷启动 311 ms；旧 schema /13",
}


def is_current(schema: str) -> bool:
    """`/19` 及以后算"当前口径"。"""
    try:
        version = int(str(schema).rsplit("/", 1)[-1])
    except (ValueError, IndexError):
        return False
    return version >= 19


def load_rows() -> list[dict]:
    rows: list[dict] = []
    for path in sorted(glob.glob(str(EVIDENCE / "**" / "*.json"), recursive=True)):
        try:
            data = json.loads(Path(path).read_text(encoding="utf-8"))
        except Exception:
            continue
        schema = str(data.get("schema", ""))
        if not schema.startswith("kivo.p12c.decode/"):
            continue
        name = os.path.basename(path)
        cold = (data.get("cold_start") or {}).get("first_frame_us")
        seek = (data.get("seek") or {}).get("total_us")
        counters = data.get("counters") or {}
        rows.append({
            "name": name,
            "schema": schema,
            "current": is_current(schema),
            "cold_us": cold if isinstance(cold, (int, float)) else None,
            "seek_us": seek if isinstance(seek, (int, float)) else None,
            "rendered": counters.get("frames_rendered") or 0,
            "dropped": counters.get("frames_dropped_late") or 0,
        })
    return rows


def evaluate(rows: list[dict], known: dict[str, str]) -> tuple[list[str], dict]:
    """纯函数：返回 (问题列表, 统计)。rows 可注入变异。"""
    problems: list[str] = []
    seen: set[str] = set()

    # 所有超预算都必须登记，**不分 schema** —— 历史轮次也要写清理由，
    # 否则"豁免表里有、证据里没有"和"证据里有、豁免表里没有"会互相掩盖。
    over_cold = [r for r in rows if r["cold_us"] is not None and r["cold_us"] > COLD_BUDGET_US]
    over_seek = [r for r in rows if r["seek_us"] is not None and r["seek_us"] > SEEK_BUDGET_US]

    for row in over_cold:
        seen.add(row["name"])
        if row["name"] not in known:
            problems.append(
                f"Q1 {row['name']}: 冷启动 {row['cold_us'] / 1000:.1f} ms > "
                f"{COLD_BUDGET_US / 1000:.0f} ms，且未登记"
            )
    for row in over_seek:
        seen.add(row["name"])
        if row["name"] not in known:
            problems.append(
                f"Q2 {row['name']}: seek {row['seek_us'] / 1000:.1f} ms > "
                f"{SEEK_BUDGET_US / 1000:.0f} ms，且未登记"
            )

    for name in sorted(known):
        if name not in seen:
            problems.append(f"Q4 豁免 {name} 不再对应任何超预算证据（过期豁免）")

    rendered = sum(r["rendered"] for r in rows)
    dropped = sum(r["dropped"] for r in rows)
    ratio = (dropped / rendered) if rendered else 0.0
    if ratio >= DROP_RATIO_LIMIT:
        problems.append(
            f"Q3 丢帧率 {ratio * 100:.4f}% >= {DROP_RATIO_LIMIT * 100:.1f}% "
            f"({dropped} / {rendered})"
        )

    stats = {
        "rows": len(rows),
        "current": sum(1 for r in rows if r["current"]),
        "over_cold": len(over_cold),
        "over_cold_current": sum(1 for r in over_cold if r["current"]),
        "over_seek": len(over_seek),
        "rendered": rendered,
        "dropped": dropped,
        "drop_ratio": ratio,
    }
    return problems, stats


def self_test() -> int:
    rows = load_rows()
    baseline, _ = evaluate(rows, KNOWN_OVER_BUDGET)
    print("=== verify_p16_perf_baseline 变异自检 ===")
    print(f"  baseline problems: {len(baseline)}  rows: {len(rows)}")
    ok = True

    # M1：注入一份当前口径、冷启动 500 ms 的证据
    injected = rows + [{"name": "__mutation__.json", "schema": "kivo.p12c.decode/19",
                        "current": True, "cold_us": 500_000, "seek_us": None,
                        "rendered": 1, "dropped": 0}]
    problems, _ = evaluate(injected, KNOWN_OVER_BUDGET)
    caught = any("__mutation__.json" in p and "Q1" in p for p in problems)
    print(f"  M1 注入超预算冷启动 : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M2：注入一份 seek 200 ms 的证据
    injected = rows + [{"name": "__mutation_seek__.json", "schema": "kivo.p12c.decode/19",
                        "current": True, "cold_us": None, "seek_us": 200_000,
                        "rendered": 1, "dropped": 0}]
    problems, _ = evaluate(injected, KNOWN_OVER_BUDGET)
    caught = any("__mutation_seek__.json" in p and "Q2" in p for p in problems)
    print(f"  M2 注入超预算 seek  : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M3：把豁免删掉，原来的 3 份超预算必须重新变红
    problems, _ = evaluate(rows, {})
    caught = any("Q1" in p for p in problems)
    print(f"  M3 删掉豁免         : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M4：注入一个不存在的豁免名
    problems, _ = evaluate(rows, {**KNOWN_OVER_BUDGET, "__ghost__.json": "不存在"})
    caught = any("__ghost__.json" in p and "Q4" in p for p in problems)
    print(f"  M4 过期豁免         : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    return 0 if ok else 2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="跑变异自检后退出")
    args = parser.parse_args()

    rows = load_rows()
    if not rows:
        print("FATAL: 没找到 decode 证据", file=sys.stderr)
        return 2
    if args.self_test:
        return self_test()

    problems, stats = evaluate(rows, KNOWN_OVER_BUDGET)
    print("=== 性能基线 ===")
    print(f"  decode 证据: {stats['rows']} 份（其中当前口径 {stats['current']} 份）")
    print(f"  冷启动超 {COLD_BUDGET_US / 1000:.0f} ms: {stats['over_cold']} 份"
          f"（其中当前口径 {stats['over_cold_current']} 份）")
    print(f"  seek 超 {SEEK_BUDGET_US / 1000:.0f} ms            : {stats['over_seek']} 份")
    print(f"  丢帧率: {stats['drop_ratio'] * 100:.4f}% "
          f"({stats['dropped']} / {stats['rendered']})  上限 {DROP_RATIO_LIMIT * 100:.1f}%")
    print(f"  已登记豁免: {len(KNOWN_OVER_BUDGET)} 条（逐条打印）")
    for name, reason in sorted(KNOWN_OVER_BUDGET.items()):
        print(f"    {name}  <- {reason}")

    if problems:
        print(f"--- FAIL: {len(problems)} 项 ---")
        for line in problems:
            print(f"  {line}")
        return 1
    print("PASS: Q1–Q4 全部守住（超预算均已登记，丢帧率在限内）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
