#!/usr/bin/env python3
"""P14-I：接收圈停顿构成的结构校验。

判据只看结构，不看"停顿该多长"——多长是读数，好坏由人判：

P1 有样本      `iterations > 0`。0 样本时后面全绿，而"没测到"和"没停顿"长得一样。
P2 计数不越界  `long_gaps <= iterations`
P3 分档不漏账  `Σ buckets == iterations`
P4 残差非负且不超 gap  `0 <= worst_unaccounted_ns <= max_gap_ns`
              —— 减法写反时会出现负残差，那是最隐蔽的一种错
P5 极值自洽    `max_gap_ns` 落在非空档里（与实现同一套边界，但独立算一遍）
P6 分段不重复计  `worst_pace_ns + worst_release_ns <= worst_pump_ns + 1 ms`
              —— pace/release 是 pump 的内部，超过就是把同一段时间算了两次

用法：
    python tools/verify_p14i_gap.py docs/evidence/p14i/*.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

EDGES_NS = [
    8_000_000,
    16_000_000,
    33_000_000,
    66_000_000,
    133_000_000,
    266_000_000,
    532_000_000,
    1_000_000_000,
]


class Failure(Exception):
    pass


def bucket_of(value: int, edges: list[int]) -> int:
    index = 0
    for edge in edges:
        if value < edge:
            return index
        index += 1
    return index


def verify(path: Path) -> str | None:
    document = json.loads(path.read_text(encoding="utf-8"))
    if "metrics" not in document:
        # 同一目录里还有 sync 证据。**跳过要说出来**，不能静默通过 ——
        # 静默跳过会让"12 份证据全绿"和"只验了 2 份"长得一样。
        return None
    gap = document["metrics"]["present_gap"]

    iterations = int(gap["iterations"])
    long_gaps = int(gap["long_gaps"])
    max_gap = int(gap["max_gap_ns"])
    buckets = [int(value) for value in gap["buckets"]]
    unaccounted = int(gap["worst_unaccounted_ns"])

    if iterations <= 0:
        raise Failure("P1 iterations = 0：一圈都没记到")
    if long_gaps > iterations:
        raise Failure(f"P2 long_gaps({long_gaps}) > iterations({iterations})")
    if sum(buckets) != iterations:
        raise Failure(f"P3 分档合计 {sum(buckets)} != iterations {iterations}")
    if unaccounted < 0:
        raise Failure(f"P4 残差为负 {unaccounted}：减法写反了")
    if unaccounted > max_gap:
        raise Failure(f"P4 残差 {unaccounted} 超过最长一圈 {max_gap}")

    index = bucket_of(max_gap, EDGES_NS)
    if index >= len(buckets) or buckets[index] == 0:
        raise Failure(f"P5 最长一圈 {max_gap / 1e6:.1f} ms 应落在第 {index} 档，那一档却是 0")

    inner = int(gap["worst_pace_ns"]) + int(gap["worst_release_ns"])
    if inner > int(gap["worst_pump_ns"]) + 1_000_000:
        raise Failure(
            f"P6 pace+release({inner / 1e6:.1f} ms) 超过 pump"
            f"({int(gap['worst_pump_ns']) / 1e6:.1f} ms)：嵌套段被重复计入"
        )

    return (
        f"{path.name}: OK  iters={iterations} long={long_gaps} "
        f"max={max_gap / 1e6:.1f} ms "
        f"[submit {int(gap['worst_submit_ns']) / 1e6:.1f} "
        f"pump {int(gap['worst_pump_ns']) / 1e6:.1f} "
        f"(pace {int(gap['worst_pace_ns']) / 1e6:.1f} / release "
        f"{int(gap['worst_release_ns']) / 1e6:.1f}) "
        f"**unaccounted {unaccounted / 1e6:.1f}**]"
    )


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    exit_code = 0
    for raw in argv[1:]:
        path = Path(raw)
        try:
            line = verify(path)
            if line is None:
                print(f"{path.name}: SKIP  不是 decode 证据（无 metrics 块）")
                continue
            print(line)
        except Failure as failure:
            print(f"{path.name}: FAIL  {failure}")
            exit_code = 1
        except (KeyError, ValueError, TypeError) as error:
            print(f"{path.name}: FAIL  结构不对：{error!r}")
            exit_code = 1
    return exit_code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
