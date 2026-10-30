#!/usr/bin/env python3
"""P12-D3 长稳测试的主机侧内存采样。

在后台跑，定期 `dumpsys meminfo` 取 PSS，写成 CSV。为什么要主机侧采而不是
应用内采：应用内采内存只能拿到自己的视角（`mallinfo` / `Runtime`），
而 PSS 是系统算的、包含共享页的摊分，**同一个数**在泄漏排查里才有意义。

用法：

    python tools/sample_p12d3_mem.py \
        --adb "C:/Android/sdk/platform-tools/adb.exe" \
        --pkg com.kivo.cinema --minutes 31 --interval 20 \
        --out build/avtmp/soak-mem.csv

输出列：

    epoch_ms,elapsed_s,pss_total_kb,native_heap_kb,dalvik_heap_kb

`pss_total_kb` 是判据用的那个数（"内存增长 < 5MB"）；另外两列用于归因
（增长在 native 还是在 Java 堆，结论完全不同）。
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import time

TOTAL_ROW = re.compile(r"^\s*TOTAL\s+(\d+)\s+(\d+)", re.MULTILINE)
TOTAL_PSS_VARIANT = re.compile(r"TOTAL PSS:\s*(\d+)")
ROW_CACHE = {
    "native": re.compile(r"^\s*Native Heap\s+(\d+)", re.MULTILINE),
    "dalvik": re.compile(r"^\s*Dalvik Heap\s+(\d+)", re.MULTILINE),
}


def read_meminfo(adb: str, pkg: str) -> tuple[int, int, int] | None:
    try:
        raw = subprocess.run(
            [adb, "shell", f"dumpsys meminfo {pkg}"],
            capture_output=True, text=True, timeout=30,
        ).stdout
    except (subprocess.SubprocessError, OSError):
        return None

    match = TOTAL_ROW.search(raw)
    if match:
        total = int(match.group(1))
    else:
        variant = TOTAL_PSS_VARIANT.search(raw)
        if not variant:
            return None
        total = int(variant.group(1))

    native = ROW_CACHE["native"].search(raw)
    dalvik = ROW_CACHE["dalvik"].search(raw)
    return (
        total,
        int(native.group(1)) if native else -1,
        int(dalvik.group(1)) if dalvik else -1,
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--adb", default="adb")
    parser.add_argument("--pkg", default="com.kivo.cinema")
    parser.add_argument("--minutes", type=float, default=31.0)
    parser.add_argument("--interval", type=float, default=20.0)
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)

    deadline = time.time() + args.minutes * 60.0
    started = time.time()
    samples = 0

    with open(args.out, "w", encoding="utf-8", newline="") as handle:
        handle.write("epoch_ms,elapsed_s,pss_total_kb,native_heap_kb,dalvik_heap_kb\n")
        handle.flush()
        while time.time() < deadline:
            reading = read_meminfo(args.adb, args.pkg)
            now_ms = int(time.time() * 1000)
            elapsed = int(time.time() - started)
            if reading is None:
                # 进程不在（还没起播或已退出）不写行：写 0 会把趋势算歪。
                print(f"[{elapsed:5d}s] no meminfo (app not running?)", flush=True)
            else:
                handle.write(f"{now_ms},{elapsed},{reading[0]},{reading[1]},{reading[2]}\n")
                handle.flush()
                samples += 1
                if samples % 5 == 0:
                    print(f"[{elapsed:5d}s] pss={reading[0]}KB native={reading[1]}KB "
                          f"dalvik={reading[2]}KB", flush=True)
            time.sleep(args.interval)

    print(f"done: {samples} samples -> {args.out}", flush=True)
    return 0 if samples > 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
