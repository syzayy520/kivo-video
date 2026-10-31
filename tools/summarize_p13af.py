#!/usr/bin/env python3
"""P13-AF 跨臂汇总：同一 ppm，分辨率/帧率会不会挪动门的触发次数。

单臂的判据（次数 vs ppm×时长/band）**不在这里重写** —— 那是
`tools/verify_p13ae_gate.py` 的 F1–F4。本脚本只加两条跨臂判据：

  R2 三臂极差 ≤ 4 次（约 10%）。超过说明还有第二个自变量。
  R5 同一臂两轮之差 ≤ 3 次。超过说明这一轮不可复现，结论不成立。

为什么极差用绝对次数而不是百分比：15 次的 10% 是 1.5，而计数是整数 ——
用百分比会因为一次四舍五入的抖动把正常结果判红。

用法：
    python tools/summarize_p13af.py docs/evidence/p13af/*-sync.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from verify_p13ae_gate import predict_triggers  # 单一来源，禁止复制判据

RANGE_LIMIT = 4
ROUND_DIFF_LIMIT = 3

# 文件名里的臂标记 → 人类可读标签
ARM_LABELS = {
    "a1080p30": "A 1080p30（基线）",
    "b1080p60": "B 1080p60（只变帧率）",
    "c4k30": "C 4K30（只变分辨率）",
}


def arm_of(name: str) -> str | None:
    for key in ARM_LABELS:
        if key in name:
            return key
    return None


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2

    rows: dict[str, list[dict]] = {}
    for raw in argv[1:]:
        path = Path(raw)
        data = json.loads(path.read_text(encoding="utf-8"))
        gate = data["drift_gate"]
        pace = data["pace"]
        # `metrics` 在 **decode** 证据里，不在 sync 里（M5 判据：三条必须在同一份
        # decode 证据里）。sync 文件旁边那份同名去 `-sync` 的就是它。
        decode_path = path.with_name(path.name.replace("-sync.json", ".json"))
        decode_doc = json.loads(decode_path.read_text(encoding="utf-8")) if decode_path.exists() else {}
        alloc = decode_doc.get("metrics", {}).get("alloc", {})
        cpu_metrics = decode_doc.get("metrics", {}).get("cpu", {})
        timeline = data["timeline"]

        ppm = float(data["clock_injection"]["rate_ppm"])
        duration_s = int(data["source"]["duration_us"]) / 1e6
        expected = predict_triggers(ppm, duration_s, int(gate["limit_ns"]), int(gate["band_ns"]))

        health = pace["clock_health"]
        rebases = int(health["drift_rebases"])
        max_corr = int(health["drift_max_correction_ns"])
        total_corr = int(health["drift_total_correction_ns"]) if "drift_total_correction_ns" in health else None
        if total_corr is None:
            total_corr = int(health.get("drift_corrected_total_ns", 0))
        steady = int(alloc.get("calls_steady", 0)) if alloc else -1

        row = {
            "file": path.name,
            "ppm": ppm,
            "duration_s": duration_s,
            "expected": expected,
            "rebases": rebases,
            "first_corr_ms": max_corr / 1e6,
            "steady_avg_ms": ((total_corr - max_corr) / (rebases - 1) / 1e6) if rebases >= 2 else 0.0,
            "dropped": int(data["video"]["frames_dropped_late"]),
            # P14-S：`/30` 起这两个键改了名（旧名说的是"pts 跨度"，而它从来不是）。
            # 两个名字都认：本脚本读的是 p13a* 的历史证据，改名的报告不该让它崩。
            "ahead_ms": (int(timeline.get("nominal_span_ns", timeline.get("video_pts_span_ns")))
                         - int(timeline["corrected_wall_span_ns"])) / 1e6,
            "rate": float(timeline.get("nominal_rate", timeline.get("playback_rate"))),
            "proc_cpu": float(cpu_metrics.get("process_ratio", 0.0)),
            "alloc_steady": steady,
        }
        arm = arm_of(path.name)
        if arm is None:
            print(f"{path.name}: 跳过（文件名里没有臂标记）")
            continue
        rows.setdefault(arm, []).append(row)

    print(f"{'臂':<22}{'轮':<5}{'次数':>5}{'预测':>5}{'首次ms':>8}{'稳态ms':>8}"
          f"{'ahead ms':>10}{'进程CPU':>9}{'稳态分配':>9}")
    exit_code = 0
    ranges = {}
    for arm in ("a1080p30", "b1080p60", "c4k30"):
        if arm not in rows:
            print(f"{ARM_LABELS[arm]}: 缺证据")
            exit_code = 1
            continue
        for index, row in enumerate(sorted(rows[arm], key=lambda item: item["file"]), start=1):
            print(f"{ARM_LABELS[arm]:<20}{index:>4}{row['rebases']:>6}{row['expected']:>6}"
                  f"{row['first_corr_ms']:>9.2f}{row['steady_avg_ms']:>8.2f}"
                  f"{row['ahead_ms']:>10.2f}{row['proc_cpu'] * 100:>8.1f}%{row['alloc_steady']:>9}")
        counts = [row["rebases"] for row in rows[arm]]
        ranges[arm] = (min(counts), max(counts))
        if len(counts) >= 2 and max(counts) - min(counts) > ROUND_DIFF_LIMIT:
            print(f"  R5 FAIL {ARM_LABELS[arm]}：两轮相差 {max(counts) - min(counts)} 次 > {ROUND_DIFF_LIMIT}")
            exit_code = 1

    if len(ranges) == 3:
        low = min(value[0] for value in ranges.values())
        high = max(value[1] for value in ranges.values())
        spread = high - low
        verdict = "PASS" if spread <= RANGE_LIMIT else "FAIL"
        print(f"\nR2 三臂极差 = {spread} 次（{low}–{high}），门槛 ≤ {RANGE_LIMIT} → {verdict}")
        if spread > RANGE_LIMIT:
            exit_code = 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
