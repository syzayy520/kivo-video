#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B34：钉住 pause-resume 的三处修复不许被悄悄改回去。

单一职责：只检查这几条形状是否还在。不判断设备数值、不 import 项目代码、
不改任何文件、不替代其它校验器。设计口径与真机数字见
`docs/design/backend-core/204-p16-p1b27-pause-resume-design.md` 第八节。

背景（三处都是真机才看得见，`b6e950ee` 实测）：
    1. `run_started_at_` 在 `begin_run()` 里无条件重置 → pause 轮 `wall_us` 只剩
       5.02 s（片源 35 s），`speed_vs_realtime` 算出 6.97。
    2. `finish()` 里 `wall_micros` 是"只让第一次生效"（CAS expected=0），
       pause 那次已写入 5.0 s，不单独归零就永远停在 5.0 s。
    3. pause 后音频 timestamp stale → tracker 走**外推**分支
       `position_ns_ += wall_delta`（实测 2.044 s），位置永久超前，丢 58 帧；
       `phase_error` 同时挂着 −2.000 s —— 同一个根因。

判据：
    P1 `run_started_at_` 的赋值只在 `if (!preserve_stats)` 分支内
    P2 preserve 分支里有 `counters_.wall_micros.store(0`
    P3 `started_at` 用 `run_started_at_ + ...paused_total_ns_`
    P4 `skip_pause` 存在，由 `FramePacer::resume_at()` 调用，且实现里前移
       `previous_mono_ns_`、清 stale 计时、重置估计器
    P5 `pause_count` / `pause_total_us` 出版到 decode 证据
    P6 CI hard-gates 含本校验器，且该段无 continue-on-error

行为侧另见 `tools/p16_behavior_probe.cpp` 的 T16/T17（需真机，不进 CI）。

退出码：0 = 全部守住；1 = 有判据失败；2 = 校验器自身失效。
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FILES = {
    "session_cxx": "backend/src/video/video_plane/backends/mediacodec/session/decode_session.cxx",
    "tracker_cxx": "backend/src/video/video_plane/core/pace/clock_position_tracker.cxx",
    "pacer_cxx": "backend/src/video/video_plane/core/pace/frame_pacer.cxx",
    "stats_hpp": "backend/include_private/video/video_plane/backends/mediacodec/session/decode_stats.hpp",
    "renderer": "backend/src/video/video_plane/backends/mediacodec/session/decode_stats_renderer.cxx",
    "ci": ".github/workflows/p2-ci-build-test.yml",
}


def _slice_block(text: str, start_marker: str, end_marker: str) -> str:
    """取 [start_marker, end_marker) 之间的片段；缺任一端返回空串。"""
    i = text.find(start_marker)
    if i < 0:
        return ""
    j = text.find(end_marker, i)
    return text[i:j] if j > 0 else text[i:]


def _assignment_inside_preserve(body: str) -> bool:
    """`run_started_at_ = SteadyClock::now()` 是否真的落在 preserve 块内。

    不能只比较"赋值位置 vs 第一个 `if (!preserve_stats)`"——`begin_run()` 开头
    还有另一个 preserve 分支（`counters_.reset()` 那块），赋值天然在它之后，
    那样判会永远 PASS（自检 M1 就是这么 MISSED 的）。
    这里用大括号深度跟踪：进入 preserve 块后，赋值出现在深度归零之前才算数。
    """
    inside = False
    depth = 0
    for line in body.splitlines():
        if not inside and "if (!preserve_stats)" in line:
            inside = True
            depth = 0
        if inside:
            depth += line.count("{") - line.count("}")
            if "run_started_at_ = SteadyClock::now()" in line:
                return True
            if depth <= 0:
                inside = False
    return False


def check_p1(texts: dict[str, str]) -> list[str]:
    """`run_started_at_` 的赋值必须落在 preserve 判断之内。"""
    body = _slice_block(texts["session_cxx"], "SessionOutcome DecodeSession::begin_run",
                        "DecodeSubmitWorker::Deps submit_deps")
    if not body:
        return ["P1 begin_run() 找不到，判据失效"]
    problems = []
    if "run_started_at_ = SteadyClock::now()" not in body:
        problems.append("P1 begin_run() 里没有 run_started_at_ 的赋值，判据失效")
    elif not _assignment_inside_preserve(body):
        problems.append("P1 run_started_at_ 不在 if (!preserve_stats) 块内 —— "
                        "pause 轮的 wall_us 会只剩 resume 之后那段")
    return problems


def check_p2_p3(texts: dict[str, str]) -> list[str]:
    """preserve 分支要单独归零 wall_micros，并把 started_at 加上 pause 偏移。"""
    problems = []
    if "counters_.wall_micros.store(0" not in texts["session_cxx"]:
        problems.append("P2 缺少 counters_.wall_micros.store(0) —— "
                        "finish() 的 CAS(expected=0) 只让第一次生效，"
                        "pause 后 wall_us 会永远停在 pause 那一刻")
    if "run_started_at_ + std::chrono::nanoseconds(paused_total_ns_)" not in texts["session_cxx"]:
        problems.append("P3 started_at 没加 paused_total_ns_ —— "
                        "wall_us 会把 pause 时长算进播放时长")
    return problems


def check_p4(texts: dict[str, str]) -> list[str]:
    """skip_pause 必须存在、被 resume_at 调用，且三件事都做。"""
    problems = []
    tracker = texts["tracker_cxx"]
    impl = _slice_block(tracker, "void ClockPositionTracker::skip_pause",
                        "void ClockPositionTracker::reset()")
    if not impl:
        problems.append("P4 ClockPositionTracker::skip_pause 不存在")
    else:
        if "previous_mono_ns_ = now_ns" not in impl:
            problems.append("P4 skip_pause 没有前移 previous_mono_ns_ —— "
                            "resume 后第一次读的 wall_delta 就是 pause 时长")
        if "stale_since_mono_ns_ = 0" not in impl:
            problems.append("P4 skip_pause 没清 stale 计时 —— resume 后可能直接进 FreeRun")
        if "rate_estimator_.reset()" not in impl:
            problems.append("P4 skip_pause 没重置频差估计器 —— phase_error 会挂着 −pause")
    if "tracker_.skip_pause(now_ns)" not in texts["pacer_cxx"]:
        problems.append("P4 FramePacer::resume_at() 没调用 tracker_.skip_pause(now_ns)")
    return problems


def check_p5(texts: dict[str, str]) -> list[str]:
    """pause 账要随证据出版，否则 sync 的 clock_off 没有上下文。"""
    problems = []
    if "pause_count" not in texts["stats_hpp"] or "pause_total_us" not in texts["stats_hpp"]:
        problems.append("P5 DecodeStats 里没有 pause_count / pause_total_us")
    renderer = texts["renderer"]
    # 注意：renderer 里是转义后的 `\"pause_count\"`，所以不能匹配裸引号 ——
    # 直接钉"出版时引用了字段"，那才是真正必要的那一步。
    if "stats.pause_count" not in renderer or "stats.pause_total_us" not in renderer:
        problems.append("P5 renderer 没出版 pause_count / pause_total_us —— "
                        "sync 的 clock_off 会被读成时钟跑偏")
    return problems


def check_p6(texts: dict[str, str]) -> list[str]:
    """CI 必须把本校验器跑起来，且不许 continue-on-error。"""
    lines = texts["ci"].splitlines()
    try:
        start = next(i for i, l in enumerate(lines) if l.strip() == "hard-gates:")
    except StopIteration:
        return ["P6 CI 里找不到 hard-gates job"]
    section = "\n".join(lines[start:])
    problems = []
    if "verify_p16_pause_gate.py" not in section:
        problems.append("P6 CI hard-gates 没跑 verify_p16_pause_gate.py")
    if "continue-on-error" in section:
        problems.append("P6 CI hard-gates 出现 continue-on-error —— 门禁可被跳过")
    return problems


def evaluate(texts: dict[str, str]) -> list[str]:
    """纯函数：只吃文本，便于变异自检注入。"""
    return (check_p1(texts) + check_p2_p3(texts) + check_p4(texts)
            + check_p5(texts) + check_p6(texts))


def load_texts() -> dict[str, str]:
    return {k: (ROOT / rel).read_text(encoding="utf-8") for k, rel in FILES.items()}


def self_test(texts: dict[str, str]) -> bool:
    """注入 5 处破坏，必须全部被抓到；抓不到即校验器失效。"""
    baseline = evaluate(texts)
    if baseline:
        print("  baseline 不是 0 问题，先修代码再跑自检：")
        for p in baseline:
            print("   -", p)
        return False

    cases = [
        ("M1 把 run_started_at_ 移出 preserve 分支",
         {"session_cxx": texts["session_cxx"].replace(
             "    if (!preserve_stats) {\n        run_started_at_ = SteadyClock::now();",
             "    run_started_at_ = SteadyClock::now();\n    if (!preserve_stats) {")},
         "P1"),
        ("M2 删掉 wall_micros 归零",
         {"session_cxx": texts["session_cxx"].replace(
             "counters_.wall_micros.store(0, std::memory_order_relaxed);", "")},
         "P2"),
        ("M3 删掉 skip_pause 的调用",
         {"pacer_cxx": texts["pacer_cxx"].replace("tracker_.skip_pause(now_ns);", "")},
         "P4"),
        ("M4 删掉 pause 账出版",
         {"renderer": texts["renderer"].replace("stats.pause_count", "stats.nope")},
         "P5"),
        ("M5 CI 加回 continue-on-error",
         {"ci": texts["ci"] + "      # continue-on-error: true\n"},
         "P6"),
    ]

    ok = True
    for name, patch, expected in cases:
        mutated = dict(texts)
        mutated.update(patch)
        problems = evaluate(mutated)
        caught = any(p.startswith(expected) for p in problems) and len(problems) > len(baseline)
        print(f"  {name:34s}: {'CAUGHT' if caught else 'MISSED'} ({expected})")
        ok &= caught
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true",
                        help="注入 5 处变异，确认每条判据都能抓到")
    args = parser.parse_args()

    missing = [rel for rel in FILES.values() if not (ROOT / rel).is_file()]
    if missing:
        print("FATAL: 缺文件，判据失效：", file=sys.stderr)
        for m in missing:
            print("  -", m, file=sys.stderr)
        return 2

    texts = load_texts()

    if args.self_test:
        print("=== pause 门禁变异自检 ===")
        ok = self_test(texts)
        print("SELFTEST:", "PASS" if ok else "FAIL")
        return 0 if ok else 2

    problems = evaluate(texts)
    if problems:
        print("FAIL: pause-resume 的修复被改回去了：", file=sys.stderr)
        for p in problems:
            print("  -", p, file=sys.stderr)
        return 1
    print("PASS: P1–P6 全部守住（wall 语义 / 单独归零 / skip_pause / pause 出版 / CI 硬门禁）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
