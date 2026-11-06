#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B1：从 P13 系列真机证据重算「频差事件 / 漂移门有没有响」的账目。

单一职责：只回答「哪些轮真的出现频差、门在其中哪些轮沉默、账目自不自洽」。
**不判断控制律好坏、不改证据、不 import 项目代码、不替代 verify_p14a_metrics.py。**

为什么需要它：183 号设计稿里 A1 的数字全部转述自源码注释与任务卡，
换速率伺服之前必须有一份能被第三方重算的基线，否则 A/B 无从谈起。

真实字段路径（与文档叫法不一致，见 184 号文档）：
    timeline.playback_rate / pace.clock_health.drift_rebases
    pace.clock_health.drift_corrected_total_ns / drift_gate.enabled
    clock_injection.offset_ns / armed_at_position_ns / rate_ppm

判据：
    R1 字段不齐 → 单列 not_measured，不判红、不冒充"门沉默"（P13-E 口径）
    R2 rebases>0 ⇔ corrected_total>0（两者都出版时才判）
    R3 drift_gate.enabled=false 的对照轮 rebases 必须为 0 且单列
    R4 offset_ns>0（给了注入参数）⇒ armed_at_position_ns >= 0
    R5 |rate-1| >= 0.005 为事件轮，分注入/自然两组；自然轮里 rebases==0 的逐份列出

退出码：0 = 账目自洽；1 = 有不一致；2 = 校验器自身失效。
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
EVENT_THRESHOLD = 0.005


def load_rows() -> list[dict]:
    rows: list[dict] = []
    for path in sorted(glob.glob(str(EVIDENCE / "**" / "*.json"), recursive=True)):
        if "p13" not in path.lower():
            continue
        try:
            data = json.loads(Path(path).read_text(encoding="utf-8"))
        except Exception:
            continue
        timeline = data.get("timeline")
        if not isinstance(timeline, dict) or "playback_rate" not in timeline:
            continue
        health = data.get("pace", {}).get("clock_health", {})
        gate = data.get("drift_gate", {})
        injection = data.get("clock_injection", {})
        rows.append({
            "path": os.path.relpath(path, ROOT).replace("\\", "/"),
            "rate": timeline.get("playback_rate"),
            "rebases": health.get("drift_rebases"),
            "corrected": health.get("drift_corrected_total_ns"),
            "gate": gate.get("enabled") if isinstance(gate, dict) else None,
            "offset": injection.get("offset_ns") if isinstance(injection, dict) else None,
            "armed": injection.get("armed_at_position_ns") if isinstance(injection, dict) else None,
            "ppm": injection.get("rate_ppm") if isinstance(injection, dict) else None,
        })
    return rows


def is_injected(row: dict) -> bool:
    return (row.get("offset") or 0) > 0 or (row.get("ppm") or 0) != 0


def evaluate(rows: list[dict]) -> tuple[list[str], dict]:
    """纯函数：返回 (问题列表, 账目报告)。rows 可注入变异。"""
    problems: list[str] = []
    not_measured: list[str] = []
    controls: list[str] = []
    events: list[dict] = []

    for row in rows:
        name = row["path"]
        rate, rebases, corrected = row["rate"], row["rebases"], row["corrected"]
        if rate is None:
            continue

        # R1：缺计数就是"不可判"，不参与账目，也不算门沉默。
        if rebases is None:
            not_measured.append(name)
        # R2：两个字段都出版时才校核一致性（早期 schema 只出 rebases）。
        if rebases is not None and corrected is not None:
            if (rebases > 0) != (corrected > 0):
                problems.append(
                    f"R2 {name}: rebases={rebases} 但 corrected_total_ns={corrected}（计数与修正账不符）"
                )
        # R3：对照轮单列，且不许出现复位。
        if row.get("gate") is False:
            controls.append(name)
            if rebases not in (None, 0):
                problems.append(f"R3 {name}: 对照轮 drift_gate.enabled=false 却 rebases={rebases}")
        # R4：给了注入参数就必须出版"确实注入了"。
        if (row.get("offset") or 0) > 0:
            armed = row.get("armed")
            if armed is None or armed < 0:
                problems.append(f"R4 {name}: 注入 offset_ns={row['offset']} 但 armed_at_position_ns={armed}")
        # R5：事件分类。
        if isinstance(rate, (int, float)) and abs(rate - 1.0) >= EVENT_THRESHOLD:
            events.append(row)

    injected = [r for r in events if is_injected(r)]
    natural = [r for r in events if not is_injected(r)]
    # 门沉默只统计"计数确实出版且为 0"的自然事件轮；not_measured 不混入。
    silent = [r for r in natural if r["rebases"] == 0 and r.get("gate") is not False]

    report = {
        "rows": len(rows),
        "not_measured": not_measured,
        "controls": controls,
        "events": events,
        "injected": injected,
        "natural": natural,
        "silent": silent,
        "rate_min": min((r["rate"] for r in events), default=None),
        "rate_max": max((r["rate"] for r in events), default=None),
        "rebases_max": max((r["rebases"] or 0 for r in rows), default=0),
        "rebases_sum": sum((r["rebases"] or 0) for r in rows),
    }
    return problems, report


def self_test() -> int:
    """对 R2/R3/R4/R1-R5 各注入一处破坏，抓不到就是校验器失效。"""
    rows = load_rows()
    baseline, base_report = evaluate(rows)
    print("=== verify_p16_rate_events 变异自检 ===")
    print(f"  baseline problems: {len(baseline)}  rows: {len(rows)}")
    ok = True

    def run(name: str, mutate, check) -> None:
        nonlocal ok
        import copy
        mutated = copy.deepcopy(rows)
        mutate(mutated)
        problems, report = evaluate(mutated)
        caught = check(problems, report, base_report)
        print(f"  {name:30s}: {'CAUGHT' if caught else 'MISSED'}")
        ok &= caught

    # M1 R2：把一笔有复位的修正账清零
    def m1(rs):
        for r in rs:
            if (r["rebases"] or 0) > 0 and (r["corrected"] or 0) > 0:
                r["corrected"] = 0
                return
    run("M1 修正账清零 (R2)", m1,
        lambda p, r, b: any("R2" in x for x in p) and len(p) > len(baseline))

    # M2 R3：给对照轮塞一个复位
    def m2(rs):
        for r in rs:
            if r["gate"] is False:
                r["rebases"] = 3
                return
    run("M2 对照轮出现复位 (R3)", m2,
        lambda p, r, b: any("R3" in x for x in p) and len(p) > len(baseline))

    # M3 R4：把注入轮的 armed 改成"没注入"
    def m3(rs):
        for r in rs:
            if (r["offset"] or 0) > 0:
                r["armed"] = -1
                return
    run("M3 注入没生效 (R4)", m3,
        lambda p, r, b: any("R4" in x for x in p) and len(p) > len(baseline))

    # M4 R1/R5：把一份有计数的事件轮改成不可判，必须进 not_measured 而不是被当成门沉默
    def m4(rs):
        for r in rs:
            if isinstance(r["rate"], float) and abs(r["rate"] - 1.0) >= EVENT_THRESHOLD and r["rebases"] is not None:
                r["rebases"] = None
                return
    run("M4 计数缺失不冒充沉默", m4,
        lambda p, r, b: len(r["not_measured"]) > len(b["not_measured"]))

    # M5 R5：把一份自然事件轮的 rate 抹平，事件数必须减少
    def m5(rs):
        for r in rs:
            if isinstance(r["rate"], float) and abs(r["rate"] - 1.0) >= EVENT_THRESHOLD and not is_injected(r):
                r["rate"] = 1.0
                return
    run("M5 事件轮被抹平 (R5)", m5,
        lambda p, r, b: len(r["events"]) < len(b["events"]))

    return 0 if ok else 2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="跑变异自检后退出")
    args = parser.parse_args()

    rows = load_rows()
    if not rows:
        print("FATAL: 没找到含 timeline.playback_rate 的 P13 证据", file=sys.stderr)
        return 2
    if args.self_test:
        return self_test()

    problems, r = evaluate(rows)
    print("=== P13 频差事件账目重算 ===")
    print(f"  含 playback_rate 的证据    : {r['rows']} 份")
    print(f"  缺 drift_rebases（不可判） : {len(r['not_measured'])} 份  [单列，不冒充门沉默]")
    print(f"  drift_gate 关闭的对照轮    : {len(r['controls'])} 份  [单列，不混入沉默]")
    print(f"  事件轮 |rate-1| >= 0.005  : {len(r['events'])} 份 "
          f"(注入 {len(r['injected'])} / 自然 {len(r['natural'])})")
    print(f"  事件轮 rate 范围           : {r['rate_min']} .. {r['rate_max']}")
    print(f"  drift_rebases 合计 / 单轮最大: {r['rebases_sum']} / {r['rebases_max']}")

    print(f"--- 自然事件轮里门没响（rebases=0）{len(r['silent'])} 份，逐份列出 ---")
    for row in r["silent"]:
        print(f"  {row['path']}  rate={row['rate']}")
    if r["not_measured"]:
        print(f"--- 不可判样例（前 5 / 共 {len(r['not_measured'])}）---")
        for name in r["not_measured"][:5]:
            print(f"  {name}")

    if problems:
        print(f"--- FAIL: {len(problems)} 项 ---")
        for line in problems[:20]:
            print(f"  {line}")
        return 1
    print("PASS: R1–R5 账目自洽（缺字段单列、对照轮单列、门沉默明示）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
