#!/usr/bin/env python3
"""P12-D5 独立复算：只吃 `probe_present_timing.py` 写出的 JSON，从原始行重算分位数。

**刻意不 import 探针。** 探针和复算共用一份代码，就只能证明"它跟自己一致"。
这里的分位数用另一套写法（秩索引），两套都对上才算数。

跑法：
    python tools/verify_p12d5_present.py docs/evidence/p12d5/p12d5-*.json
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

INT64_MAX = 2**63 - 1
MONOTONIC_THRESHOLD_NS = 10**14
STARTUP_ROWS = 15  # 仅作为证据里没写这个字段时的兜底


class Failure(RuntimeError):
    pass


def rank_percentile(sorted_values: list[int], q: float) -> int:
    """秩索引分位数（nearest-rank）。与探针的写法不同源，故意如此。"""
    if not sorted_values:
        raise Failure("空样本")
    index = int(math.ceil(q * len(sorted_values))) - 1
    return sorted_values[max(0, min(index, len(sorted_values) - 1))]


def check_file(path: Path) -> dict:
    raw = path.read_text(encoding="utf-8")
    if not raw.strip():
        # 空文件比缺文件更坏：它会让"证据存在"这句话成立，却什么都没有。
        raise Failure(f"{name}: 0 字节")
    data = json.loads(raw)
    name = path.name

    if data.get("schema") != "kivo.p12d5.present-timing/1":
        raise Failure(f"{name}: schema 不是 kivo.p12d5.present-timing/1")

    # 层名必须带 #id。少了它 --latency 只返回刷新率一行 —— C7 就是栽在这，
    # 把"参数写错"当成了"这个架构测不了"。这条检查就是防止它再发生。
    layer = data.get("layer", "")
    if "#" not in layer:
        raise Failure(f"{name}: 层名里没有 #id（{layer!r}），这个窗口的数据不可信")

    refresh = int(data.get("refresh_period_ns", 0))
    if refresh <= 0:
        raise Failure(f"{name}: refresh_period_ns 非法（{refresh}）")

    rows = data.get("rows", [])
    summary = data.get("summary", {})
    if summary.get("samples") != len(rows):
        raise Failure(f"{name}: summary.samples({summary.get('samples')}) != len(rows)({len(rows)})")

    scheduled: list[tuple[int, int]] = []
    pending = unscheduled = 0
    for row in rows:
        desired, actual = int(row["desired"]), int(row["actual"])
        if actual >= INT64_MAX - 1_000_000:
            pending += 1
        elif desired < MONOTONIC_THRESHOLD_NS:
            unscheduled += 1
        else:
            scheduled.append((desired, actual))

    if pending + unscheduled + len(scheduled) != len(rows):
        raise Failure(f"{name}: 三类之和不等于总行数")
    for key, value in (("pending_samples", pending), ("unscheduled_samples", unscheduled),
                       ("scheduled_samples", len(scheduled))):
        if summary.get(key) != value:
            raise Failure(f"{name}: {key} 证据写 {summary.get(key)}，重算得 {value}")

    schedule = data.get("present_schedule")
    if schedule == "surface_at_time" and not scheduled:
        raise Failure(f"{name}: surface_at_time 下却一帧可算的都没有 —— 时间戳没传下去？")
    if schedule == "cpu_paced" and scheduled:
        # 不是不可能，但那样"cpu_paced 测不了 Δ"这句话就错了，必须重写文档。
        raise Failure(f"{name}: cpu_paced 下竟有 {len(scheduled)} 帧带单调时间戳，与既有结论矛盾")

    if not scheduled:
        reason = summary.get("delta_unavailable_reason", "")
        if not reason:
            raise Failure(f"{name}: 没有可算样本，但没写理由 —— 空白会被读成'零偏差'")
        return {"name": name, "samples": len(rows), "delta": None, "pending": pending,
                "reason": reason}

    deltas = sorted(a - d for d, a in scheduled)
    expected = {
        "p01": rank_percentile(deltas, 0.01),
        "p10": rank_percentile(deltas, 0.10),
        "p50": rank_percentile(deltas, 0.50),
        "p90": rank_percentile(deltas, 0.90),
        "p99": rank_percentile(deltas, 0.99),
        "min": deltas[0],
        "max": deltas[-1],
    }
    claimed = summary.get("delta_ns") or {}
    for key, value in expected.items():
        if int(claimed.get(key, -10**18)) != value:
            raise Failure(
                f"{name}: delta_ns.{key} 证据写 {claimed.get(key)}，重算得 {value}"
                f"（差 {(int(claimed.get(key, 0)) - value) / 1e6:.3f} ms）"
            )

    missed = sum(1 for d in deltas if d >= refresh)
    if summary.get("vsync_missed_frames") != missed:
        raise Failure(
            f"{name}: vsync_missed_frames 证据写 {summary.get('vsync_missed_frames')}，重算得 {missed}"
        )

    # 稳态必须**按时间顺序**切：sorted 之后切等于削掉最小的几个 Δ。
    #
    # 切多少帧**从证据里读**，不在这里写死。写死过一次：探针把窗口从 3 帧改成 15 帧，
    # 复算脚本还按 3 帧算，于是 7 份全红 —— 那不是数据错了，是两份代码各记了一个常量。
    startup_rows = int(summary.get("startup_rows_excluded", STARTUP_ROWS))
    chrono = [a - d for d, a in scheduled]

    startup = sorted(chrono[:startup_rows])
    if startup and summary.get("startup_delta_ns"):
        claimed_startup = summary["startup_delta_ns"]
        for key, q in (("p50", 0.50), ("p99", 0.99)):
            if int(claimed_startup.get(key, -10**18)) != rank_percentile(startup, q):
                raise Failure(f"{name}: startup_delta_ns.{key} 对不上")
        startup_missed = sum(1 for d in startup if d >= refresh)
        if summary.get("startup_vsync_missed_frames") != startup_missed:
            raise Failure(f"{name}: startup_vsync_missed_frames 对不上")

    steady = sorted(chrono[startup_rows:])
    if steady and summary.get("steady_delta_ns"):
        claimed_steady = summary["steady_delta_ns"]
        for key in ("p50", "p99", "max"):
            q = {"p50": 0.50, "p99": 0.99}.get(key)
            value = rank_percentile(steady, q) if q else steady[-1]
            if int(claimed_steady.get(key, -10**18)) != value:
                raise Failure(f"{name}: steady_delta_ns.{key} 对不上")
        steady_missed = sum(1 for d in steady if d >= refresh)
        if summary.get("steady_vsync_missed_frames") != steady_missed:
            raise Failure(f"{name}: steady_vsync_missed_frames 对不上")

    return {
        "name": name,
        "samples": len(rows),
        "scheduled": len(scheduled),
        "pending": pending,
        "delta": expected,
        "missed": missed,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="+")
    args = parser.parse_args()

    failures = 0
    for pattern in args.files:
        for path in sorted(Path(".").glob(pattern)) if any(c in pattern for c in "*?") else [Path(pattern)]:
            try:
                result = check_file(path)
            except Failure as exc:
                print(f"FAIL {exc}")
                failures += 1
                continue
            if result["delta"] is None:
                print(f"OK   {result['name']}: {result['samples']} 行，无可用 Δ（{result['reason'][:28]}…）")
            else:
                d = result["delta"]
                print(
                    f"OK   {result['name']}: scheduled={result['scheduled']} pending={result['pending']} "
                    f"Δ p50={d['p50']/1e6:.3f} p90={d['p90']/1e6:.3f} p99={d['p99']/1e6:.3f} "
                    f"max={d['max']/1e6:.3f} ms  vsync_miss={result['missed']}"
                )
    print(f"--- {failures} 份不一致 ---")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
