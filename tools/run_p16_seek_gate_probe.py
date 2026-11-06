#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P16-P1B6：量化「set_muted(true) 到在途回调归零」到底要多久。

单一职责：编译 + 设备运行 + 输出 p50/p99/max，与 seek 栅栏的 10 ms 对照。
不改产品代码、不产证据 JSON、不注册 ctest（backend/CMakeLists.txt 被他人主线占用）、
不进 CI（需真机）。C++ 源内嵌，产物落既有 build/p16-seek-gate/。

设计口径：docs/design/backend-core/189-p16-p1b6-seek-gate-observation-plan.md

结论边界：测的是本进程 render() 临界区，不是 AAudio 平台回调的调度延迟。
后者这台设备造不出来，所以「p99 << 10ms」不能证明 10ms 覆盖平台抖动。

退出码：0 = O1/O2/O3 全 PASS；1 = 断言失败；2 = 编译 / 设备不可用。
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build" / "p16-seek-gate"
REMOTE = "/data/local/tmp/kivo_seek_gate_probe"
# 多机型矩阵：主测试机 b6e950ee 归他人使用，其余任意设备都可测。
FORBIDDEN_DEVICE = "b6e950ee"
DEFAULT_DEVICE = "dd137cf7"

HEADER = (ROOT / "backend" / "include_private" / "video" / "video_plane" /
          "backends" / "aaudio" / "buffer" / "ring_buffer_sample_source.hpp")
INCLUDE_DIRS = (ROOT / "backend" / "include_private", ROOT / "backend" / "include")
PLANE = ROOT / "backend" / "src" / "video" / "video_plane"
SOURCES = (PLANE / "core" / "audio" / "spsc_audio_ring_buffer.cxx",)

CPP_SOURCE = r"""
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#include "video/video_plane/backends/aaudio/buffer/ring_buffer_sample_source.hpp"
#include "video/video_plane/core/audio/spsc_audio_ring_buffer.hpp"

int main() {
    using Clock = std::chrono::steady_clock;
    kivo::video::core::audio::SpscAudioRingBuffer ring(2, 16384);
    kivo::video::aaudio::RingBufferSampleSource source(ring);

    std::vector<float> fill(192 * 2, 0.25f);
    std::vector<float> sink(192 * 2, 0.0f);
    (void)ring.write(fill.data(), 192);

    std::atomic<bool> stop{false};
    // 高频压力档：worker 同时充当生产者与消费者（SPSC 契约仍成立，无并发写）。
    std::thread worker([&] {
        while (!stop.load(std::memory_order_relaxed)) {
            (void)source.render(sink.data(), 192);
            (void)ring.write(fill.data(), 192);
        }
    });

    constexpr int kRounds = 500;
    std::vector<std::int64_t> samples;
    samples.reserve(kRounds);
    for (int i = 0; i < kRounds; ++i) {
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        const auto t0 = Clock::now();
        source.set_muted(true);
        int spins = 0;
        while (source.in_flight() != 0 && spins < 2000000) { ++spins; }
        if (spins >= 2000000) { std::printf("  WARN: spin ceiling hit at round %d\n", i); }
        const auto t1 = Clock::now();
        samples.push_back(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
        source.set_muted(false);
    }
    stop.store(true, std::memory_order_relaxed);
    worker.join();

    const bool o2 = source.in_flight() == 0;
    std::sort(samples.begin(), samples.end());
    const std::int64_t p50 = samples[kRounds / 2];
    const std::int64_t p99 = samples[(kRounds * 99) / 100];
    const std::int64_t mx = samples.back();

    std::printf("  O2 in_flight() == 0 after join : %s\n", o2 ? "PASS" : "FAIL");
    std::printf("  rounds=%d  p50=%lld us  p99=%lld us  max=%lld us  budget=10000 us\n",
                kRounds, static_cast<long long>(p50), static_cast<long long>(p99),
                static_cast<long long>(mx));
    const bool o3 = p99 < 10000;
    std::printf("  O3 p99 < 10ms                 : %s\n", o3 ? "PASS" : "FAIL");
    std::printf("RESULT: %s\n", (o2 && o3) ? "PASS" : "FAIL");
    return (o2 && o3) ? 0 : 1;
}
"""


def sdk_helpers():
    """复用 187 的 SDK 三级解析，不重复实现。"""
    path = ROOT / "tools" / "run_p16_burst_behavior_test.py"
    spec = importlib.util.spec_from_file_location("kivo_burst_behavior", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(cmd, *, shell: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, shell=shell, capture_output=True, text=True,
                          encoding="utf-8", errors="replace")


def static_check() -> bool:
    """O1：计数递增必须早于判空，且两个出口都递减。

    只看 render() 函数体 —— 全文 find("muted_.load") 会先命中注释里
    「若放在 muted_.load() 之后」那句，把判据自己骗了（实现期更正，保留）。
    """
    text = HEADER.read_text(encoding="utf-8")
    start = text.find("std::int32_t render")
    end = text.find("/// 只读观测", start)
    body = text[start:end] if end > start >= 0 else ""
    add = body.find("in_flight_.fetch_add")
    muted = body.find("muted_.load")
    ok = (bool(body) and 0 <= add < muted
          and body.count("in_flight_.fetch_sub") == 2
          and "in_flight()" in text)
    print(f"  O1 counter before muted check / 2 exits / getter : {'PASS' if ok else 'FAIL'}")
    return ok


def compile_probe() -> Path | None:
    helpers = sdk_helpers()
    compiler = helpers.resolve_compiler()
    if compiler is None:
        sdk, source = helpers.resolve_sdk_root()
        print(f"FATAL: 找不到 NDK wrapper (sdk={sdk}, source={source})", file=sys.stderr)
        return None
    BUILD.mkdir(parents=True, exist_ok=True)
    test_cpp = BUILD / "seek_gate_probe.cpp"
    test_cpp.write_text(CPP_SOURCE, encoding="utf-8")
    binary = BUILD / "seek_gate_probe"
    includes = " ".join(f'-I "{p}"' for p in INCLUDE_DIRS)
    sources = " ".join(f'"{p}"' for p in (*SOURCES, test_cpp))
    result = run(f'"{compiler}" -std=c++23 -O0 -Wall -Wextra {includes} {sources} '
                 f'-o "{binary}"', shell=True)
    if result.returncode != 0:
        print("--- compile FAILED ---")
        print((result.stdout + result.stderr)[-3000:])
        return None
    print(f"  compile: OK ({len(SOURCES) + 1} TUs -> {binary.name})")
    return binary


def device_probe(binary: Path, device: str) -> int:
    helpers = sdk_helpers()
    adb = helpers.resolve_adb()
    if adb is None:
        print("FATAL: 找不到 adb", file=sys.stderr)
        return 2
    online = run([str(adb), "devices"]).stdout
    if f"{device}\tdevice" not in online:
        print(f"FATAL: {device} 不在线", file=sys.stderr)
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
    return 0 if "RESULT: PASS" in executed.stdout else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", default=DEFAULT_DEVICE,
                        help=f"除 {FORBIDDEN_DEVICE}（主测试机）外的任意设备")
    parser.add_argument("--compile-only", action="store_true", help="只编译，不碰设备")
    parser.add_argument("--self-check", action="store_true",
                        help="只做 O1 静态形状检查")
    args = parser.parse_args()

    print("=== P16-P1B6 seek 栅栏在途窗口测量 ===")
    if args.self_check:
        return 0 if static_check() else 1
    if args.device == FORBIDDEN_DEVICE:
        print(f"FATAL: {FORBIDDEN_DEVICE} 是主测试机（他人使用中），拒绝操作", file=sys.stderr)
        return 2
    if not static_check():
        return 1
    binary = compile_probe()
    if binary is None:
        return 2
    if args.compile_only:
        print("  --compile-only：跳过设备运行")
        return 0
    print(f"  device: {args.device} (b6e950ee 全程不碰)")
    return device_probe(binary, args.device)


if __name__ == "__main__":
    sys.exit(main())
