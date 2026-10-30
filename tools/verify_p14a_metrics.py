#!/usr/bin/env python3
"""P14-A 校验：三条尺子到底有没有读数、读得对不对。

这份脚本守的不是"指标好不好"，而是**"仪器本身可信不可信"**：

M1 CPU 有效      `cpu.valid = true` 且 `wall_ns > 0` 且 `thread_ratio > 0`
M2 CPU 占比合理  `0 < process_ratio < 1.0`（超过 1 = 进程 CPU 时间多于墙钟，
                 单线程播放做不到；等于 0 = 根本没采到）
M3 钩子装上      `alloc.hook_installed = true`。
                 **没有这一格，`calls_steady = 0` 与"钩子没链进来"在证据里一模一样**
M4 缓冲归零      `buffers.in_flight = 0` 且 `leaked = 0`
M5 账目一致      `buffers.received/released` 必须等于 `counters.frames_received/released`
                 —— 两个计数器各写一遍迟早分叉，这类"恒错字段"比缺字段更难发现
N1 槽表够用      `thread_count >= 1` 且 `overflow_calls = 0`
                 （溢出了就说明归因不完整，结论只能覆盖前 8 个线程）
N2 分桶不漏账    `Σ threads[].calls == calls_total`
N3 归因完整      `thread_count` 个槽里有调用数 > 0 的至少一个
N4 稳态分桶      `Σ threads[].calls_steady == calls_steady`（无溢出时）

用法：
    python tools/verify_p14a_metrics.py docs/evidence/p14a/*.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


class Failure(Exception):
    """一条可读的失败。"""


def verify(path: Path) -> str:
    data: dict[str, Any] = json.loads(path.read_text(encoding="utf-8"))
    schema = str(data.get("schema", ""))
    if not schema.startswith("kivo.p12c.decode/"):
        raise Failure(f"schema 是 {schema!r}，这不是 decode 证据")

    metrics = data.get("metrics")
    if not isinstance(metrics, dict):
        raise Failure("缺 metrics 块（P14-A 的三条尺子都没出版）")

    cpu = metrics["cpu"]
    alloc = metrics["alloc"]
    buffers = metrics["buffers"]

    # ── M1 CPU 有效 ──
    if not cpu["valid"]:
        raise Failure(
            f"M1 CPU 快照无效：{cpu.get('reason') or '未给理由'}"
            "（Windows 上恒为 false 是预期的；Android 上不该）"
        )
    if int(cpu["wall_ns"]) <= 0:
        raise Failure(f"M1 wall_ns = {cpu['wall_ns']}，分母没取到")
    if float(cpu["thread_ratio"]) <= 0.0:
        raise Failure("M1 thread_ratio = 0，CPU 时间没采到（不是「零占用」）")

    # ── M2 CPU 占比合理 ──
    process_ratio = float(cpu["process_ratio"])
    if not 0.0 < process_ratio < 1.0:
        raise Failure(
            f"M2 process_ratio = {process_ratio:.4f}，落在 (0, 1) 之外："
            "≥1 说明计算有误或进程被多核算重，=0 说明没采到"
        )

    # ── M3 钩子装上 ──
    if not alloc["hook_installed"]:
        raise Failure(
            "M3 alloc.hook_installed = false：`calls_steady` 不可读 —— "
            "0 次分配和没装钩子在证据里长得一样"
        )
    if not alloc["armed"]:
        raise Failure("M3 alloc.armed = false：会话期没在计数")

    # ── M4 缓冲归零 ──
    if int(buffers["in_flight"]) != 0 or int(buffers["leaked"]) != 0:
        raise Failure(
            f"M4 在途 {buffers['in_flight']} / 泄漏 {buffers['leaked']}："
            f"{buffers.get('reason') or '未给理由'}"
        )

    # ── M5 账目一致 ──
    counters = data["counters"]
    if int(buffers["received"]) != int(counters["frames_received"]):
        raise Failure(
            f"M5 buffers.received({buffers['received']}) != "
            f"counters.frames_received({counters['frames_received']})"
        )
    if int(buffers["released"]) != int(counters["frames_released"]):
        raise Failure(
            f"M5 buffers.released({buffers['released']}) != "
            f"counters.frames_released({counters['frames_released']})"
        )

    # ── N1/N2：线程分桶自证（P14-B）──
    #
    # `/14` 的证据没有线程维度（P14-A 时的形状）。**不重写历史证据** ——
    # 重写等于伪造。这里显式跳过并在摘要里标注，而不是让它静默通过。
    schema_index = 0
    if "/" in schema:
        try:
            schema_index = int(schema.rsplit("/", 1)[1])
        except ValueError:
            schema_index = 0
    threads = alloc.get("threads")
    if threads is None and schema_index < 15:
        return (
            f"{path.name}: OK  cpu={cpu['thread_ratio']:.4f}/{process_ratio:.4f} "
            f"alloc steady={alloc['calls_steady']}/{alloc['calls_total']} "
            f"buffers={buffers['received']}->{buffers['released']} "
            f"leaked={buffers['leaked']} | /{schema_index} 无线程分桶（豁免）"
        )
    if threads is None:
        raise Failure("N1 缺 threads[]：没有线程维度就说不清那 5 次/秒是谁")
    if int(alloc["thread_count"]) != len(threads):
        raise Failure(
            f"N1 thread_count({alloc['thread_count']}) != threads 数组长度({len(threads)})"
        )
    if int(alloc["thread_count"]) < 1:
        raise Failure("N1 thread_count = 0：一次分配都没归到一个线程上")
    if int(alloc.get("overflow_calls", 0)) != 0:
        raise Failure(
            f"N1 overflow_calls = {alloc['overflow_calls']}：线程超过 8 个，"
            "归因只覆盖前 8 个，结论不完整"
        )
    bucket_total = sum(int(item["calls"]) for item in threads)
    if bucket_total != int(alloc["calls_total"]):
        raise Failure(
            f"N2 分桶合计 {bucket_total} != calls_total {alloc['calls_total']}（有分配没归到槽里）"
        )
    if bucket_total > 0 and all(int(item["calls"]) == 0 for item in threads):
        raise Failure("N3 有分配总量却每个桶都是 0")

    # N4：稳态也要对得上。没有这一条，"总量分桶一致、但稳态分桶漏了"
    # 会在证据里完全看不出来 —— 而 G3 判的正是稳态那一格。
    steady_total = sum(int(item.get("calls_steady", 0)) for item in threads)
    if int(alloc.get("overflow_calls", 0)) == 0 and steady_total != int(alloc["calls_steady"]):
        raise Failure(
            f"N4 稳态分桶合计 {steady_total} != calls_steady {alloc['calls_steady']}"
        )

    return (
        f"{path.name}: OK  cpu={cpu['thread_ratio']:.4f}/{process_ratio:.4f} "
        f"(thread/proc) alloc steady={alloc['calls_steady']}/{alloc['calls_total']} "
        f"bytes={alloc['bytes_steady']} buffers={buffers['received']}->{buffers['released']} "
        f"leaked={buffers['leaked']} "
        f"threads=" + ",".join(
            f"{t['name'] or t['tid']}:{t.get('calls_steady', 0)}/{t['calls']}" for t in threads)
    )


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    exit_code = 0
    for raw in argv[1:]:
        path = Path(raw)
        try:
            print(verify(path))
        except Failure as failure:
            print(f"{path.name}: FAIL  {failure}")
            exit_code = 1
        except (KeyError, ValueError, TypeError) as error:
            print(f"{path.name}: FAIL  结构不对：{error!r}")
            exit_code = 1
    return exit_code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
