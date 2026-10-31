#!/usr/bin/env python3
"""P14-G 残差尾部的结构校验 + 出现率汇总。

为什么要这个脚本，而不是继续用 `verify_p13ae_gate`：那张卡判的是**门的次数**，
本卡判的是**残差分布**。两件事的错法完全不同 —— 前者错在策略，后者错在记账
（分档漏一个、极值和分布对不上、越阈值数漏算）。

五条判据（理由见 `docs/design/backend-core/128-p14g-design.md` §四）：

T1 样本非空    `samples > 0`。0 样本时后面全绿，而"没测到"和"没问题"长得一样。
T2 符号不漏账  `forward + backward == samples`。每个样本只有一个符号，
              漏一个就说明有分支没记账。
T3 分档不漏账  `Σ buckets == samples`。加档时忘了扩数组，这里会红。
T4 极值自洽    按证据里自己的 `edges_ns` 重新算一遍 `max_forward_ns` /
              `max_backward_ns` 该落在第几档，那一档必须非空。
              **独立写一遍边界计算**（不调用被测实现），否则就是拿实现验实现。
T8 位置自洽      `long_gaps > 0` 时 `last_long_gap_at_ns` 必须落在 `[0, span_ns]`，
              且 `first <= last`。这一格是 P14-J 加的：次数说不清"在开头还是结尾"。
T5 钳制可解释  守卫的条件是「候选步长 > 200 ms」，所以 `long_candidate_steps`
              必须 ≥ `jumps_clamped`。
              **不能写成"残差越阈值 或 间隔越阈值"** —— 首轮 30 分钟里有一次钳制，
              间隔 190 ms、残差 48 ms，两者都没越线，和却是 238 ms。
              这条判据的第一版就漏掉了它（P14-G 首轮被自己的脚本判红，是对的）。
T6 间隔不漏账  `long_gaps <= intervals`：越阈值的间隔是间隔的子集。
T7 间隔自洽    `long_gaps > 0` 当且仅当 `max_mono_step_ns >= clamp_threshold_ns`。

随后打印两行汇总（不是判据，是读数）：
  · 每轮钳制次数与本轮时长；
  · 全部轮次合并后的「次数 / 小时」泊松 95% 区间 —— 一个事件回答不了频率，
    所以只给区间，不给点估计。

用法：
    python tools/verify_p14g_tail.py docs/evidence/p14g/*-sync.json
"""

from __future__ import annotations

import json
import math
import sys
from pathlib import Path
from typing import Any


class Failure(Exception):
    pass


EXPECTED_EDGES_NS = [
    1_000_000,
    2_000_000,
    4_000_000,
    8_000_000,
    16_000_000,
    32_000_000,
    64_000_000,
    128_000_000,
    256_000_000,
    512_000_000,
]


def bucket_of(magnitude_ns: int, edges: list[int]) -> int:
    """独立实现：第 i 档覆盖 [edges[i-1], edges[i])，第 0 档是 [0, edges[0])。

    故意不复用被测代码的循环 —— 两边各写一遍，写错的方向才可能不同。
    """
    index = 0
    for edge in edges:
        if magnitude_ns < edge:
            return index
        index += 1
    return index


def poisson_interval(count: int) -> tuple[float, float]:
    """泊松 95% 区间（Garwood 精确式，用 chi-square 的近似展开）。

    只在汇总里用，不参与判定。1 次事件的区间宽到 [0.025, 5.57]，
    这正是本卡"不要点估计"的理由。
    """
    if count <= 0:
        return (0.0, 3.69)
    # 下界：chi2(0.025, 2n)/2；上界：chi2(0.975, 2n+2)/2
    lower = 0.5 * _chi2_lower_half(2 * count)
    upper = 0.5 * _chi2_upper_half(2 * count + 2)
    return (lower, upper)


def _chi2_lower_half(dof: int) -> float:
    # Wilson–Hilferty 近似，dof 很小时误差偏大，但汇总行只需量级。
    z = -1.959963985
    return dof * (1.0 - 2.0 / (9.0 * dof) + z * math.sqrt(2.0 / (9.0 * dof))) ** 3


def _chi2_upper_half(dof: int) -> float:
    z = 1.959963985
    return dof * (1.0 - 2.0 / (9.0 * dof) + z * math.sqrt(2.0 / (9.0 * dof))) ** 3


def verify(path: Path) -> tuple[str, dict[str, Any]]:
    document = json.loads(path.read_text(encoding="utf-8"))
    clock = document["audio"]["clock"]
    tail = clock["tail"]

    samples = int(tail["samples"])
    forward = int(tail["forward"])
    backward = int(tail["backward"])
    buckets = [int(value) for value in tail["buckets"]]
    edges = [int(value) for value in tail["edges_ns"]]

    # ── T1 ──
    if samples <= 0:
        raise Failure("T1 samples = 0：尾部一个样本都没采到（不是\"没有异常\"）")

    # ── T2 ──
    if forward + backward != samples:
        raise Failure(
            f"T2 符号不漏账：forward({forward}) + backward({backward}) "
            f"!= samples({samples})"
        )

    # ── T3 ──
    if sum(buckets) != samples:
        raise Failure(f"T3 分档合计 {sum(buckets)} != samples {samples}（有样本没进档）")

    # 边界必须就是约定的那十个：它进证据的意义是让读者能自己复算档位，
    # 边界漂了而档数没变，读者算出来的档号会整体错位。
    if edges != EXPECTED_EDGES_NS:
        raise Failure(f"T4 档边界与约定不符：{edges}")

    # ── T4 ──
    for key, label in (("max_forward_ns", "正"), ("max_backward_ns", "负")):
        magnitude = int(tail[key])
        if magnitude == 0:
            continue
        index = bucket_of(magnitude, edges)
        if index >= len(buckets) or buckets[index] == 0:
            raise Failure(
                f"T4 极值自洽：{label}残差最大 {magnitude / 1e6:.3f} ms 应落在第 {index} 档，"
                f"但那一档计数为 0（分布与极值不是同一批样本）"
            )

    # ── T8：长间隔的位置自洽（P14-J）──
    first_at = int(tail.get("first_long_gap_at_ns", -1))
    last_at = int(tail.get("last_long_gap_at_ns", -1))
    span = int(tail.get("span_ns", 0))

    # ── T5：钳制必须被"真跳"或"长间隔"之一解释掉 ──
    beyond = int(tail["beyond_threshold"])
    clamps = int(clock["jumps_clamped"])
    intervals = int(tail["intervals"])
    long_gaps = int(tail["long_gaps"])
    max_step = int(tail["max_mono_step_ns"])
    long_steps = int(tail["long_candidate_steps"])
    max_cand = int(tail["max_candidate_step_ns"])
    if long_steps < clamps:
        raise Failure(
            f"T5 钳制可解释：jumps_clamped = {clamps}，但候选步长越阈值只有 "
            f"{long_steps} 次（最长 {max_cand / 1e6:.1f} ms）—— 有钳制没有来由"
        )
    if long_steps > intervals:
        raise Failure(f"T6 步长不漏账：long_candidate_steps({long_steps}) > intervals({intervals})")

    # `/24` 之前没有这两格。**不重写历史证据**，显式跳过并在输出里说明 ——
    # 静默跳过会让"3 份全绿"和"只验了 1 份"长得一样。
    schema_index = 0
    if "/" in str(document.get("schema", "")):
        try:
            schema_index = int(str(document["schema"]).rsplit("/", 1)[1])
        except ValueError:
            schema_index = 0

    if schema_index >= 24 and long_gaps > 0:
        if last_at < 0 or span <= 0:
            raise Failure(f"T8 有 {long_gaps} 次长间隔，却没记下位置（last_at={last_at}）")
        if first_at < 0 or first_at > last_at:
            raise Failure(f"T8 首个长间隔位置 {first_at} 与最后一个 {last_at} 顺序不对")
        if last_at > span:
            raise Failure(f"T8 最后长间隔位置 {last_at} 超过会话跨度 {span}")
    if schema_index >= 24 and long_gaps == 0 and last_at >= 0:
        raise Failure(f"T8 long_gaps = 0 却记下了位置 {last_at}")

    # ── T6 ──
    if long_gaps > intervals:
        raise Failure(f"T6 间隔不漏账：long_gaps({long_gaps}) > intervals({intervals})")

    # ── T7 ──
    threshold = int(tail["clamp_threshold_ns"])
    if long_gaps > 0 and max_step < threshold:
        raise Failure(
            f"T7 间隔自洽：long_gaps = {long_gaps} 但最长间隔只有 {max_step / 1e6:.2f} ms"
        )
    if long_gaps == 0 and max_step >= threshold:
        raise Failure(
            f"T7 间隔自洽：最长间隔 {max_step / 1e6:.2f} ms 已越阈值，long_gaps 却是 0"
        )

    duration_s = float(document["source"]["duration_us"]) / 1e6
    return (
        f"{path.name}: OK  samples={samples} fwd/bwd={forward}/{backward} "
        f"max=+{int(tail['max_forward_ns']) / 1e6:.2f}/-{int(tail['max_backward_ns']) / 1e6:.2f} ms "
        f"beyond={beyond} clamps={clamps} "
        f"steps={long_steps}/{max_cand / 1e6:.0f}ms gaps={long_gaps}/{max_step / 1e6:.0f}ms "
        f"rejects={int(clock['monotonic_rejects'])} "
        f"dur={duration_s:.0f}s p99={document['pace']['lateness_ns']['p99'] / 1e6:.2f} ms",
        {
            "duration_s": duration_s,
            "clamps": clamps,
            "rejects": int(clock["monotonic_rejects"]),
            "beyond": beyond,
            "long_gaps": long_gaps,
            "long_steps": long_steps,
            "max_cand_ms": max_cand / 1e6,
            "max_step_ms": max_step / 1e6,
            "samples": samples,
            "buckets": buckets,
        },
    )


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2

    exit_code = 0
    durations = 0.0
    total_clamps = 0
    total_rejects = 0
    total_gaps = 0
    total_steps = 0
    worst_cand_ms = 0.0
    worst_gap_ms = 0.0
    total_samples = 0
    sum_beyond = 0
    merged = [0] * len(EXPECTED_EDGES_NS)

    for raw in argv[1:]:
        path = Path(raw)
        try:
            line, facts = verify(path)
            print(line)
        except Failure as failure:
            print(f"{path.name}: FAIL  {failure}")
            exit_code = 1
            continue
        except (KeyError, ValueError, TypeError) as error:
            print(f"{path.name}: FAIL  结构不对：{error!r}")
            exit_code = 1
            continue

        durations += facts["duration_s"]
        total_clamps += facts["clamps"]
        total_rejects += facts["rejects"]
        total_gaps += facts["long_gaps"]
        total_steps += facts["long_steps"]
        worst_cand_ms = max(worst_cand_ms, facts["max_cand_ms"])
        worst_gap_ms = max(worst_gap_ms, facts["max_step_ms"])
        total_samples += facts["samples"]
        sum_beyond += facts["beyond"]
        for index, value in enumerate(facts["buckets"][: len(merged)]):
            merged[index] += value

    if durations > 0:
        hours = durations / 3600.0
        low, high = poisson_interval(total_clamps)
        print()
        print(
            f"合计 {len(argv) - 1} 轮 / {durations / 60:.1f} 分钟 / {total_samples} 个样本"
        )
        print(
            f"  钳制 {total_clamps} 次（泊松 95% 区间 {low / hours:.2f}–{high / hours:.2f} 次/小时）"
            f"　微倒退 {total_rejects} 次（{total_rejects / hours:.2f} 次/小时）"
        )
        # 钳制里有多少次是"我们没看"造成的：这些不是设备的错，是查询循环被卡住。
        print(
            f"  长间隔（≥200 ms）{total_gaps} 次，最长 {worst_gap_ms:.0f} ms；"
            f"候选步长越阈值 {total_steps} 次，最长 {worst_cand_ms:.0f} ms；"
            f"越阈值残差 {sum_beyond} 次"
        )
        # 尾部形状：只报 16 ms 以上的档，下面的档是正常量化，看不看都一样。
        tail_line = "  ".join(
            f"{EXPECTED_EDGES_NS[index - 1] / 1e6:.0f}-"
            f"{EXPECTED_EDGES_NS[index] / 1e6:.0f}ms:{merged[index]}"
            for index in range(5, len(merged) - 1)
        )
        print(f"  尾部（每档样本数） {tail_line}  >512ms:{merged[-1]}")
    return exit_code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
