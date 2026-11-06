#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B7：钉住 seek 栅栏不再靠固定睡眠，且超时带非空 reason。

单一职责：只检查 A2 实现卡的形状。不判断 seek 数字、不改证据、不 import 项目代码、
不替代其它门禁。设计口径见 docs/design/backend-core/190-p16-p1b7-seek-gate-design.md。

背景：旧实现 `set_muted(true)` 后固定 `sleep_for(10ms)`，正确性挂在一台设备的
burst 时长上（"K40 burst 192 帧 ≈ 4 ms，取 10 ms 留余量"）。P1-B6 加了早于判空的
in_flight() 计数，本卡用它做显式凭据：等归零、10 ms 只作 deadline 上限。

判据：
    S1 不再有固定 sleep_for(10ms) 的静音栅栏
    S2 有 in_flight() 等待 + deadline，且超时分支写非空 reason 并解除静音
    S3 CI hard-gates 含本门禁且该段无 continue-on-error

退出码：0 = 全部守住；1 = 有判据失败；2 = 校验器自身失效。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FILES = {
    "audio": "backend/src/video/video_plane/backends/avsync/session/audio_playback.cxx",
    "source": "backend/include_private/video/video_plane/backends/aaudio/buffer/ring_buffer_sample_source.hpp",
    "budget": "backend/src/video/video_plane/core/seek/seek_budget.cxx",
    "ci": ".github/workflows/p2-ci-build-test.yml",
}


def load_texts() -> dict[str, str]:
    out: dict[str, str] = {}
    for key, rel in FILES.items():
        path = ROOT / rel
        if not path.is_file():
            raise FileNotFoundError(f"缺文件: {rel}")
        out[key] = path.read_text(encoding="utf-8")
    return out


def check_s1_s2(audio: str, source: str) -> list[str]:
    problems: list[str] = []
    if re.search(r"sleep_for\(std::chrono::milliseconds\(10\)\)", audio):
        problems.append("S1 静音栅栏又回到固定 sleep_for(10ms)，显式凭据被绕过")
    if "in_flight() != 0" not in audio or "gate_deadline" not in audio:
        problems.append("S2 缺少 in_flight() 等待或 deadline（Q4 的超时预算没落地）")
    if "seek gate timeout:" not in audio:
        problems.append("S2 超时分支没有非空 reason（G9）")
    if re.search(r"out\.reason\s*=", audio) is None:
        problems.append("S2 超时没有写回 SeekResult.reason，失败到不了用户眼前")
    block_start = audio.find("seek gate timeout:")
    if block_start < 0 or "set_muted(false)" not in audio[block_start:block_start + 600]:
        problems.append("S2 超时返回前没有解除静音，会留下半截状态")
    # 189 的前提：计数必须早于判空，否则"已过判空"的回调不计入。
    body_start = source.find("std::int32_t render")
    body_end = source.find("/// 只读观测", body_start)
    body = source[body_start:body_end] if body_end > body_start >= 0 else ""
    if not body or body.find("in_flight_.fetch_add") > body.find("muted_.load"):
        problems.append("S2 前提被破坏：in_flight 计数不再早于判空（189 O1）")
    return problems


def check_s3(ci: str) -> list[str]:
    problems: list[str] = []
    match = re.search(r"^  hard-gates:\s*$", ci, re.M)
    if match is None:
        problems.append("S3 CI 里没有 hard-gates job")
        return problems
    block = ci[match.start():]
    if "verify_p16_seek_gate.py" not in block:
        problems.append("S3 hard-gates 没有接入本门禁")
    if "continue-on-error" in block:
        problems.append("S3 hard-gates 出现 continue-on-error")
    return problems


def check_s4(budget: str) -> list[str]:
    """P16-P1B8：首帧 pts 晚于 target 时，归因必须说成落点/关键帧问题，不是解码慢。"""
    problems: list[str] = []
    if "video_first_pts_us > facts.target_us" not in budget:
        problems.append(
            "S4 seek 归因分支不见了：首帧 pts 晚于 target 会被误报成解码慢"
        )
    elif "keyframe-spacing problem, not decode latency" not in budget:
        problems.append("S4 归因分支的 reason 没有区分关键帧间隔与解码延迟")
    return problems


def evaluate(texts: dict[str, str]) -> list[str]:
    """纯函数：只吃文本，便于变异自检注入。"""
    return (check_s1_s2(texts["audio"], texts["source"])
            + check_s3(texts["ci"])
            + check_s4(texts["budget"]))


def self_test() -> int:
    texts = load_texts()
    baseline = evaluate(texts)
    print("=== verify_p16_seek_gate 变异自检 ===")
    print(f"  baseline problems: {len(baseline)}")
    cases: list[tuple[str, dict[str, str], str]] = [
        ("M1 固定睡眠回潮", {"audio": texts["audio"].replace(
            "const auto gate_deadline",
            "std::this_thread::sleep_for(std::chrono::milliseconds(10));\n"
            "    const auto gate_deadline")}, "S1"),
        ("M2 删掉超时 reason", {"audio": texts["audio"].replace(
            "seek gate timeout: audio callback still inside render() after 10 ms; ", "")}, "S2"),
        ("M3 计数挪到判空之后", {"source": texts["source"].replace(
            "        in_flight_.fetch_add(1, std::memory_order_relaxed);\n"
            "        if (muted_.load(std::memory_order_acquire)) {",
            "        if (muted_.load(std::memory_order_acquire)) {\n"
            "            in_flight_.fetch_add(1, std::memory_order_relaxed);")}, "S2"),
        ("M4 CI 加回 continue-on-error", {"ci": texts["ci"] + "      # continue-on-error: true\n"}, "S3"),
        ("M5 删掉 pts 归因分支", {"budget": texts["budget"].replace(
            "facts.video_first_pts_us > facts.target_us", "false")}, "S4"),
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
    print("=== seek 栅栏显式凭据门禁 ===")
    if problems:
        print(f"--- FAIL: {len(problems)} 项 ---")
        for line in problems:
            print(f"  {line}")
        return 1
    print("PASS: S1–S4 全部守住（无固定睡眠 / 显式凭据+超时 reason / CI 硬门禁 / pts 归因分离）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
