#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B4：钉住 P16 新增脚本不得再绑定本机绝对路径。

单一职责：只检查工具链路径的**来源**是否可移植。不改历史脚本、不判历史脚本红、
不 import 项目代码、不替代其它门禁。

背景：AGENTS.md §0.3「换机器必补三样」；186 的行为测试脚本一度把
C:/Android/sdk 与 windows-x86_64 写死，整目录拷走后就会以「NDK 没装」的样子失败。

判据：
    P1 P16 新增的 6 个脚本里不得出现硬编码盘符 / SDK / FFmpeg 绝对路径
    P2 行为测试脚本通过 resolve_sdk_root() 三级解析，含 --self-check 与 prebuilt/* 通配
    P3 CI hard-gates 含本门禁且该段无 continue-on-error

历史 tools/*.py 的硬编码路径只统计、不判红（豁免明示）。

退出码：0 = 全部守住；1 = 有判据失败；2 = 校验器自身失效。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"

P16_SCRIPTS = (
    "verify_source_manifest.py",
    "verify_p16_p0_gates.py",
    "verify_p16_rate_events.py",
    "verify_p16_burst_gate.py",
    "run_p16_burst_behavior_test.py",
    "run_p16_seek_gate_probe.py",
    "verify_p16_seek_gate.py",
    "verify_p16_perf_baseline.py",
    "verify_p16_pause_gate.py",
    "verify_tools_portability.py",
)
BEHAVIOR = "run_p16_burst_behavior_test.py"
CI_REL = ".github/workflows/p2-ci-build-test.yml"

# 只抓引号里**成形的路径字面量**；像 "^\s*hard-gates:\s*$" 这种正则转义不会误判。
HARDCODED = re.compile(
    r'''["'](?:(?:[A-Za-z]:[\\/]|/c/|/f/|/d/)[^"']*?(?:Android|sdk|ffmpeg|kivo|Users|Files))''',
    re.IGNORECASE,
)


def load_texts() -> dict[str, str]:
    out: dict[str, str] = {}
    for name in P16_SCRIPTS:
        path = TOOLS / name
        if not path.is_file():
            raise FileNotFoundError(f"缺 P16 脚本: tools/{name}")
        out[name] = path.read_text(encoding="utf-8")
    ci = ROOT / CI_REL
    if not ci.is_file():
        raise FileNotFoundError(f"缺文件: {CI_REL}")
    out["ci"] = ci.read_text(encoding="utf-8")
    return out


def check_p1(texts: dict[str, str]) -> list[str]:
    problems: list[str] = []
    for name in P16_SCRIPTS:
        for number, line in enumerate(texts[name].splitlines(), 1):
            if HARDCODED.search(line):
                problems.append(f"P1 tools/{name}:{number} 出现硬编码本机路径: {line.strip()[:100]}")
    return problems


def check_p2(behavior: str) -> list[str]:
    problems: list[str] = []
    for token in ("resolve_sdk_root", "ANDROID_HOME", "local.properties",
                  "--self-check", "prebuilt"):
        if token not in behavior:
            problems.append(f"P2 {BEHAVIOR} 缺少 {token}（SDK 来源不可移植或无法自检）")
    return problems


def check_p3(ci: str) -> list[str]:
    problems: list[str] = []
    match = re.search(r"^  hard-gates:\s*$", ci, re.M)
    if match is None:
        problems.append("P3 CI 里没有 hard-gates job")
        return problems
    block = ci[match.start():]
    if "verify_tools_portability.py" not in block:
        problems.append("P3 hard-gates 没有接入本门禁")
    if "continue-on-error" in block:
        problems.append("P3 hard-gates 出现 continue-on-error")
    return problems


def evaluate(texts: dict[str, str]) -> list[str]:
    """纯函数：只吃文本，便于变异自检注入。"""
    return (check_p1(texts)
            + check_p2(texts[BEHAVIOR])
            + check_p3(texts["ci"]))


def self_test() -> int:
    texts = load_texts()
    baseline = evaluate(texts)
    print("=== verify_tools_portability 变异自检 ===")
    print(f"  baseline problems: {len(baseline)}")
    cases: list[tuple[str, dict[str, str], str]] = [
        # 拼接构造，避免变异用例自身成为门禁脚本里的硬编码路径字面量。
        ("M1 注入硬编码 SDK 路径", {BEHAVIOR: texts[BEHAVIOR] + "\nSDK_FALLBACK = \""
                                    + "C:" + "/Android/sdk" + "\"\n"}, "P1"),
        ("M2 去掉三级解析", {BEHAVIOR: texts[BEHAVIOR].replace(
            "resolve_sdk_root", "removed_resolver")}, "P2"),
        ("M3 CI 加回 continue-on-error", {"ci": texts["ci"] + "      # continue-on-error: true\n"}, "P3"),
    ]
    ok = True
    for name, patch, tag in cases:
        mutated = dict(texts)
        mutated.update(patch)
        problems = evaluate(mutated)
        caught = any(p.startswith(tag) for p in problems) and len(problems) > len(baseline)
        print(f"  {name:28s}: {'CAUGHT' if caught else 'MISSED'} ({tag})")
        ok &= caught
    return 0 if ok else 2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="跑变异自检后退出")
    args = parser.parse_args()
    try:
        texts = load_texts()
    except FileNotFoundError as error:
        print(f"FATAL: {error}", file=sys.stderr)
        return 2
    if args.self_test:
        return self_test()

    problems = evaluate(texts)
    historical = [p.name for p in sorted(TOOLS.glob("*.py"))
                  if HARDCODED.search(p.read_text(encoding="utf-8", errors="ignore"))]
    print("=== P16 工具链可移植性 ===")
    print(f"  P16 脚本: {len(P16_SCRIPTS)} 份（判红范围）")
    print(f"  历史 tools/*.py 中带硬编码路径: {len(historical)} 份 —— 只统计、不判红")
    for name in historical:
        print(f"    {name}")
    if problems:
        print(f"--- FAIL: {len(problems)} 项 ---")
        for line in problems:
            print(f"  {line}")
        return 1
    print("PASS: P1–P3 全部守住（无硬编码路径 / SDK 三级解析 / CI 硬门禁）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
