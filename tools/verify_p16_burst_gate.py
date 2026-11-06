#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B2：钉住「AAudio burst 兜底不许再变回静默降级」。

单一职责：只检查 G9 这条约束的形状是否还在。不判断设备数值、不 import 项目代码、
不改任何文件、不替代其它校验器。设计口径见
`docs/design/backend-core/185-p16-p1b2-burst-g9-plan.md` 第三节。

背景：`AAudioStream_getFramesPerBurst` 返回 <= 0 时，旧实现会让 AaudioClockSource
悄悄退回 192（K40 的 4 ms）。证据里看起来像"设备协商到 192"，而别的设备上
stale_after_ns 会把新鲜误判成陈旧。修复后由 AaudioOutputSession::open() 明确失败并带
非空 reason，走既有 code+message → audio.error → last_error → UI 通道。

判据：
    B1 开流侧在构造时钟之前有 frames_per_burst <= 0 的显式拒绝分支
    B2 该分支设 kOpenFailed，message 含 refusing to fall back 与 device burst
    B3 AaudioClockSource 提供 used_fallback() / fallback_reason()，实现里记录该状态
    B4 192 自卫兜底仍在，且注释标明产品路径已在上游拒绝
    B5 CI hard-gates 含本校验器，且该段无 continue-on-error
    B6 失败链中间/下游接线仍在（audio_playback 的 stats_.error 拼接 +
       av_sync_report 的 audio.error / completed），否则 reason 到不了用户眼前

P16-P1B3 追加 B6/M6；行为侧另见 tools/run_p16_burst_behavior_test.py（需真机，不进 CI）。

退出码：0 = 全部守住；1 = 有判据失败；2 = 校验器自身失效。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FILES = {
    "session": "backend/src/video/video_plane/backends/aaudio/session/aaudio_output_session.cxx",
    "clock_hpp": "backend/include_private/video/video_plane/backends/aaudio/clock/aaudio_clock_source.hpp",
    "clock_cxx": "backend/src/video/video_plane/backends/aaudio/clock/aaudio_clock_source.cxx",
    "ci": ".github/workflows/p2-ci-build-test.yml",
    # B6 只读这两份：失败链的中间与下游接线。
    "audio_playback": "backend/src/video/video_plane/backends/avsync/session/audio_playback.cxx",
    "av_report": "backend/src/video/video_plane/backends/avsync/report/av_sync_report.cxx",
}


def load_texts() -> dict[str, str]:
    out: dict[str, str] = {}
    for key, rel in FILES.items():
        path = ROOT / rel
        if not path.is_file():
            raise FileNotFoundError(f"缺文件: {rel}")
        out[key] = path.read_text(encoding="utf-8")
    return out


def check_b1_b2(session: str) -> list[str]:
    problems: list[str] = []
    guard = session.find("frames_per_burst <= 0")
    clock_ctor = session.find("make_unique<AaudioClockSource>")
    if guard < 0:
        problems.append("B1 开流侧没有 frames_per_burst <= 0 的显式拒绝分支")
        return problems
    if clock_ctor >= 0 and guard > clock_ctor:
        problems.append("B1 拒绝分支出现在构造时钟之后，兜底已经生效了才检查")
    block = session[guard:clock_ctor if clock_ctor > guard else guard + 800]
    if "kOpenFailed" not in block:
        problems.append("B2 拒绝分支没有设 kOpenFailed，失败通道不会触发")
    if "refusing to fall back" not in block or "device burst" not in block:
        problems.append("B2 拒绝分支的 message 缺少非空、可读的 reason（G9）")
    if not re.search(r"outcome_\.message\s*=", block):
        problems.append("B2 拒绝分支没有写 outcome_.message")
    return problems


def check_b3_b4(clock_hpp: str, clock_cxx: str) -> list[str]:
    problems: list[str] = []
    for token in ("used_fallback()", "fallback_reason()"):
        if token not in clock_hpp:
            problems.append(f"B3 头文件缺少 {token}，兜底状态不可查询")
    if "used_fallback_(" not in clock_cxx:
        problems.append("B3 构造没有记录 used_fallback_ 状态")
    if "AaudioClockSource::fallback_reason" not in clock_cxx:
        problems.append("B3 缺少 fallback_reason() 实现")
    if "kDefaultFramesPerBurst = 192" not in clock_cxx:
        problems.append("B4 192 自卫兜底不见了（传 0 会算出非正陈旧阈值）")
    if "产品路径" not in clock_cxx or "G9" not in clock_cxx:
        problems.append("B4 兜底常量缺少「产品路径已在上游拒绝」的说明")
    return problems


def check_b5(ci: str) -> list[str]:
    problems: list[str] = []
    match = re.search(r"^  hard-gates:\s*$", ci, re.M)
    if match is None:
        problems.append("B5 CI 里没有 hard-gates job")
        return problems
    block = ci[match.start():]
    if "verify_p16_burst_gate.py" not in block:
        problems.append("B5 hard-gates 没有接入本校验器")
    if "continue-on-error" in block:
        problems.append("B5 hard-gates 出现 continue-on-error，门禁退回只打印不失败")
    return problems


def check_b6(audio_playback: str, av_report: str) -> list[str]:
    """失败链的中间与下游接线：reason 必须能走到用户眼前。

    只读这两份文件——尤其 av_sync_report.cxx 正被他人主线改动，本校验器不写它。
    若对方重构了 audio.error 的写法，这里会红：那是有意的接线声明，
    收口文档必须写明「报表重构时需同步更新 B6」，不要当成代码缺陷。
    """
    problems: list[str] = []
    if '"aaudio open: "' not in audio_playback or "stats_.error" not in audio_playback:
        problems.append(
            'B6 audio_playback.cxx 丢了 stats_.error = "aaudio open: " + opened.message 接线，'
            "reason 到不了 audio.error"
        )
    if "audio.error" not in av_report or "completed" not in av_report:
        problems.append("B6 av_sync_report.cxx 丢了 audio.error / completed 接线")
    return problems


def evaluate(texts: dict[str, str]) -> list[str]:
    """纯函数：只吃文本，便于变异自检注入。"""
    return (check_b1_b2(texts["session"])
            + check_b3_b4(texts["clock_hpp"], texts["clock_cxx"])
            + check_b5(texts["ci"])
            + check_b6(texts["audio_playback"], texts["av_report"]))


def self_test() -> int:
    texts = load_texts()
    baseline = evaluate(texts)
    print("=== verify_p16_burst_gate 变异自检 ===")
    print(f"  baseline problems: {len(baseline)}")
    cases: list[tuple[str, dict[str, str], str]] = [
        ("M1 删掉拒绝分支", {"session": texts["session"].replace(
            "if (outcome_.frames_per_burst <= 0) {", "if (false) {")}, "B1"),
        ("M2 清空 reason", {"session": texts["session"].replace(
            "refusing to fall back to 192 because device burst is required", "no reason")}, "B2"),
        ("M3 去掉可查询兜底", {"clock_hpp": texts["clock_hpp"].replace(
            "used_fallback()", "removed_getter()")}, "B3"),
        ("M4 删掉 192 自卫", {"clock_cxx": texts["clock_cxx"].replace(
            "kDefaultFramesPerBurst = 192", "kDefaultFramesPerBurst = 0")}, "B4"),
        ("M5 CI 加回 continue-on-error", {"ci": texts["ci"] + "      # continue-on-error: true\n"}, "B5"),
        ("M6 删掉失败链拼接", {"audio_playback": texts["audio_playback"].replace(
            '"aaudio open: "', '"open failed: "')}, "B6"),
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
    print("=== burst 兜底 G9 门禁 ===")
    if problems:
        print(f"--- FAIL: {len(problems)} 项 ---")
        for line in problems:
            print(f"  {line}")
        return 1
    print("PASS: B1–B6 全部守住（显式拒绝 / 非空 reason / 可查询兜底 / 自卫保留 / CI 硬门禁 / 失败链接线）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
