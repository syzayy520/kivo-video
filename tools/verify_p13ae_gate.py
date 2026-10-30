#!/usr/bin/env python3
"""P13-AE 门的标定校验：次数是不是算术应有的那个数。

预测只用证据里已有的四格：`drift_gate.limit_ns`、`drift_gate.band_ns`、
`clock_injection.rate_ppm`、`source.duration_us`。改策略只要升 schema，
不会出现"新实现被脚本里的旧上限卡住"。

四条判据（理由见 `docs/design/backend-core/114-p13ae-status.md` §三）：

F1 算术违背   累积量还没到 `limit` 却**反复**触发（>1 次）—— 迟滞带坏了。
              允许 1 次，前提是同一轮有 `monotonic_rejects` / `extrapolated_frames`：
              那是**时钟事件**被门如实吸收，不是门凭空动作。
F2 次数偏差   观测与预测相差超过 2 次（两个方向都算）。
F3 稳态幅度   除第一次之外，每次修正必须 ≤ `band` + 1 ms。
              「第一次」可以很大：它要吃掉启动/跳变留下的一次性偏移，
              这条区别是 AE 真机跑出来的 —— 把 F3 写成"每次都 ≤ band"会打红 r2000 的 62 ms 首次修正。
F4 关闸对照   门关时必须 0 次，且领先量 ≈ ppm × 有效时长（±15%）。

用法：
    python tools/verify_p13ae_gate.py docs/evidence/p13ae/*-sync.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

# 注入点与启动盲期各 1 s，两者可能重叠。取 2 s 是保守估计：
# T_eff 小 → 预测次数小 → F2 容忍变松。宁可放松，也不要判错一个正确的门。
EFFECTIVE_LOSS_S = 2.0

COUNT_SLACK = 2

# `T_eff` 的不确定性（秒）折算成次数：1 s 的偏差在 5000 ppm 上就是 1 次触发。
# 不折算的话，越快的速率越容易被"估算误差"判红 —— 那是脚本的错，不是门的错。
T_EFF_UNCERTAINTY_S = 1.0

CORRECTION_SLACK_NS = 1_000_000
OFF_TRACK_TOLERANCE = 0.15


class Failure(Exception):
    """一条可读的失败。"""


def predict_triggers(ppm: float, duration_s: float, limit_ns: int, band_ns: int) -> int:
    """第一次攒满 limit，之后每次只攒 band（迟滞带）。"""
    if band_ns <= 0 or duration_s <= 0 or ppm <= 0.0:
        return 0
    accumulated_ns = (ppm / 1e6) * max(0.0, duration_s - EFFECTIVE_LOSS_S) * 1e9
    if accumulated_ns <= limit_ns:
        return 0
    return int((accumulated_ns - limit_ns) // band_ns) + 1


def verify(path: Path) -> str:
    data: dict[str, Any] = json.loads(path.read_text(encoding="utf-8"))
    schema = str(data.get("schema", ""))
    if not schema.startswith("kivo.p12c.sync/"):
        raise Failure(f"schema 是 {schema!r}，不像 sync 证据")

    gate = data["drift_gate"]
    enabled = bool(gate["enabled"])
    limit_ns = int(gate["limit_ns"])
    band_ns = int(gate["band_ns"])
    ppm = float(data["clock_injection"]["rate_ppm"])
    duration_s = int(data["source"]["duration_us"]) / 1_000_000.0

    pace = data["pace"]
    health = pace["clock_health"]
    rebases = int(health["drift_rebases"])
    max_correction = int(health["drift_max_correction_ns"])
    total_correction = int(health["drift_corrected_total_ns"])
    clock_events = int(health["monotonic_rejects"]) + int(health["extrapolated_frames"])
    dropped = int(data["video"]["frames_dropped_late"])
    frame_ns = int(pace["frame_duration_ns"])
    p99 = int(pace["lateness_ns"]["p99"])

    timeline = data["timeline"]
    ahead_ns = int(timeline["video_pts_span_ns"]) - int(timeline["corrected_wall_span_ns"])
    rate = float(timeline["playback_rate"])

    expected = predict_triggers(ppm, duration_s, limit_ns, band_ns) if enabled else 0

    # ── F4：门关对照 ──
    if not enabled:
        if rebases != 0:
            raise Failure(f"F4 门关却触发了 {rebases} 次")
        predicted_ahead = (ppm / 1e6) * max(0.0, duration_s - EFFECTIVE_LOSS_S) * 1e9
        if predicted_ahead > 0:
            ratio = ahead_ns / predicted_ahead
            if not (1.0 - OFF_TRACK_TOLERANCE) <= ratio <= (1.0 + OFF_TRACK_TOLERANCE):
                raise Failure(
                    f"F4 门关领先量 {ahead_ns / 1e6:.1f} ms 与 ppm×时长 "
                    f"{predicted_ahead / 1e6:.1f} ms 差 {(ratio - 1) * 100:+.1f}%"
                )
        return (
            f"{path.name}: OK  gate=off ppm={ppm:.0f} rebases=0 "
            f"ahead={ahead_ns / 1e6:+.2f} ms rate={rate:.6f} dropped={dropped}"
        )

    # ── F1：算术违背（低速率下反复触发）──
    accumulated_ns = (ppm / 1e6) * max(0.0, duration_s - EFFECTIVE_LOSS_S) * 1e9
    unexplained = accumulated_ns <= limit_ns
    if unexplained and rebases > 1:
        raise Failure(
            f"F1 {ppm:.0f} ppm × {duration_s - EFFECTIVE_LOSS_S:.0f} s = "
            f"{accumulated_ns / 1e6:.1f} ms 未达门限 {limit_ns / 1e6:.1f} ms，却触发了 {rebases} 次"
        )
    if unexplained and rebases == 1 and clock_events == 0:
        raise Failure(
            f"F1 {ppm:.0f} ppm 只累积 {accumulated_ns / 1e6:.1f} ms，触发了一次，"
            f"而同轮没有任何时钟事件（monotonic_rejects / extrapolated 均为 0）—— 无法归因"
        )

    # ── F2：次数偏差 ──
    slack = COUNT_SLACK + int((ppm / 1e6) * T_EFF_UNCERTAINTY_S * 1e9 / band_ns) if band_ns > 0 else COUNT_SLACK
    if abs(rebases - expected) > slack:
        raise Failure(
            f"F2 观测 {rebases} 次与预测 {expected} 次相差超过 {slack}"
            f"（{ppm:.0f} ppm / {duration_s:.0f} s / band {band_ns / 1e6:.1f} ms）"
        )

    # ── F3：稳态幅度（第一次除外）──
    if rebases >= 2:
        steady_avg = (total_correction - max_correction) / (rebases - 1)
        if steady_avg > band_ns + CORRECTION_SLACK_NS:
            raise Failure(
                f"F3 除首次外平均每次修 {steady_avg / 1e6:.2f} ms，超出 band "
                f"{band_ns / 1e6:.1f} ms + 1 ms"
            )
    # 丢帧只有在**没有时钟事件**的时候才算门的责任：
    # r100 那一轮的 1 帧丢在 `monotonic_rejects = 1` 的同一次跳变上，
    # 把账算给门是错的（见 114-p13ae-status.md §四）。
    if dropped > 0 and clock_events == 0:
        raise Failure(f"F3 触发期间丢帧 {dropped} 帧，且同轮没有任何时钟事件可归因")
    if frame_ns > 0 and p99 > frame_ns:
        raise Failure(f"F3 lateness p99 {p99 / 1e6:.2f} ms 超过一帧 {frame_ns / 1e6:.2f} ms")

    steady = (
        (total_correction - max_correction) / (rebases - 1) / 1e6 if rebases >= 2 else 0.0
    )
    return (
        f"{path.name}: OK  gate=on ppm={ppm:.0f} rebases={rebases} expected={expected} "
        f"first={max_correction / 1e6:.2f} ms steady={steady:.2f} ms "
        f"ahead={ahead_ns / 1e6:+.2f} ms rate={rate:.6f} dropped={dropped} p99={p99 / 1e6:.2f} ms"
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
