#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B3：跑一次真实的 AaudioClockSource 构造，证明「兜底可查询」不是纸面承诺。

单一职责：编译 + 设备运行 + 断言 T1/T2/T3。不注册 ctest（backend/CMakeLists.txt 被他人主线占用）、
不改任何仓库源文件、不产出证据 JSON、不在仓库里新建目录（测试源内嵌、产物落既有 build/）。

为什么内嵌 C++：家族树 §一 第 2 条「子域文件数 >= 3 才开子目录」，单个测试 TU 不该新开目录；
塞进别人的 p12c4_clock_tests/ 又会把 P16 语义混进 P12 卡。

设计口径：docs/design/backend-core/186-p16-p1b3-behavior-evidence-plan.md

退出码：0 = T1/T2/T3 全 PASS；1 = 行为断言失败；2 = 编译 / 设备不可用。
"""

from __future__ import annotations

import argparse
import glob
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build" / "p16-burst-behavior"
REMOTE = "/data/local/tmp/kivo_burst_behavior"


# P16-P1B4：SDK 路径绝不写死。AGENTS.md §0.3 已经记过一次「换机器后代码是全的、
# 构建却起不来」的教训；把 C:/Android/sdk 和 windows-x86_64 写死等于再加第四样绑定。
def resolve_sdk_root() -> tuple[Path, str]:
    """三级优先：环境变量 → android/local.properties → PATH 里 adb 上溯两级。

    返回 (sdk_root, 来源)；来源会打印出来，换机器排错时一眼能看出用的是哪个。
    """
    for var in ("ANDROID_HOME", "ANDROID_SDK_ROOT"):
        value = os.environ.get(var, "").strip()
        if value and Path(value).is_dir():
            return Path(value), f"env:{var}"

    props = ROOT / "android" / "local.properties"
    if props.is_file():
        for line in props.read_text(encoding="utf-8", errors="ignore").splitlines():
            line = line.strip()
            if line.startswith("sdk.dir="):
                value = line.split("=", 1)[1].strip().replace("\\:", ":").replace("\\", "/")
                if Path(value).is_dir():
                    return Path(value), "android/local.properties"

    found = shutil.which("adb")
    if found:
        adb = Path(found).resolve()
        if adb.parent.name == "platform-tools":
            return adb.parent.parent, "PATH:adb"
    return Path(""), "unresolved"


def resolve_adb() -> Path | None:
    sdk, _ = resolve_sdk_root()
    if sdk == Path(""):
        found = shutil.which("adb")
        return Path(found) if found else None
    name = "adb.exe" if os.name == "nt" else "adb"
    candidate = sdk / "platform-tools" / name
    return candidate if candidate.is_file() else None


def resolve_compiler() -> Path | None:
    """prebuilt/* 覆盖 windows-x86_64 / linux-x86_64 / darwin-x86_64。"""
    sdk, _ = resolve_sdk_root()
    if sdk == Path(""):
        return None
    pattern = str(sdk / "ndk" / "*" / "toolchains" / "llvm" / "prebuilt" / "*" /
                  "bin" / "aarch64-linux-android26-clang++*")
    candidates = sorted(glob.glob(pattern))
    if os.name == "nt":
        wrapped = [p for p in candidates if p.endswith(".cmd")]
        if wrapped:
            return Path(wrapped[-1])
    bare = [p for p in candidates if not p.endswith(".cmd")]
    return Path(bare[-1]) if bare else None

# 硬白名单：本卡只允许操作第二台设备，绝不碰主测试机 b6e950ee。
FORBIDDEN_DEVICE = "b6e950ee"
DEFAULT_DEVICE = "dd137cf7"
# 主测试机默认拦住（别台机器可能正被他人取证）。设备所有者明确授权时，
# 设 `KIVO_ALLOW_PRIMARY=1` 才放行 —— 默认拦、显式开，两边都不靠记忆。
ALLOW_PRIMARY_ENV = "KIVO_ALLOW_PRIMARY"

INCLUDE_DIRS = (
    ROOT / "backend" / "include_private",
    ROOT / "backend" / "include",
)
PLANE = ROOT / "backend" / "src" / "video" / "video_plane"
SOURCES = (
    PLANE / "backends/aaudio/clock/aaudio_clock_source.cxx",
    PLANE / "backends/aaudio/clock/aaudio_clock_tail.cxx",
    PLANE / "backends/aaudio/clock/aaudio_clock_trace.cxx",
    PLANE / "backends/aaudio/clock/aaudio_monotonic_now.cxx",
    PLANE / "backends/aaudio/clock/aaudio_timestamp_reading.cxx",
    PLANE / "core/audio/clock/frame_position_extrapolator.cxx",
    PLANE / "core/audio/clock/monotonic_clock_guard.cxx",
    PLANE / "core/audio/clock/audio_clock_types.cxx",
    # `FramePacer` 拉进来的依赖太多（histogram / drift / worst episode …），
    # 与其一个个补符号，不如把 `core/pace/` 整个编进来 —— 都是纯逻辑、零平台头。
    *sorted((PLANE / "core" / "pace").glob("*.cxx")),
)

CPP_SOURCE_PATH = ROOT / "tools" / "p16_behavior_probe.cpp"


def run(cmd: list[str] | str, *, shell: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, shell=shell, capture_output=True, text=True, encoding="utf-8",
                          errors="replace")


def compile_test() -> Path | None:
    compiler = resolve_compiler()
    if compiler is None:
        sdk, source = resolve_sdk_root()
        print(f"FATAL: 找不到 NDK 的 aarch64 clang++ wrapper "
              f"(sdk={sdk}, source={source})", file=sys.stderr)
        return None
    BUILD.mkdir(parents=True, exist_ok=True)
    test_cpp = CPP_SOURCE_PATH
    binary = BUILD / "burst_fallback_behavior"

    includes = " ".join(f'-I "{p}"' for p in INCLUDE_DIRS)
    sources = " ".join(f'"{p}"' for p in (*SOURCES, test_cpp))
    command = (f'"{compiler}" -std=c++23 -O0 -Wall -Wextra {includes} '
               f'{sources} -o "{binary}"')
    result = run(command, shell=True)
    if result.returncode != 0:
        print("--- compile FAILED ---")
        print((result.stdout + result.stderr)[-3000:])
        return None
    print(f"  compile: OK ({len(SOURCES) + 1} TUs -> {binary.name})")
    return binary


def device_test(binary: Path, device: str) -> int:
    adb = resolve_adb()
    if adb is None:
        sdk, source = resolve_sdk_root()
        print(f"FATAL: 找不到 adb (sdk={sdk}, source={source})", file=sys.stderr)
        return 2
    online = run([str(adb), "devices"]).stdout
    if device not in online or f"{device}\tdevice" not in online:
        print(f"FATAL: {device} 不在线（或状态不对）", file=sys.stderr)
        return 2

    pushed = run([str(adb), "-s", device, "push", str(binary), REMOTE])
    if pushed.returncode != 0:
        print(f"FATAL: push 失败: {pushed.stderr.strip()}", file=sys.stderr)
        return 2

    chmod = run([str(adb), "-s", device, "shell", f"chmod 755 {REMOTE}"])
    if chmod.returncode != 0:
        print(f"FATAL: chmod 失败: {chmod.stderr.strip()}", file=sys.stderr)
        return 2

    executed = run([str(adb), "-s", device, "shell", REMOTE])
    print(executed.stdout.rstrip())
    if executed.stderr.strip():
        print("  stderr:", executed.stderr.strip()[:300])
    run([str(adb), "-s", device, "shell", f"rm -f {REMOTE}"])

    if "RESULT: PASS" not in executed.stdout:
        print("FAIL: 行为断言未全过")
        return 1
    return 0


def print_self_check() -> int:
    """只解析工具链路径并打印来源：换机器交接时的第一条自检命令。"""
    sdk, source = resolve_sdk_root()
    adb = resolve_adb()
    compiler = resolve_compiler()
    print(f"  sdk_root : {sdk if sdk != Path('') else '(unresolved)'}")
    print(f"  source   : {source}")
    print(f"  adb      : {adb or '(missing)'}")
    print(f"  compiler : {compiler or '(missing)'}")
    ok = source != "unresolved" and adb is not None and compiler is not None
    print(f"  RESULT   : {'PASS' if ok else 'FAIL'}")
    return 0 if ok else 2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", default=DEFAULT_DEVICE,
                        help=f"除 {FORBIDDEN_DEVICE}（主测试机）外的任意设备")
    parser.add_argument("--compile-only", action="store_true", help="只编译，不碰设备")
    parser.add_argument("--self-check", action="store_true",
                        help="只解析工具链路径并打印来源，不编译、不碰设备")
    args = parser.parse_args()

    if args.self_check:
        print("=== P16 工具链自检（不碰设备） ===")
        return print_self_check()

    if args.device == FORBIDDEN_DEVICE and os.environ.get(ALLOW_PRIMARY_ENV) != "1":
        print(f"FATAL: {FORBIDDEN_DEVICE} 是主测试机（他人使用中），拒绝操作；"
              f"若设备所有者已明确授权，设 {ALLOW_PRIMARY_ENV}=1", file=sys.stderr)
        return 2

    print("=== P16-P1B3 兜底行为测试 ===")
    binary = compile_test()
    if binary is None:
        return 2
    if args.compile_only:
        print("  --compile-only：跳过设备运行")
        return 0
    print(f"  device: {args.device} (b6e950ee 全程不碰)")
    return device_test(binary, args.device)


if __name__ == "__main__":
    sys.exit(main())
