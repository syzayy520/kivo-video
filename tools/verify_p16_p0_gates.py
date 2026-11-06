#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1A：钉住 P0 四条加固，防止后续改动悄悄把它们回退掉。

单一职责：只判断「机制的形状还在不在」，**不判断性能数字、不判断证据内容**。
不 import 任何项目代码，不修改任何文件，不替代 `verify_source_manifest.py`
（那个守的是源文件注册，本脚本守的是加固机制）。

设计口径见 `docs/design/backend-core/182-p16-p1a-plan.md` 第三节。

G1 JNI 边界覆盖    每个 JNIEXPORT 导出都成对出现 KIVO_JNI_BEGIN / KIVO_JNI_END_*
G2 分配钩子默认     option 默认 ON、release 传 OFF、hook 源码有对应 #if
G3 停止可观测       有界等待在 join 之前、超时 reason 非空、worker 报数会复位
G4 CI 硬门禁        hard-gates 存在、无 continue-on-error、含三个检查、分支过滤含 codex/**
G5 文件纪律         bridge < 300 行、边界头存在、M3 仍要求 hook_installed

退出码：0 = 全部守住；1 = 有判据失败；2 = 校验器自身失效。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

FILES = {
    "bridge": "android/app/src/main/cpp/jni/jni_player_bridge.cxx",
    "cmake": "android/app/src/main/cpp/CMakeLists.txt",
    "gradle": "android/app/build.gradle.kts",
    "hook": "backend/src/video/video_plane/core/metrics/alloc_probe_hook.cxx",
    "session": "backend/src/video/video_plane/backends/mediacodec/session/decode_session.cxx",
    "gate_cxx": "backend/src/video/video_plane/backends/mediacodec/session/decode_pipeline_gate.cxx",
    "ci": ".github/workflows/p2-ci-build-test.yml",
    "metrics": "tools/verify_p14a_metrics.py",
}
GUARD_HEADER = ROOT / "android/app/src/main/cpp/jni/jni_call_guard.hpp"


def load_texts() -> dict[str, str]:
    out: dict[str, str] = {}
    for key, rel in FILES.items():
        path = ROOT / rel
        if not path.is_file():
            raise FileNotFoundError(f"缺文件: {rel}")
        out[key] = path.read_text(encoding="utf-8")
    return out


def check_g1(bridge: str) -> list[str]:
    problems: list[str] = []
    exports = len(re.findall(r"\bJNIEXPORT\b", bridge))
    begins = len(re.findall(r"\bKIVO_JNI_BEGIN\b", bridge))
    ends = len(re.findall(r"\bKIVO_JNI_END_(?:INT|LONG|VOID|STRING)\b", bridge))
    if exports == 0:
        problems.append("G1 一个 JNIEXPORT 都没找到，bridge 可能被换掉了")
    if not (exports == begins == ends):
        problems.append(
            f"G1 边界数量不齐: JNIEXPORT={exports} BEGIN={begins} END={ends} "
            "(新增导出漏接异常边界，或宏被删)"
        )
    segments = [s for s in re.split(r"(?=^JNIEXPORT)", bridge, flags=re.M) if "JNIEXPORT" in s]
    for index, segment in enumerate(segments, 1):
        if "KIVO_JNI_BEGIN" not in segment:
            problems.append(f"G1 第 {index} 个导出缺少 KIVO_JNI_BEGIN")
        if not re.search(r"\bKIVO_JNI_END_(?:INT|LONG|VOID|STRING)\b", segment):
            problems.append(f"G1 第 {index} 个导出缺少 KIVO_JNI_END_*")
    return problems


def check_g2(cmake: str, gradle: str, hook: str) -> list[str]:
    problems: list[str] = []
    if not re.search(r"option\(\s*KIVO_ENABLE_GLOBAL_ALLOC_HOOK[^)]*\bON\s*\)", cmake):
        problems.append("G2 CMake 的 KIVO_ENABLE_GLOBAL_ALLOC_HOOK 默认值不是 ON")
    if "if(NOT KIVO_ENABLE_GLOBAL_ALLOC_HOOK)" not in cmake or "KIVO_DISABLE_GLOBAL_ALLOC_HOOK" not in cmake:
        problems.append("G2 CMake 没有把 OFF 下传为编译定义")
    if not re.search(r"release\s*\{[\s\S]{0,600}?KIVO_ENABLE_GLOBAL_ALLOC_HOOK=OFF", gradle):
        problems.append("G2 release flavor 没有传 -DKIVO_ENABLE_GLOBAL_ALLOC_HOOK=OFF")
    if gradle.count("KIVO_ENABLE_GLOBAL_ALLOC_HOOK=OFF") != 1:
        problems.append("G2 OFF 开关出现次数不为 1（debug 取证口径可能被一起关掉）")
    if "#if !defined(KIVO_DISABLE_GLOBAL_ALLOC_HOOK)" not in hook:
        problems.append("G2 alloc_probe_hook.cxx 没有对应的编译开关分支")
    return problems


def check_g3(session: str, gate_cxx: str) -> list[str]:
    problems: list[str] = []
    wait = session.find("wait_for_workers(")
    join = session.find(".join()")
    if wait < 0:
        problems.append("G3 stop() 里的有界等待 wait_for_workers 不见了")
    elif join >= 0 and wait > join:
        problems.append("G3 wait_for_workers 出现在 join 之后，卡死时将没有可读原因")
    if not re.search(r'failure_\.mark\(\s*"stop join timeout:[^"]{10,}', session):
        problems.append("G3 超时分支没有带非空 reason（违反 G9）")
    if session.count("gate_.note_worker_exited()") != 3:
        problems.append("G3 worker 退出报数不是 3 处（split 2 + merged 1），线程模型改动需同步更新本判据")
    for token in ("note_worker_exited", "wait_for_workers", "workers_exited_.store(0"):
        if token not in gate_cxx:
            problems.append(f"G3 闸门实现缺少 {token}（计数不复位会让 seek 后误报超时）")
    return problems


def check_g4(ci: str) -> list[str]:
    problems: list[str] = []
    match = re.search(r"^  hard-gates:\s*$", ci, re.M)
    if match is None:
        problems.append("G4 CI 里没有 hard-gates job")
        return problems
    block = ci[match.start():]
    if "continue-on-error" in block:
        problems.append("G4 hard-gates 出现 continue-on-error，门禁又变回只打印不失败")
    for token in ("verify_source_manifest.py", "verify_p16_p0_gates.py",
                  "Core has zero platform headers"):
        if token not in block:
            problems.append(f"G4 hard-gates 缺少检查: {token}")
    if "codex/**" not in ci[:match.start()]:
        problems.append("G4 push 分支过滤没有 codex/**，当前开发分支永远触发不了门禁")
    return problems


def check_g5(bridge: str, metrics: str, guard_header_exists) -> list[str]:
    problems: list[str] = []
    lines = len(bridge.splitlines())
    if lines >= 300:
        problems.append(f"G5 jni_player_bridge.cxx 有 {lines} 行，越过家族树 §四 的 300 行线")
    if not guard_header_exists():
        problems.append("G5 jni_call_guard.hpp 不见了，JNI 边界失去单一真相")
    if "hook_installed" not in metrics or "M3" not in metrics:
        problems.append(
            "G5 verify_p14a_metrics.py 的 M3 不再守 hook_installed，"
            "OFF flavor 会悄悄产出不能用的 G3 证据"
        )
    return problems


def evaluate(texts: dict[str, str], guard_header_exists) -> list[str]:
    """纯函数：所有判据只吃文本，便于变异自检注入。"""
    problems: list[str] = []
    problems += check_g1(texts["bridge"])
    problems += check_g2(texts["cmake"], texts["gradle"], texts["hook"])
    problems += check_g3(texts["session"], texts["gate_cxx"])
    problems += check_g4(texts["ci"])
    problems += check_g5(texts["bridge"], texts["metrics"], guard_header_exists)
    return problems


def self_test() -> int:
    """每个判据注入一个回退，确认校验器真的会红；抓不到就说明门禁是摆设。"""
    texts = load_texts()
    exists = GUARD_HEADER.is_file
    baseline = evaluate(texts, exists)
    print("=== verify_p16_p0_gates 变异自检 ===")
    print(f"  baseline problems: {len(baseline)}")

    cases: list[tuple[str, dict[str, str], object, str]] = [
        (
            "M1 删掉一个 JNI 边界宏",
            {"bridge": texts["bridge"].replace(
                'KIVO_JNI_END_INT(env, nullptr, "nativeProbeCapabilities")', "/* removed */")},
            exists, "G1",
        ),
        (
            "M2 把分配钩子默认改成 OFF",
            {"cmake": texts["cmake"].replace(
                '"Install process-wide operator new probe" ON',
                '"Install process-wide operator new probe" OFF')},
            exists, "G2",
        ),
        (
            "M3 删掉 stop 的有界等待",
            {"session": texts["session"].replace(
                "gate_.wait_for_workers(expected_workers, kStopJoinBudgetUs)", "true")},
            exists, "G3",
        ),
        (
            "M4 给硬门禁加回 continue-on-error",
            {"ci": texts["ci"] + "      # continue-on-error: true\n"},
            exists, "G4",
        ),
        (
            "M5 bridge 撑过 300 行",
            {"bridge": texts["bridge"] + ("// pad\n" * 20)},
            exists, "G5",
        ),
        (
            "M5b M3 被删掉",
            {"metrics": texts["metrics"].replace("hook_installed", "no_longer_checked")},
            exists, "G5",
        ),
        ("M5c 边界头消失", {}, lambda: False, "G5"),
    ]

    ok = True
    for name, patch, exists_fn, expected in cases:
        mutated = dict(texts)
        mutated.update(patch)
        problems = evaluate(mutated, exists_fn)
        caught = any(p.startswith(expected) for p in problems) and len(problems) > len(baseline)
        print(f"  {name:26s}: {'CAUGHT' if caught else 'MISSED'} ({expected})")
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

    problems = evaluate(texts, GUARD_HEADER.is_file)
    print("=== P0 加固防回退门禁 ===")
    # 反斜杠不能放进 f-string 表达式（Python < 3.12 语法错误），先算再印
    jni_export_count = len(re.findall(r"\bJNIEXPORT\b", texts["bridge"]))
    print(f"  JNI 导出: {jni_export_count} 个")
    print(f"  bridge 行数: {len(texts['bridge'].splitlines())}")
    if problems:
        print(f"--- FAIL: {len(problems)} 项 ---")
        for line in problems:
            print(f"  {line}")
        return 1
    print("PASS: G1–G5 全部守住（JNI 边界 / 分配钩子 / 停止可观测 / CI 硬门禁 / 文件纪律）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
