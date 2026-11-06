#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B27：验证 `AAudioStream_requestPause` 之后 timestamp 到底是冻结还是失效。

单一职责：编译 + 设备运行 + 输出四个位置读数。不改产品路径（`pause()` 只被探针调用）。

为什么必须实测：`stop()` 用的是 `requestStop`，注释明确写"帧位置会归零"；
而 `requestPause` 理论上保留位置 —— 但本项目**从未用过**。
pause-resume 走哪条路，取决于这个实验。

设计口径：docs/design/backend-core/204-p16-p1b27-pause-resume-design.md

退出码：0 = 探针跑完；2 = 编译 / 设备不可用。
"""

from __future__ import annotations

import argparse
import glob
import importlib.util
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build" / "p16-pause-probe"
REMOTE = "/data/local/tmp/kivo_pause_probe"
# 多机型矩阵：主测试机 b6e950ee 归他人使用，其余任意设备都可测。
FORBIDDEN_DEVICE = "b6e950ee"
DEFAULT_DEVICE = "dd137cf7"

INCLUDE_DIRS = (ROOT / "backend" / "include_private", ROOT / "backend" / "include")
PLANE = ROOT / "backend" / "src" / "video" / "video_plane"
SOURCES = (
    PLANE / "backends/aaudio/session/aaudio_output_session.cxx",
    PLANE / "backends/aaudio/device/aaudio_stream_opener.cxx",
    PLANE / "backends/aaudio/device/aaudio_format_names.cxx",
    PLANE / "backends/aaudio/clock/aaudio_clock_source.cxx",
    PLANE / "backends/aaudio/clock/aaudio_clock_tail.cxx",
    PLANE / "backends/aaudio/clock/aaudio_clock_trace.cxx",
    PLANE / "backends/aaudio/clock/aaudio_monotonic_now.cxx",
    PLANE / "backends/aaudio/clock/aaudio_timestamp_reading.cxx",
    PLANE / "backends/aaudio/clock/aaudio_timestamp_reader.cxx",
    PLANE / "core/audio/clock/frame_position_extrapolator.cxx",
    PLANE / "core/audio/clock/monotonic_clock_guard.cxx",
    PLANE / "core/audio/clock/audio_clock_types.cxx",
)

CPP_SOURCE = r"""
#include <chrono>
#include <cstdio>
#include <thread>

#include "video/video_plane/backends/aaudio/session/aaudio_output_session.hpp"

namespace {
class SilenceSource final : public kivo::video::aaudio::AaudioOutputSession::SampleSource {
public:
    std::int32_t render(float* out, std::int32_t frames) override {
        for (std::int32_t i = 0; i < frames * 2; ++i) {
            out[i] = 0.0f;
        }
        return frames;
    }
};

void nap_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
}  // namespace

int main() {
    using kivo::video::aaudio::AaudioOutputSession;
    using kivo::video::aaudio::AaudioStreamConfig;

    AaudioOutputSession session;
    SilenceSource source;
    AaudioStreamConfig config;
    config.sample_rate = 48000;
    config.channel_count = 2;
    config.format = AaudioStreamConfig::SampleFormat::kFloat;
    config.prefer_exclusive = false;   // 独占可能被拒；本实验只关心 pause 语义
    config.prefer_low_latency = true;

    const auto opened = session.open(config, source);
    if (!opened.ok()) {
        std::printf("OPEN_FAILED: %s\n", opened.message.c_str());
        return 1;
    }
    const auto started = session.start();
    if (!started.ok()) {
        std::printf("START_FAILED: %s\n", started.message.c_str());
        return 1;
    }
    auto* clock = session.clock();
    if (clock == nullptr) {
        std::printf("NO_CLOCK\n");
        return 1;
    }

    nap_ms(1000);
    const std::int64_t p1 = clock->now().position_ns;

    const bool paused = session.pause();
    nap_ms(100);
    const std::int64_t p2 = clock->now().position_ns;

    nap_ms(2000);
    const std::int64_t p3 = clock->now().position_ns;

    const bool resumed = session.resume();
    nap_ms(500);
    const std::int64_t p4 = clock->now().position_ns;

    std::printf("pause_ok=%d resume_ok=%d\n", paused ? 1 : 0, resumed ? 1 : 0);
    std::printf("p1_before_pause_ns=%lld\n", static_cast<long long>(p1));
    std::printf("p2_after_pause_ns=%lld\n", static_cast<long long>(p2));
    std::printf("p3_after_2s_pause_ns=%lld\n", static_cast<long long>(p3));
    std::printf("p4_after_resume_ns=%lld\n", static_cast<long long>(p4));
    std::printf("RESULT: DONE\n");
    session.stop();
    return 0;
}
"""


def sdk_helpers():
    path = ROOT / "tools" / "run_p16_burst_behavior_test.py"
    spec = importlib.util.spec_from_file_location("kivo_burst_behavior", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(cmd, *, shell: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, shell=shell, capture_output=True, text=True,
                          encoding="utf-8", errors="replace")


def compile_probe() -> Path | None:
    helpers = sdk_helpers()
    compiler = helpers.resolve_compiler()
    if compiler is None:
        print("FATAL: 找不到 NDK wrapper", file=sys.stderr)
        return None
    BUILD.mkdir(parents=True, exist_ok=True)
    test_cpp = BUILD / "pause_probe.cpp"
    test_cpp.write_text(CPP_SOURCE, encoding="utf-8")
    binary = BUILD / "pause_probe"
    includes = " ".join(f'-I "{p}"' for p in INCLUDE_DIRS)
    sources = " ".join(f'"{p}"' for p in (*SOURCES, test_cpp))
    command = (f'"{compiler}" -std=c++23 -O0 -Wall -Wextra {includes} {sources} '
               f'-laaudio -landroid -llog -o "{binary}"')
    result = run(command, shell=True)
    if result.returncode != 0:
        print("--- compile FAILED ---")
        print((result.stdout + result.stderr)[-3000:])
        return None
    print(f"  compile: OK ({len(SOURCES) + 1} TUs)")
    return binary


def device_probe(binary: Path, device: str) -> int:
    helpers = sdk_helpers()
    adb = helpers.resolve_adb()
    if adb is None:
        print("FATAL: 找不到 adb", file=sys.stderr)
        return 2
    if run([str(adb), "-s", device, "push", str(binary), REMOTE]).returncode != 0:
        print("FATAL: push 失败", file=sys.stderr)
        return 2
    run([str(adb), "-s", device, "shell", f"chmod 755 {REMOTE}"])
    executed = run([str(adb), "-s", device, "shell", REMOTE])
    print(executed.stdout.rstrip())
    if executed.stderr.strip():
        print("  stderr:", executed.stderr.strip()[:300])
    run([str(adb), "-s", device, "shell", f"rm -f {REMOTE}"])
    return 0 if "RESULT: DONE" in executed.stdout else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", default=DEFAULT_DEVICE)
    parser.add_argument("--compile-only", action="store_true")
    args = parser.parse_args()
    if args.device == FORBIDDEN_DEVICE:
        print(f"FATAL: {FORBIDDEN_DEVICE} 是主测试机（他人使用中），拒绝操作", file=sys.stderr)
        return 2
    print("=== P16-P1B27 pause 探针 ===")
    binary = compile_probe()
    if binary is None:
        return 2
    if args.compile_only:
        print("  --compile-only：跳过设备")
        return 0
    print(f"  device: {args.device} (b6e950ee 全程不碰)")
    return device_probe(binary, args.device)


if __name__ == "__main__":
    sys.exit(main())
