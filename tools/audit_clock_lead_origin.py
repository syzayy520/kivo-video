#!/usr/bin/env python3
"""对全仓 A/V 证据做「钟领先量的成因」普查（P14-U）。

**它回答的唯一问题**：那些领先量远超一帧的证据，能不能用"钟的速率不对"来解释？

不能。而且不需要逐份调参数、也不需要新证据 —— 一条物理上限就够：

    消费级晶振（含 SoC 音频域与 CLOCK_MONOTONIC 所用的那两颗）的频差在 ±50 ppm 量级。
    留 40 倍余量取 2000 ppm 作上限：超过它的偏移**不可能**是速率，只能是位置跳变。

    lead_ppm = |clock_lead_ns| * 1e6 / arrive_mono_span_ns

P14-T §三 已经用**跨份对比**证明过同一件事（8 秒窗口一样长、领先量散 10 倍），
但那条论证需要"凑一批"。本脚本把它变成**逐份可判**，外加三条结构关系。

## 为什么不用登记表

P14-S 的 `KNOWN_OUT_OF_BAND` 带反向判据，因为那张表会吸收"新出现的越界"。
这里换了个做法：**不登记具体文件，只冻结结构计数**（A1）并断言三条关系（A2–A4）。
关系变了说明有东西真的变了，人来查；多一份少一份都会让 A1 变红。

登记表的问题不是"要维护"，是它会**慢慢变成一张'曾经发生过什么'的清单**，
而读者以为它在说"现在是什么样"。

## 三条关系（都是本卡新发现的，不是复述）

A2：`drift_gate.enabled == true` 时 `|lead| ≤ limit_ns × 1.02` —— **门真的把领先量压住了**。
A3：`drift_gate.enabled == false` 时 `|lead| / limit_ns ≥ 2` —— **门关掉时确实越界**，
    与 A2 合起来才是"门有效"的完整证据（只有 A2 的话，也可以解释成"这些轮次本来就不偏"）。
A4：最大一份的 ppm > 50 000 —— 比晶振上限高三个数量级，"5.6% 是频率差"在物理上不成立。

用法：
    python tools/audit_clock_lead_origin.py                     # 全仓
    python tools/audit_clock_lead_origin.py docs/evidence/p13r  # 只看某个目录

退出码：0 = 四条全部成立；2 = 有出入。
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

EVIDENCE_ROOT = Path("docs/evidence")

# 超过这个偏移就不可能是频差。见文件头的推导。
LEAD_PPM_UPPER = 2000.0

# A1：全仓「必非速率」的份数。**这是一个观察值，不是豁免额度。**
# 它变了就说明有新证据进来（或旧证据被替换），要人去看一眼为什么，
# 而不是往一张表里补一行。
EXPECTED_BEYOND_RATE_COUNT = 30

# A2/A3 的容差。门把领先量夹到上限时不会正好等于上限
# （它按帧修正、一次削一点），实测 0.889 / 0.951，所以给 2% 余量。
GATE_COVER_RATIO_MAX = 1.02
GATE_OPEN_OVERSHOOT_MIN = 2.0

# A4：最大一份的 ppm 下限。`p13r-4k60-r3` 实测 56 585 ppm。
MIN_PEAK_PPM = 50_000.0


def load(path: Path) -> dict[str, Any] | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def clock_lead_ns(doc: dict[str, Any]) -> tuple[int, int] | None:
    """返回 `(领先量, 墙钟跨度)`；量不出时返回 None。

    两处读法按版本选择，**不混用**：
      · `/31` 起报告直接给 `pace.clock_lead_ns`；
      · `/16`–`/30` 没有这一格，但 `elapsed_span_ns` 与 `arrive_mono_span_ns` 都在，
        两者相减就是同一个数（P14-T §二 的恒等式，113 份逐份核过）。
    """
    pace = doc.get("pace")
    if not isinstance(pace, dict):
        return None
    span = pace.get("arrive_mono_span_ns")
    if not isinstance(span, int) or span <= 0:
        return None
    lead = pace.get("clock_lead_ns")
    if isinstance(lead, int):
        return lead, span
    elapsed = pace.get("elapsed_span_ns")
    if not isinstance(elapsed, int) or elapsed == 0:
        return None
    return elapsed - span, span


def main(argv: list[str]) -> int:
    roots = [Path(arg) for arg in argv[1:]] or [EVIDENCE_ROOT]
    files: list[Path] = []
    for root in roots:
        if root.is_dir():
            files.extend(sorted(root.rglob("*-sync.json")))
        elif root.is_file():
            files.append(root)

    if not files:
        print("没有找到任何 *-sync.json", file=sys.stderr)
        return 2

    failures: list[str] = []
    beyond: list[tuple[str, int, float, bool | None, int]] = []
    within: list[str] = []
    unjudgeable: list[str] = []

    for path in files:
        doc = load(path)
        if doc is None:
            failures.append(f"{path}: 读不出来（损坏或非 JSON）")
            continue
        schema = str(doc.get("schema", ""))
        if not schema.startswith("kivo.p12c.sync/"):
            unjudgeable.append(f"{path}: schema={schema or '缺失'}，不是 A/V 合并证据")
            continue

        try:
            key = path.relative_to(EVIDENCE_ROOT).as_posix()
        except ValueError:
            key = path.name

        measured = clock_lead_ns(doc)
        if measured is None:
            # `surface_at_time` 档不带钟读数，这里没有分子 —— 不可判，不是通过。
            unjudgeable.append(f"{key}: 没有钟读数（clock_supplied_frames=0 或跨度缺失）")
            continue

        lead, span = measured
        ppm = abs(lead) * 1.0e6 / span
        if ppm <= LEAD_PPM_UPPER:
            within.append(key)
            continue

        gate = doc.get("drift_gate") or {}
        enabled = gate.get("enabled")
        limit = gate.get("limit_ns")
        limit_ns = limit if isinstance(limit, int) and limit > 0 else 0
        beyond.append((key, lead, ppm, enabled if isinstance(enabled, bool) else None, limit_ns))

    # ── A1：结构计数 ──
    if len(beyond) != EXPECTED_BEYOND_RATE_COUNT:
        failures.append(
            f"A1 份数变了：{len(beyond)} 份超过 {LEAD_PPM_UPPER:.0f} ppm，"
            f"常数写的是 {EXPECTED_BEYOND_RATE_COUNT} —— 去查这批新证据是什么"
        )

    # ── A2 / A3：门开压住、门关放开 ──
    covered = 0
    overshoot = 0
    for key, lead, ppm, enabled, limit_ns in beyond:
        if enabled is True and limit_ns > 0:
            covered += 1
            ratio = abs(lead) / limit_ns
            if ratio > GATE_COVER_RATIO_MAX:
                failures.append(
                    f"A2 {key}: 门开着（limit {limit_ns} ns）但领先量 {lead} ns 是门限的 "
                    f"{ratio:.3f} 倍，超过容差 {GATE_COVER_RATIO_MAX} —— 门没压住"
                )
        elif enabled is False and limit_ns > 0:
            overshoot += 1
            ratio = abs(lead) / limit_ns
            if ratio < GATE_OPEN_OVERSHOOT_MIN:
                failures.append(
                    f"A3 {key}: 门关着（limit {limit_ns} ns）但领先量 {lead} ns 只有门限的 "
                    f"{ratio:.3f} 倍，没到 {GATE_OPEN_OVERSHOOT_MIN} —— 门关掉与否没区别"
                )

    if covered == 0:
        failures.append("A2 一份门开着的越界样本都没有，这条关系本仓无法成立")
    if overshoot == 0:
        failures.append("A3 一份门关着的越界样本都没有，这条关系本仓无法成立")

    # ── A4：峰值 ──
    peak = max(beyond, key=lambda item: item[2]) if beyond else None
    if peak is None or peak[2] < MIN_PEAK_PPM:
        failures.append(
            f"A4 最大领先量只有 {peak[2] if peak else 0:.0f} ppm，"
            f"低于 {MIN_PEAK_PPM:.0f} —— 本仓不再有'速率解释不可能成立'的样本"
        )

    print(f"覆盖：{len(files)} 份证据")
    print(f"  A1 必非速率（>{LEAD_PPM_UPPER:.0f} ppm）：{len(beyond)} 份"
          f"（常数 {EXPECTED_BEYOND_RATE_COUNT}）")
    print(f"  A2 门开且仍越界（被门压到 ≤{GATE_COVER_RATIO_MAX:.2f}× 门限）：{covered} 份")
    print(f"  A3 门关且越界（≥{GATE_OPEN_OVERSHOOT_MIN:.1f}× 门限）：{overshoot} 份")
    if peak is not None:
        print(f"  A4 峰值：{peak[2]:.0f} ppm（{peak[1] / 1e6:.3f} ms，{peak[0]}）")
    print(f"  上限内：{len(within)} 份")
    print(f"  不可判：{len(unjudgeable)} 份")

    if beyond:
        print("\n必非速率的明细（|领先量| 降序）：")
        print(f"  {'lead_ms':>9} {'ppm':>7} {'gate':>6} {'ratio':>6}  path")
        for key, lead, ppm, enabled, limit_ns in sorted(beyond, key=lambda item: -abs(item[1])):
            gate_text = "on" if enabled is True else ("off" if enabled is False else "-")
            ratio_text = f"{abs(lead) / limit_ns:.3f}" if limit_ns > 0 else "-"
            print(f"  {lead / 1e6:9.3f} {ppm:7.0f} {gate_text:>6} {ratio_text:>6}  {key}")

    if unjudgeable:
        print("\n不可判的（这些**不是**通过）：")
        for line in unjudgeable[:15]:
            print(f"  - {line}")
        if len(unjudgeable) > 15:
            print(f"  … 另有 {len(unjudgeable) - 15} 份")

    if failures:
        print("\n失败：", file=sys.stderr)
        for line in failures:
            print(f"  {line}", file=sys.stderr)
        return 2

    print("\nA1–A4 全部成立。")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
