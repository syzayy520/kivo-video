#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""校验 Android 与 backend 两份 CMake 源文件清单的注册完整性。

单一职责：只回答「新增/删掉的 .cxx 有没有在两份清单里对齐」。
不 import 任何项目代码，不判断编译结果，不修改任何文件。

为什么需要它（`181-p16-p0-hardening-plan.md` D4）：
`android/app/src/main/cpp/CMakeLists.txt` 与 `backend/CMakeLists.txt` 并列维护同一批
`src/video/video_plane/` 源文件。漏注册一处的报应是「桌面全绿、arm64 链接时炸
undefined symbol」—— 这个错在 Windows 上永远看不出来。

判据（第七节更正后的口径，**比较域按设计收窄，不是放宽**）：
  1. `core/` 是三端共用族：两份清单的差集必须为 0；
  2. 磁盘上 `core/**/*.cxx` 必须被**两份**清单都注册；
  3. 磁盘上 `backends/{mediacodec,aaudio,avsync}/**/*.cxx` 必须被 Android 清单注册；
  4. 清单指向不存在文件的路径一律算错；
  5. 豁免必须显式列出、逐条打印、过期即报错（项目铁律：隐藏换来的全绿比红更危险）；
  6. 同一路径在一份清单里出现多次**只打印不判错** —— CMake 多 target 复用
     同一份纯逻辑源是合法用法（`kivo_p12e1_color_tests` / `kivo_p12e2_hdr_tests`）。

退出码：0 = 一致；1 = 有不一致；2 = 校验器自身失效。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ANDROID_CMAKE = ROOT / "android" / "app" / "src" / "main" / "cpp" / "CMakeLists.txt"
BACKEND_CMAKE = ROOT / "backend" / "CMakeLists.txt"
PLANE_SRC = ROOT / "backend" / "src" / "video" / "video_plane"
PREFIX = "src/video/video_plane/"

# 三端共用、两份清单都该对齐的语义域。
SHARED_FAMILIES = ("core/",)
# Android 专用族：桌面清单本来就不该编它们，只要求 Android 清单覆盖磁盘。
ANDROID_ONLY_FAMILIES = (
    "backends/mediacodec/",
    "backends/aaudio/",
    "backends/avsync/",
)
ALL_FAMILIES = SHARED_FAMILIES + ANDROID_ONLY_FAMILIES

ANDROID_PATTERN = re.compile(
    r"\$\{KIVO_BACKEND\}/(src/video/video_plane/[^\s\"\)]+\.(?:cxx|cpp))"
)
BACKEND_PATTERN = re.compile(
    r"(?<![\w/])(src/video/video_plane/[^\s\"\)]+\.(?:cxx|cpp))"
)

# 已知且经设计确认的差异。**每条都必须带理由**，不许用通配符。
EXEMPT_ONLY_ANDROID: dict[str, str] = {}
EXEMPT_ONLY_BACKEND: dict[str, str] = {}
EXEMPT_CORE_UNREGISTERED: dict[str, str] = {}
EXEMPT_ANDROID_UNREGISTERED: dict[str, str] = {}


def in_scope(rel: str) -> bool:
    return any(rel.startswith(family) for family in ALL_FAMILIES)


def is_shared(rel: str) -> bool:
    return any(rel.startswith(family) for family in SHARED_FAMILIES)


def collect(text: str, pattern: re.Pattern[str]) -> dict[str, int]:
    """抽出范围内的相对路径（去掉 src/video/video_plane/ 前缀），并记出现次数。"""
    found: dict[str, int] = {}
    for match in pattern.finditer(text):
        tail = match.group(1).replace("\\", "/")[len(PREFIX):]
        if in_scope(tail):
            found[tail] = found.get(tail, 0) + 1
    return found


def scan_disk() -> set[str]:
    """磁盘是"该文件存在"的唯一真相来源，而不是任何一份清单。"""
    found: set[str] = set()
    for family in ALL_FAMILIES:
        for suffix in ("*.cxx", "*.cpp"):
            for path in (PLANE_SRC / family).rglob(suffix):
                found.add(path.relative_to(PLANE_SRC).as_posix())
    return found


def evaluate(
    android: dict[str, int],
    backend: dict[str, int],
    disk: set[str],
    exempt_a: dict[str, str],
    exempt_b: dict[str, str],
    exempt_core: dict[str, str],
    exempt_android: dict[str, str],
    exists,
) -> tuple[list[str], list[str], list[str]]:
    """纯函数：返回 (问题列表, 已声明豁免列表, 仅供参考的提示列表)。"""
    problems: list[str] = []
    exemptions: list[str] = []
    notes: list[str] = []

    shared_a = {r for r in android if is_shared(r)}
    shared_b = {r for r in backend if is_shared(r)}
    only_android = sorted(shared_a - shared_b)
    only_backend = sorted(shared_b - shared_a)

    for rel in only_android:
        if rel in exempt_a:
            exemptions.append(f"only-android  {rel}  <- {exempt_a[rel]}")
        else:
            problems.append(f"only-android  {rel}  (core 文件只在 Android 清单里)")
    for rel in only_backend:
        if rel in exempt_b:
            exemptions.append(f"only-backend  {rel}  <- {exempt_b[rel]}")
        else:
            problems.append(f"only-backend  {rel}  (core 文件只在 backend 清单里)")

    # 磁盘覆盖：这是"漏注册 → arm64 链接炸"的直接防线。
    for rel in sorted(r for r in disk if is_shared(r)):
        for side, table, exempt in (
            ("android", android, exempt_core),
            ("backend", backend, exempt_core),
        ):
            if rel in table:
                continue
            key = f"{side}/{rel}"
            if key in exempt:
                exemptions.append(f"unregistered  {key}  <- {exempt[key]}")
            else:
                problems.append(f"unregistered  {key}  (磁盘有此 core 源文件，清单没注册)")
    for rel in sorted(r for r in disk if not is_shared(r)):
        if rel in android:
            continue
        if rel in exempt_android:
            exemptions.append(f"unregistered  android/{rel}  <- {exempt_android[rel]}")
        else:
            problems.append(f"unregistered  android/{rel}  (磁盘有此 Android 后端源文件，清单没注册)")

    for rel in sorted(set(android) | set(backend)):
        if not exists(PREFIX + rel):
            problems.append(f"missing-file  {rel}  (清单指向的文件在 backend/ 下不存在)")

    for table, name in ((exempt_a, "only-android"), (exempt_b, "only-backend")):
        for rel in sorted(table):
            if rel not in (only_android if name == "only-android" else only_backend):
                problems.append(f"stale-exempt  {name}/{rel}  (豁免已不再需要)")
    for rel in sorted(exempt_core):
        side, path = rel.split("/", 1)
        if path in disk and path in (android if side == "android" else backend):
            problems.append(f"stale-exempt  unregistered/{rel}  (豁免已不再需要)")
    for rel in sorted(exempt_android):
        if rel in disk and rel in android:
            problems.append(f"stale-exempt  unregistered/android/{rel}  (豁免已不再需要)")

    for side, table in (("android", android), ("backend", backend)):
        for rel, count in sorted(table.items()):
            if count > 1:
                notes.append(f"duplicate     {side}/{rel}  ({count} 个 target 各编一次，合法)")

    return problems, exemptions, notes


def disk_exists(rel: str) -> bool:
    return (ROOT / "backend" / rel).is_file()


def self_test() -> int:
    """变异自检：注入的缺陷必须被抓到；抓不到就是校验器失效。"""
    ok = True
    empty = ({}, {}, set(), {}, {}, {}, {})
    base_core = "core/pace/frame_pacer.cxx"

    # M1：core 文件只加到 Android 清单
    problems, _, _ = evaluate({base_core: 1}, {}, set(), {}, {}, {}, {}, lambda _: True)
    caught = any("only-android" in p for p in problems)
    print(f"  M1 core 单侧差集    : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M2：磁盘有 core 文件、两份清单都没注册
    problems, _, _ = evaluate({}, {}, {base_core}, {}, {}, {}, {}, lambda _: True)
    caught = sum("unregistered" in p for p in problems) == 2
    print(f"  M2 core 漏注册      : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M3：磁盘有 Android 后端文件、Android 清单没注册
    problems, _, _ = evaluate(
        {}, {}, {"backends/mediacodec/session/x.cxx"}, {}, {}, {}, {}, lambda _: True
    )
    caught = any("unregistered  android/" in p for p in problems)
    print(f"  M3 后端漏注册       : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M4：清单指向不存在的文件
    problems, _, _ = evaluate(
        {"core/pace/__mutation__.cxx": 1},
        {"core/pace/__mutation__.cxx": 1},
        set(), {}, {}, {}, {},
        lambda rel: not rel.endswith("__mutation__.cxx"),
    )
    caught = any("missing-file" in p for p in problems)
    print(f"  M4 假路径           : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M5：显式豁免能放行，过期豁免会被抓到
    problems, exemptions, _ = evaluate(
        {base_core: 1}, {}, set(), {base_core: "测试豁免"}, {}, {}, {}, lambda _: True
    )
    caught = not problems and len(exemptions) == 1
    print(f"  M5 显式豁免放行     : {'PASS' if caught else 'FAIL'}")
    ok &= caught
    problems, _, _ = evaluate(
        {base_core: 1}, {base_core: 1}, set(), {"core/pace/stale.cxx": "过期"}, {}, {}, {},
        lambda _: True,
    )
    caught = any("stale-exempt" in p for p in problems)
    print(f"  M5b 过期豁免        : {'CAUGHT' if caught else 'MISSED'}")
    ok &= caught

    # M6：漏注册豁免确实能放行（且理由被打印）
    problems, exemptions, _ = evaluate(
        {}, {}, {base_core}, {}, {}, {"backend/" + base_core: "桌面不编"}, {}, lambda _: True
    )
    caught = not any("backend/" + base_core in p for p in problems) and len(exemptions) == 1
    print(f"  M6 漏注册豁免放行   : {'PASS' if caught else 'FAIL'}")
    ok &= caught

    # M7：重复注册只提示、不判错
    problems, _, notes = evaluate(
        {base_core: 2}, {base_core: 1}, set(), {}, {}, {}, {}, lambda _: True
    )
    caught = not problems and any("duplicate" in n for n in notes)
    print(f"  M7 重复注册只提示   : {'PASS' if caught else 'FAIL'}")
    ok &= caught

    return 0 if ok else 2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="跑变异自检后退出")
    args = parser.parse_args()

    if args.self_test:
        print("=== verify_source_manifest 变异自检 ===")
        return self_test()

    for path in (ANDROID_CMAKE, BACKEND_CMAKE):
        if not path.is_file():
            print(f"FATAL: 清单不存在: {path}", file=sys.stderr)
            return 2

    android = collect(ANDROID_CMAKE.read_text(encoding="utf-8"), ANDROID_PATTERN)
    backend = collect(BACKEND_CMAKE.read_text(encoding="utf-8"), BACKEND_PATTERN)
    disk = scan_disk()
    problems, exemptions, notes = evaluate(
        android, backend, disk,
        EXEMPT_ONLY_ANDROID, EXEMPT_ONLY_BACKEND,
        EXEMPT_CORE_UNREGISTERED, EXEMPT_ANDROID_UNREGISTERED,
        disk_exists,
    )

    print("=== 源文件清单注册完整性 ===")
    print(f"  磁盘 core/           : {sum(is_shared(r) for r in disk):3d} 个源文件")
    print(f"  磁盘 Android 后端    : {sum(not is_shared(r) for r in disk):3d} 个源文件")
    print(f"  android 清单 (范围内): {len(android):3d} 条")
    print(f"  backend 清单 (范围内): {len(backend):3d} 条")
    if exemptions:
        print(f"--- 已声明豁免 {len(exemptions)} 条（逐条打印，不隐藏） ---")
        for line in exemptions:
            print(f"  {line}")
    else:
        print("--- 已声明豁免 0 条 ---")
    if notes:
        print(f"--- 提示 {len(notes)} 条（不判错） ---")
        for line in notes:
            print(f"  {line}")

    if problems:
        print(f"--- FAIL: {len(problems)} 项 ---")
        for line in problems:
            print(f"  {line}")
        return 1

    print("PASS: core 双清单一致、磁盘源文件全部注册、所有路径指向真实文件")
    return 0


if __name__ == "__main__":
    sys.exit(main())
