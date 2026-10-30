#!/usr/bin/env python3
"""独立复算 P12-C8 的 A/V 合并证据（`kivo.p12c.sync/1`）。

这个脚本存在的唯一理由，是让"可复算"不只是一句口号。

证据里除了 p50/p95/p99/max，还带**完整分布**（溢出计数 + 精确极值 + 非空桶计数
+ 桶映射的三个常量）。于是任何人——包括一个不相信这份证据的人——都能从分布把
分位数重算一遍，看看是不是同一个数。只给汇总值的话，读者唯一的选择是相信。

重算规则与 `backend/src/video/video_plane/core/pace/pace_histogram.cxx` 的
`SignedHistogram::percentile_ns()` 逐行对应：秩取上整、桶心取值、结果钳进
[min, max] 之间。任何一处不一致，本脚本都会**按整数逐位比对**并报错——
这里不做"容差内就算过"，因为误差为 0 才是这套设计的意义。

用法：
    python tools/verify_p12c_sync.py docs/evidence/p12c/p12c8-av-r1-sync.json
    python tools/verify_p12c_sync.py docs/evidence/p12c/*-sync.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

# 与 C++ 侧逐字一致。写死而不是从 JSON 里读，是为了让"读到的常量"本身也被检查：
# 如果哪天桶宽被改了而脚本没跟上，这里会报错，而不是悄悄用新常量算出"通过"。
EXPECTED_BUCKET_WIDTH_NS = 250_000
EXPECTED_LOW_NS = -128_000_000
EXPECTED_BUCKET_COUNT = 1024

REQUIRED_TOP_LEVEL = (
    "device",
    "codec",
    "source",
    "frame_count",
    "frames_dropped",
    "xrun_count",
    "clock_source",
    "av_drift",
    "present_jitter",
    "timestamp_fallback_count",
    "completed",
    "error",
)


class Failure(Exception):
    """一条可读的失败。用异常而不是布尔返回值：调用方不需要区分失败类型。"""


def bucket_center_ns(index: int) -> int:
    return EXPECTED_LOW_NS + index * EXPECTED_BUCKET_WIDTH_NS + EXPECTED_BUCKET_WIDTH_NS // 2


def replay_percentile(hist: dict[str, Any], quantile: float) -> int:
    """只吃导出到 JSON 的分布，重放 `SignedHistogram::percentile_ns()`。"""
    count = int(hist["count"])
    if count == 0:
        return 0
    if quantile <= 0.0:
        return int(hist["min_ns"])
    if quantile >= 1.0:
        return int(hist["max_ns"])

    # 秩取上整：100 个样本的 p99 是第 99 个，不是第 98.01 个。
    scaled = quantile * float(count)
    rank = int(scaled)
    if float(rank) < scaled:
        rank += 1
    if rank < 1:
        rank = 1

    low = int(hist["low_ns"])
    max_ns = int(hist["max_ns"])
    min_ns = int(hist["min_ns"])

    def clamp(value: int) -> int:
        if value < min_ns:
            return min_ns
        if value > max_ns:
            return max_ns
        return value

    below = int(hist["below"])
    if rank <= below:
        return min_ns

    buckets = {int(key): int(value) for key, value in hist["buckets"].items()}
    accumulated = below
    for index in range(EXPECTED_BUCKET_COUNT):
        accumulated += buckets.get(index, 0)
        if rank <= accumulated:
            center = low + index * EXPECTED_BUCKET_WIDTH_NS + EXPECTED_BUCKET_WIDTH_NS // 2
            return clamp(center)
    return max_ns


def check_distribution(name: str, block: dict[str, Any]) -> None:
    """分布自身的自洽 + 与汇总值的逐位比对。"""
    hist = block.get("histogram")
    if hist is None:
        raise Failure(f"{name}: 缺少 histogram，无法复算")

    if int(hist["bucket_width_ns"]) != EXPECTED_BUCKET_WIDTH_NS:
        raise Failure(f"{name}: 桶宽 {hist['bucket_width_ns']} != {EXPECTED_BUCKET_WIDTH_NS}")
    if int(hist["low_ns"]) != EXPECTED_LOW_NS:
        raise Failure(f"{name}: 下界 {hist['low_ns']} != {EXPECTED_LOW_NS}")
    if int(hist["bucket_count"]) != EXPECTED_BUCKET_COUNT:
        raise Failure(f"{name}: 桶数 {hist['bucket_count']} != {EXPECTED_BUCKET_COUNT}")

    count = int(hist["count"])
    below = int(hist["below"])
    above = int(hist["above"])
    buckets = {int(k): int(v) for k, v in hist["buckets"].items()}
    bucket_total = sum(buckets.values())
    if bucket_total + below + above != count:
        raise Failure(
            f"{name}: 桶合计 {bucket_total} + 下溢 {below} + 上溢 {above} "
            f"!= 样本数 {count}（有样本没被记到任何地方）"
        )
    if any(index < 0 or index >= EXPECTED_BUCKET_COUNT for index in buckets):
        raise Failure(f"{name}: 桶下标越界")

    for quantile, key in ((0.50, "p50"), (0.95, "p95"), (0.99, "p99")):
        recomputed = replay_percentile(hist, quantile)
        claimed = int(block[key])
        if recomputed != claimed:
            raise Failure(
                f"{name}.{key}: 证据写 {claimed} ns，从分布重算得 {recomputed} ns"
                f"（差 {claimed - recomputed} ns）"
            )

    if int(block["max"]) != int(hist["max_ns"]):
        raise Failure(f"{name}.max 与 histogram.max_ns 不一致")
    if int(block["samples"]) != count:
        raise Failure(f"{name}.samples 与 histogram.count 不一致")

    values = [int(block["p50"]), int(block["p95"]), int(block["p99"])]
    if values != sorted(values):
        raise Failure(f"{name}: p50/p95/p99 不是单调的：{values}")
    if int(hist["min_ns"]) > values[0] or values[-1] > int(hist["max_ns"]):
        raise Failure(f"{name}: 分位数越出 [min, max]")
    # C6B/C7 的老毛病：分位数落在极值之外，看起来像"p99 比 max 还大"。
    if int(block["p99"]) > int(block["max"]):
        raise Failure(f"{name}: p99 > max，读数自相矛盾")


def verify(path: Path) -> str:
    data = json.loads(path.read_text(encoding="utf-8"))

    schema = data.get("schema")
    if schema != "kivo.p12c.sync/1":
        raise Failure(f"schema 是 {schema!r}，期望 'kivo.p12c.sync/1'")

    missing = [key for key in REQUIRED_TOP_LEVEL if key not in data]
    if missing:
        raise Failure(f"缺必含字段：{missing}")

    # `av_drift` 只有在能被测量时才该带分布；不可测量时必须是"带理由的缺席"，
    # 而不是一堆 0 —— 一堆 0 会被读成"零偏差"，那是最坏的一种假证据。
    drift = data["av_drift"]
    measurable = bool(drift.get("measurable", False))
    if measurable:
        check_distribution("av_drift", drift)
    else:
        reason = str(drift.get("unavailable_reason", "")).strip()
        if not reason:
            raise Failure("av_drift 不可测量却没说理由")
        if int(drift.get("samples", 0)) != 0:
            raise Failure("av_drift 不可测量却带了样本")

    check_distribution("present_jitter", data["present_jitter"])

    # 帧数闭合：漏交一个缓冲，几十帧后解码器会彻底停住。
    video = data["video"]
    received = int(video["frames_received"])
    released = int(video["frames_released"])
    rendered = int(video["frames_rendered"])
    dropped = int(video["frames_dropped_late"])
    # P12-D1 之后闭合式多了一项：「没有面可上」丢掉的帧。
    # 缺这一项时按 0 处理，旧证据（/4、/5）照样能过。
    discarded = int(video.get("frames_discarded_no_surface", 0))
    if released != received:
        raise Failure(f"frames_released={released} != frames_received={received}（有输出槽没归还）")
    if rendered + dropped + discarded + 1 != received:
        raise Failure(
            f"frames_rendered({rendered}) + frames_dropped_late({dropped}) + "
            f"frames_discarded_no_surface({discarded}) + 1 != "
            f"frames_received({received})（末尾 EOS 缓冲没算对）"
        )
    if int(data["frame_count"]) != rendered:
        raise Failure("顶层 frame_count 与 video.frames_rendered 不一致")
    if int(data["frames_dropped"]) != dropped:
        raise Failure("顶层 frames_dropped 与 video.frames_dropped_late 不一致")

    # 时钟分档：三者之和必须等于决策帧数，漏记会让回退比例算小。
    mix = data["pace"]["clock_mix"]
    mix_total = int(mix["primary_frames"]) + int(mix["fallback_frames"]) + int(mix["system_frames"])
    if int(data["pace"]["frames_decided"]) > 0 and mix_total != int(data["pace"]["frames_decided"]):
        raise Failure(
            f"clock_mix 合计 {mix_total} != frames_decided {data['pace']['frames_decided']}"
        )

    # 顶层 clock_source 必须与逐帧分档说同一件事。
    clock_source = str(data["clock_source"])
    if mix_total > 0:
        if clock_source == "system" and int(mix["system_frames"]) != mix_total:
            raise Failure(
                f"顶层 clock_source=system，但 {mix_total - int(mix['system_frames'])} 帧用的是音频档"
            )
        if clock_source != "system" and int(mix["system_frames"]) == mix_total:
            raise Failure("顶层 clock_source 是音频档，但每一帧都用的是 system 时钟")

    # 播放速率：媒体时间 / 墙钟时间。
    #
    # 这是唯一一个不与节拍器结论循环的检查：`av_drift` 与 `schedule_lead` 都以
    # 节拍器自己的目标时刻为基准，节拍器整体跑偏时它们可能依然"完美"；
    # 这个比值只看墙钟，跑偏就露出来。
    #
    # 刻意不拿"音频时钟跨度 ÷ 视频 pts 跨度"来比：音频会话比视频多活一段
    # （排空 + 轮询间隔），那个差值量的是收尾耗时，不是分叉。
    timeline = data["timeline"]
    video_span = int(timeline["video_pts_span_ns"])
    wall_span = int(timeline["present_wall_span_ns"])
    correction = int(timeline.get("lead_correction_ns", 0))
    corrected = int(timeline.get("corrected_wall_span_ns", wall_span + correction))
    if corrected != wall_span + correction:
        raise Failure(
            f"corrected_wall_span_ns({corrected}) != present_wall_span_ns({wall_span}) "
            f"+ lead_correction_ns({correction})"
        )
    if video_span > 0 and corrected > 0:
        rate = video_span / float(corrected)
        claimed = float(timeline["playback_rate"])
        if abs(rate - claimed) > 1e-6:
            raise Failure(f"playback_rate 对不上：证据写 {claimed:.6f}，重算得 {rate:.6f}")
        if not 0.98 <= rate <= 1.02:
            raise Failure(
                f"播放速率 {rate:.4f} 偏离实时（媒体 {video_span} ns / 修正墙钟 {corrected} ns），"
                f"超出 [0.98, 1.02]"
            )

    if data["completed"] and data["error"]:
        raise Failure("completed=true 却带着非空 error")

    summary = (
        f"{path.name}: OK  frames={rendered} dropped={dropped} "
        f"clock={clock_source} fallback={data['timestamp_fallback_count']} "
        f"xrun={data['xrun_count']}"
    )
    if measurable:
        summary += (
            f" | av_drift p50/p95/p99/max = "
            f"{drift['p50']}/{drift['p95']}/{drift['p99']}/{drift['max']} ns"
        )
    else:
        summary += " | av_drift 不可测量（理由已记录）"
    return summary


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
