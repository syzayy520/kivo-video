#!/usr/bin/env python3
"""独立复算 A/V 合并证据（`kivo.p12c.sync/1` … `/32`）。

这个脚本存在的唯一理由，是让"可复算"不只是一句口号。

证据里除了 p50/p95/p99/max，还带**完整分布**（溢出计数 + 精确极值 + 非空桶计数
+ 桶映射的三个常量）。于是任何人——包括一个不相信这份证据的人——都能从分布把
分位数重算一遍，看看是不是同一个数。只给汇总值的话，读者唯一的选择是相信。

重算规则与 `backend/src/video/video_plane/core/pace/pace_histogram.cxx` 的
`SignedHistogram::percentile_ns()` 逐行对应：秩取上整、桶心取值、结果钳进
[min, max] 之间。任何一处不一致，本脚本都会**按整数逐位比对**并报错——
这里不做"容差内就算过"，因为误差为 0 才是这套设计的意义。

判的东西分两类，结局的种类也不同：

  · **账目**（四项闭合、分布复算、钟分档、名义跨度恒等式）：只有过与不过。
  · **节拍**（`media_rate` 有没有偏离实时）：有第三种结局「不可判」——
    报告里没存实测 pts 跨度时，这个命题没有证据可谈。**不可判不等于通过**，
    主程序把它单独数出来。

P14-S 之前，"播放速率"的分子是 `(帧数 − 1) × 名义帧时长`，而它被叫做
`video_pts_span_ns`。于是 `/1`–`/29` 的全部 216 份证据里，那条判据看上去在判
"媒体时间 ÷ 墙钟"，实际判的是"帧数换成的秒 ÷ 墙钟"。这一版把两者分开：
`nominal_rate` 只验恒等式，`media_rate` 才吃区间。

用法：
    python tools/verify_p12c_sync.py docs/evidence/p12c/p12c8-av-r1-sync.json
    python tools/verify_p12c_sync.py docs/evidence/p1*/*-sync.json
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


#: 本脚本判得动的版本范围。
#:
#: `/1` 是它出生的那一版，`/30` 是 P14-S 改名的这一版。中间**全部**在范围内：
#: P14-R 只放进 `/29`，让 `/2`–`/28` 的 187 份证据在"四项闭合"上没有人判；
#: P14-S 放开之后才发现 15 份没过速率检查 —— 那些数字一直在那儿，只是没人看。
#:
#: `/2` **从未存在过**（`/1` 直接跳到 `/3`），所以它不是"漏了"，是当时就没写。
MIN_SCHEMA_VERSION = 1
MAX_SCHEMA_VERSION = 38
ABSENT_SCHEMA_VERSIONS = frozenset({2})

#: 从这一版起，`pace.pts_span_ns`（实测首末 pts 差）与 `pace.clock_supplied_frames`
#: 出现在报告里（P13-R，为四跨度恒等式加的）。**这才是能当速率分子的那个跨度。**
SCHEMA_WITH_MEASURED_SPAN = 16
#: 从这一版起 `end_reason` 在顶层（P14-R）。
SCHEMA_WITH_END_REASON = 29
#: 从这一版起 `timeline` 改名并自带 `media_rate`（P14-S）。
SCHEMA_WITH_MEDIA_RATE = 30
#: 从这一版起 `pace.clock_lead_ns` 在场（P14-T）。它一直是可算的，
#: 但从这一版起"可算"升级成"必须印出来并被判"。
SCHEMA_WITH_CLOCK_LEAD = 31
#: 从这一版起 `pace.clock_lead_trace` 与 `av_drift.vs_wall` 在场（P14-U）。
#: 两条判据跟着它走：U 组只在 `/32` 起才有东西可判（更早的版本没有这两个块），
#: 而 `clock_lead_ns` 的判据在 `/31` 起就一直在判 —— 两者别混成一个门。
SCHEMA_WITH_REFERENCE_SHIFT = 32
#: 从这一版起 `drift_gate` 带 `limit_ppm_equivalent*` 与 `limit_reach_s*`（P14-V）。
#:
#: 门限本身**一个字没动**（还是 45 ms）—— 这一版只是把"45 ms 在这一次会话里
#: 等价于多少 ppm、按本轮速率多久撞门"算出来印上。所以 `/32` 及更早的证据
#: 缺这两对字段是**历史事实**，不是缺陷，不补。
SCHEMA_WITH_GATE_SCALE = 33
#: 从 `/34` 起，门**动过**的一轮不许再给"多久撞门"（P14-W）。
#:
#: 原因：门一旦开始修，`media_rate` 就从"输入"变成"修完之后剩下的"。
#: 实测 60 s / 5 000 ppm：门第 9 秒动手、共 49 次，而按净速率算出的
#: `limit_reach_s` 是 63.6 s —— 一个与事实相反的数。所以那一格必须判"不可算"，
#: 并给出专门的理由（`kGateActedReason`），不能只印个大数让人自己体会。
SCHEMA_WITH_GATE_ACTED = 34
#: 从 `/35` 起顶层有 `drift_rebase_trace`（P14-X）：每一笔复位的时刻与幅度。
SCHEMA_WITH_REBASE_TRACE = 35
#: 从 `/36` 起顶层有 `drift_discontinuity`（P14-Y）：钟的不连续那本账 ——
#: 每一笔台阶的墙钟增量、原始位置增量、以及它对领先量的贡献。
SCHEMA_WITH_DISCONTINUITY = 36
#: 从 `/37` 起 `drift_gate.step_repay` 出版（P14-Z）：台阶还掉还是不还。
#: 两臂的"零修正"读起来一样、意思相反，不写清楚就分不开。
SCHEMA_WITH_STEP_REPAY = 37
#: 从 `/38` 起跨度有"是谁喂的"这一格（P14-AA）：presented 还是 scheduled。
#: 以前 at_time 档压根不喂 DueStep，四跨度恒 0、`media_rate` 无从算。
SCHEMA_WITH_SPAN_BASIS = 38

#: 已知越界 —— **不是豁免判据，是登记在案的异常**。值是 `(说明, 钟领先量指纹)`。
#:
#: 每一条都必须说清三件事：哪一份、越多少、谁记的。并且有两条反向判据：
#: **表里任何一份不再越界，本脚本就红**；**钟领先量变了（±1 帧），也红** ——
#: 后者是 P14-T 加的：这一版的归因就是"钟领先量"，于是豁免必须带着它的证据
#: 留在身上，而不是一句"已登记"就能盖住任何变化。
KNOWN_OUT_OF_BAND = {
    "p13r/p13r-4k60-r3-sync.json": (
        "P14-S 记录：4K60 8s，media_rate=1.0565（丢 37 帧，墙钟短 427 ms）", 426_703_545),
    "p13ad/p13ad-rate50-off-r1-sync.json": (
        "P14-S 记录：4K60 8s，media_rate=1.0434（丢 0 帧，墙钟短 332 ms）", 331_771_991),
    "p13s/p13s-4k60-r1-sync.json": (
        "P14-S 记录：4K60 8s，media_rate=1.0320（丢 18 帧，墙钟短 248 ms）", 247_686_616),
    "p13v/p13v-8x-r2-sync.json": (
        "P14-S 记录：4K60 8s，media_rate=1.0212（丢 7 帧，墙钟短 166 ms）", 165_579_639),
    # P14-V 的活体标本，**不是**历史遗留问题：
    # 人为把钟一步推 400 ms（`kivo.clock_offset_ms 400`），于是"媒体时间 / 墙钟"
    # 必然偏大 5.27%。同一份证据里 `av_drift.p50` 仍是 125 µs ——
    # 那个 125 µs 就是"相对钟"这个参考系的全部产出，而实际差 400 ms（`vs_wall.last`）。
    # 登记它，是因为它的速率越界是**实验设计**，不是实现缺陷。
    "p14u/p14u-step-400ms-sync.json": (
        "P14-V 标本：钟阶跃 +400 ms，media_rate=1.0527（墙钟短 399.9 ms，p50 仍 125 µs）",
        400_015_731),
}


def evidence_key(path: Path) -> str:
    """以 `docs/evidence/` 为根的相对路径，作为登记表的键。

    **不能用文件名**：`p14g/p14g-long1800-r1-sync.json` 与
    `p14g/pre/p14g-long1800-r1-sync.json` 同名。P14-S 第一版用基名做键，
    216 份跑出来只有 215 条记录 —— 少的那一份被同名的另一份顶掉了，
    而汇总行照样打印"合计 215 份"。判据在数自己的样本数时丢样本，
    这种错只能靠"键唯一"从根上避免。
    """
    parts = path.parts
    if "evidence" in parts:
        index = len(parts) - 1 - parts[::-1].index("evidence")
        return "/".join(parts[index + 1:])
    return path.name

#: 登记表里的"钟领先量"指纹的容差。见 `KNOWN_OUT_OF_BAND` 的用法：
#: 它不是判据线（那是 `[0.98, 1.02]` 与 `T2`），是"这份证据有没有被换过"的指纹。
FINGERPRINT_TOLERANCE_NS = 1_000_000

RATE_LOW = 0.98
RATE_HIGH = 1.02


def schema_version(schema: Any) -> int:
    """把 `kivo.p12c.sync/N` 解成 N，并把三种坏法分开报。

    合在一起报会让"格式变了"和"版本不在范围里"长成同一句话，而修法完全不同。
    """
    prefix = "kivo.p12c.sync/"
    if not isinstance(schema, str) or not schema.startswith(prefix):
        raise Failure(f"schema 是 {schema!r}，不是 {prefix}N 的形状")
    tail = schema[len(prefix):]
    if not tail.isdigit():
        raise Failure(f"schema 是 {schema!r}，版本号不是十进制整数")
    version = int(tail)
    if version in ABSENT_SCHEMA_VERSIONS:
        raise Failure(f"schema 是 {schema!r}，但 /{version} 这一版从未存在过（/1 直接跳到 /3）")
    if not MIN_SCHEMA_VERSION <= version <= MAX_SCHEMA_VERSION:
        raise Failure(f"schema 是 {schema!r}，超出已知范围 "
                      f"/{MIN_SCHEMA_VERSION}–/{MAX_SCHEMA_VERSION}")
    return version


def nominal_span_of(timeline: dict[str, Any], version: int) -> int:
    """「交出去的帧数 × 名义帧时长」——**不是**媒体时间，只是它的旧名字这么暗示。

    `/30` 起叫 `nominal_span_ns`。同一份里**不允许**再出现旧名：
    两个名字指同一个数，就是"同一笔账记两处"，早晚不一致。
    """
    if version >= SCHEMA_WITH_MEDIA_RATE:
        if "video_pts_span_ns" in timeline:
            raise Failure(
                f"/{version} 的报告里还留着 `timeline.video_pts_span_ns` 这个旧名。"
                f"那一版它已经改叫 `nominal_span_ns` —— 两个都在，读者没法知道该信哪个"
            )
        return int(timeline["nominal_span_ns"])
    return int(timeline["video_pts_span_ns"])


def nominal_rate_of(timeline: dict[str, Any], version: int) -> float:
    if version >= SCHEMA_WITH_MEDIA_RATE:
        if "playback_rate" in timeline:
            raise Failure(
                f"/{version} 的报告里还留着 `timeline.playback_rate` 这个旧名"
                f"（它已改叫 `nominal_rate`）：旧名字声称是媒体时间，而分子是帧数"
            )
        return float(timeline["nominal_rate"])
    return float(timeline["playback_rate"])


def media_span_of(data: dict[str, Any], version: int) -> tuple[int, int, str]:
    """实测 pts 跨度与它覆盖的帧数。第三个返回值非空 = 这一份判不了，理由在此。

    **拿不到时不许退回名义跨度**。退回就等于把"没测到"印成"测到了"，
    而这两件事在 P14-L / P14-R 里各撞过一次。
    """
    timeline = data["timeline"]
    pace = data["pace"]
    if version >= SCHEMA_WITH_MEDIA_RATE:
        frames = int(timeline["media_span_frames"])
        span = int(timeline["media_pts_span_ns"])
        if not bool(timeline["media_rate_measured"]):
            reason = str(timeline.get("media_rate_reason") or "").strip()
            if not reason:
                # 「带理由的缺席」是这个项目的老规矩（`av_drift` 先立的那条）：
                # 不可测可以，但**必须说清缺哪一格**。一声不响的 false 与
                # 一声不响的 0 是同一种病 —— 读者无从判断该不该信。
                raise Failure(
                    "media_rate_measured=false 却没给 media_rate_reason："
                    "不可测可以，但要说清是哪一格缺"
                )
            return span, frames, reason
        return span, frames, ""
    if version >= SCHEMA_WITH_MEASURED_SPAN:
        frames = int(pace.get("clock_supplied_frames", 0))
        span = int(pace.get("pts_span_ns", 0))
        if frames < 2:
            return span, frames, (f"只有 {frames} 帧带钟读数（`/16`–`/29` 的 "
                                  f"`pace.clock_supplied_frames`）：跨度不存在")
        return span, frames, ""
    return 0, 0, (f"/{version} 没有实测 pts 跨度（P13-R 之前没有这一格）："
                  f"报告里只有名义跨度，补不出来")


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


def distribution_blocks(drift: dict[str, Any], version: int) -> list[tuple[str, dict[str, Any]]]:
    """该复算哪几份分布，以及各自叫什么。

    `/1`：分位数挂在 `av_drift` 顶层，分布也在那里（`av_drift.histogram`）。
    `/3` 起：分位数收进口径块，分布跟着搬进 `quantiles.<口径>.histogram` ——
    于是 `/29` 上"`av_drift` 缺 histogram"不是证据坏了，是**这个脚本还在读旧位置**。

    **每一份都复算**，不只挑一份：口径块里的三格范围（P14-O/P）就是从各自那份分布
    算出来的，而两份分布若有一份被手改，另一份看不出来。
    """
    if version < 3:
        return [("av_drift", drift)]
    quantiles = drift.get("quantiles")
    if not isinstance(quantiles, dict) or not quantiles:
        raise Failure(f"av_drift.quantiles 缺失或为空：可测的 /{version} 证据必须至少有一份口径块")
    return [(f"av_drift.quantiles.{mode}", block)
            for mode, block in quantiles.items() if isinstance(block, dict)]


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


# 超过这个偏移就不可能是频差（与 `tools/audit_clock_lead_origin.py` 同一个常数，
# 推导在那边：消费级晶振 ±50 ppm，留 40 倍余量）。
LEAD_PPM_UPPER = 2000.0


def check_reference_shift(
    data: dict[str, Any],
    pace: dict[str, Any],
    clock_lead: int,
    frame_duration: int,
    notes: list[str],
) -> None:
    """U 组：报告有没有把「第二个参考系」说对（P14-U，`/32` 起）。

    五条判据全部是**逐位**关系 —— 它们判的是同一批数在两三个地方的一致性，
    不是测量精度，所以不给容差。给容差的判据会慢慢变成"差一点点也算对"，
    而这个项目已经栽过两次（`/26` 的裸 p99、`/30` 的分子换错）。

    U1 之所以能要求逐位：`P14-U` 把四个跨度的基准帧挪进了"有钟读数"的分支，
    于是 `clock_lead_ns` 与 `clock_lead_trace.last_ns` 用的是**同一个首帧、
    同一个末帧**。它们在 `/31` 上可能差一点（基准帧可能不同），`/32` 起不可能。
    """
    trace = pace.get("clock_lead_trace")
    if not isinstance(trace, dict):
        raise Failure("/32 起 pace.clock_lead_trace 必须存在")
    wall = data["av_drift"].get("vs_wall")
    if not isinstance(wall, dict):
        raise Failure("/32 起 av_drift.vs_wall 必须存在")

    supplied = int(pace.get("clock_supplied_frames", 0))
    samples = int(trace.get("samples", 0))
    verdict = str(trace.get("verdict", ""))
    trace_measured = bool(trace.get("measured", False))
    step_frames = int(trace.get("step_frames", 0))
    max_step = int(trace.get("max_step_ns", 0))
    last_lead = int(trace.get("last_ns", 0))
    rate_ppm = float(trace.get("rate_ppm", 0.0))

    # U0 接线：有钟读数的帧数必须与喂进轨迹的帧数相等。两者一个是分支计数、
    # 一个是记录器自数，路径不同 —— 只有它们数的是同一批帧才会相等。
    if samples != supplied:
        raise Failure(
            f"U0 接线断了：clock_supplied_frames={supplied} 但 clock_lead_trace.samples="
            f"{samples}。两者必须相等（同一个分支喂的）"
        )

    if supplied == 0:
        # U3：没读数时三件事必须同时成立 —— 它们说的是同一句话。
        if verdict != "unknown":
            raise Failure(f"U3 没有钟读数，verdict 却是 {verdict!r}（必须是 unknown）")
        if trace_measured:
            raise Failure("U3 没有钟读数，clock_lead_trace.measured 却是 true")
        if bool(wall.get("measurable", False)):
            raise Failure("U3 没有钟读数，vs_wall.measurable 却是 true")
        return

    # U1：两支尺子同源。一个用 arrive 首末、一个用 present 首末，`/32` 起基准帧相同。
    if last_lead != clock_lead:
        raise Failure(
            f"U1 两支尺子不同源：clock_lead_trace.last_ns={last_lead} 而 "
            f"clock_lead_ns={clock_lead}（差 {last_lead - clock_lead} ns）。"
            f"`/32` 起两者必须逐位相等 —— 不等说明基准帧或末帧不是同一帧"
        )

    # U2：`vs_wall.last == last_lateness − last_lead`。这是定义式，不是近似。
    expected = int(pace["last_lateness_ns"]) - last_lead
    actual = int(wall.get("last_ns", 0))
    if actual != expected:
        raise Failure(
            f"U2 vs_wall.last_ns={actual} 而 last_lateness_ns({pace['last_lateness_ns']}) "
            f"− clock_lead_trace.last_ns({last_lead}) = {expected}（差 {actual - expected} ns）。"
            f"定义式不成立；若差一帧量级，先查最后一帧有没有钟读数"
        )

    # U4：超过物理上限的偏移不许被归成"速率"。
    #
    # **但注入器能造出超限速率。** P14-V 的真机标本 `p14u-ramp-5000ppm` 就是
    # 人为注入 5 000 ppm，`clock_lead_trace` 算出 3 742 ppm —— 两个数都对，
    # 口径不同：注入只覆盖第 2 s 之后的那 6 s，而 `rate_ppm` 摊在整段 8 s 上，
    # 所以整段平均必然**小于**注入的瞬时值。这不是"把跳变认成速率"，
    # 是判据不知道输入被换掉了。
    #
    # 所以上限换成注入值：它给的是一个天花板（整段平均不可能超过注入瞬时值），
    # 留 5% 余量给量化误差与首帧落点。没注入的轮次照旧用 `LEAD_PPM_UPPER`。
    injected_rate_ppm = abs(int((data.get("clock_injection") or {}).get("rate_ppm", 0)))
    lead_upper = LEAD_PPM_UPPER if injected_rate_ppm == 0 else injected_rate_ppm * 1.05
    if abs(rate_ppm) > lead_upper and verdict == "ramped":
        raise Failure(
            f"U4 {rate_ppm:.0f} ppm 超过 {lead_upper:.0f} ppm 上限"
            f"{'' if injected_rate_ppm == 0 else f'（注入 {injected_rate_ppm} ppm）'}，"
            f"却被判成 ramped —— 消费级晶振到不了这个量级，这一定是位置跳变"
        )

    # U5：`stepped` 与"确有一步步超一帧"互为充要。
    if verdict == "stepped":
        if step_frames < 1 or max_step <= frame_duration:
            raise Failure(
                f"U5 verdict=stepped 但 step_frames={step_frames}、max_step_ns={max_step}"
                f"（帧长 {frame_duration}）—— 没有一步超一帧，凭什么说它跳了"
            )
    elif step_frames != 0:
        raise Failure(
            f"U5 有 {step_frames} 步超过一帧，verdict 却是 {verdict!r}（必须是 stepped）"
        )

    notes.append(
        f"参考系：相对钟 p50 见上；相对墙 last={actual} ns、worst={wall.get('worst_ns')} ns"
        f"（上界 {wall.get('bound_ns')} ns）；领先量 {last_lead} ns（{rate_ppm:.0f} ppm，"
        f"{verdict}）"
    )


def check_gate_scale(
    data: dict[str, Any],
    pace: dict[str, Any],
    notes: list[str],
) -> None:
    """V 组：门限的量纲（P14-V，`/33` 起）。

    判的是「报告有没有把这个换算算对」，**不判「45 ms 合不合适」**。
    后者是调参：`pace_policy.hpp` 第 11 行写明必须另开卡并给对照数据。

    为什么值得一组判据：`limit_ns` 恒定 45 000 000 出现在每一份里（65 份全同），
    读起来像"容差恒定"。而它守的是累积漂移 = 速率误差 × 时长，
    于是同一个 45 ms 在 6.3 s 上等价 7 138 ppm、在 1 800 s 上等价 25 ppm。
    这两格数并排在文件里几十年也不会有人去比 —— 除非报告自己算出来。
    """
    gate = data.get("drift_gate")
    if not isinstance(gate, dict):
        raise Failure("/33 起 drift_gate 必须存在（换算就住在这一块里）")

    version = schema_version(data.get("schema"))
    rebases = int((pace.get("clock_health") or {}).get("drift_rebases", 0))
    if "limit_ppm_equivalent" not in gate:
        raise Failure(
            "/33 起 drift_gate.limit_ppm_equivalent 必须存在 —— 这一版的主题就是它"
        )

    # V0：可测性两格必须在场。缺了会被 `.get(..., False)` 悄悄当成"没量到"放过，
    # 于是"漏接"和"真的算不出来"在判据眼里长得一样。
    for key in ("limit_ppm_equivalent_measured", "limit_reach_s_measured"):
        if key not in gate:
            raise Failure(f"/33 起 drift_gate.{key} 必须存在（与值成对出版）")

    limit_ns = int(gate["limit_ns"])
    span_ns = int(pace.get("elapsed_span_ns", 0))
    ppm = float(gate["limit_ppm_equivalent"])
    ppm_measured = bool(gate["limit_ppm_equivalent_measured"])
    ppm_reason = str(gate.get("limit_ppm_equivalent_reason", ""))
    reach_s = float(gate.get("limit_reach_s", 0.0))
    reach_measured = bool(gate["limit_reach_s_measured"])
    reach_reason = str(gate.get("limit_reach_s_reason", ""))

    # V2：可算与不可算是**充要**关系。只判一侧不够 ——
    # 只判"可算时值对不对"会放过"该可算却报了 0"，而那个 0 读作
    # "门严到一点点速率误差都不许有"，正好是最容易被误读的假观测。
    should_have_ppm = span_ns > 0 and limit_ns > 0
    if should_have_ppm != ppm_measured:
        raise Failure(
            f"V2 等价速率的可测性与输入矛盾：span_ns={span_ns}、limit_ns={limit_ns} "
            f"⇒ 应{'可算' if should_have_ppm else '不可算'}，但 measured={ppm_measured}"
        )
    if should_have_ppm:
        if ppm <= 0.0:
            raise Failure(
                f"V2 span_ns={span_ns} 却算出 limit_ppm_equivalent={ppm}（必须 > 0）"
            )
        if ppm_reason:
            raise Failure(f"V2 算出来了却带着理由 {ppm_reason!r}（两者互斥）")
    else:
        if ppm != 0.0:
            raise Failure(f"V2 不可算时 limit_ppm_equivalent 必须是 0，实际 {ppm}")
        if not ppm_reason:
            raise Failure("V2 不可算却没给理由")

    # V1：定义式。一次除法，相对容差 1e-7（报告印 6 位小数，这是那个精度）。
    if should_have_ppm:
        expected = limit_ns * 1e6 / span_ns
        if abs(ppm - expected) > abs(expected) * 1e-7 + 1e-9:
            raise Failure(
                f"V1 limit_ppm_equivalent={ppm} ≠ limit_ns({limit_ns}) × 1e6 / "
                f"span_ns({span_ns}) = {expected}"
            )

    # V3/V4：撞门时间。
    if reach_measured:
        timeline = data.get("timeline") or {}
        if not timeline.get("media_rate_measured"):
            raise Failure(
                "V3 limit_reach_s 声称算出来了，但 timeline.media_rate_measured=false "
                "—— 没有速率偏差，撞门时间无从谈起"
            )
        deviation = abs(float(timeline["media_rate"]) - 1.0)
        if deviation <= 0.0:
            raise Failure(
                f"V3 limit_reach_s 声称算出来了，但 media_rate={timeline['media_rate']} "
                f"与 1 没有偏差"
            )
        back = reach_s * deviation * 1e9
        # 容差能这么严，是因为 `media_rate` 与 `limit_reach_s` **同源**：
        # 报告先把它量化到 9 位再用它算 reach（`published_rate`）。
        # 上一版没有这一步，6 位小数的舍入被倒数放大成 12%，
        # 于是"从印出的数复算 reach"根本解不回去 —— 那不是判据该放宽，
        # 是证据自己没打算被复算。
        if abs(back - limit_ns) > limit_ns * 1e-8:
            raise Failure(
                f"V3 limit_reach_s({reach_s}) × |media_rate−1|({deviation}) × 1e9 = "
                f"{back} ns，解不回 limit_ns={limit_ns}（差 {back - limit_ns} ns）"
            )
        notes.append(
            f"门限量纲：45 ms 在 {span_ns / 1e9:.1f} s 窗口上等价 {ppm:.0f} ppm，"
            f"按本轮速率 {reach_s:.1f} s 撞门"
        )
    else:
        if not reach_reason:
            raise Failure("V4 limit_reach_s 不可算却没给理由")
        suffix = "（门已动作，撞门时间不可算）" if "post-correction rate" in reach_reason \
            else "（撞门时间不可算）"
        notes.append(
            f"门限量纲：45 ms 在 {span_ns / 1e9:.1f} s 窗口上等价 {ppm:.0f} ppm{suffix}"
        )

    # ── V5（`/34` 起）：门动过之后，"多久撞门"这一格该闭嘴 ──
    #
    # 放在 if/else **之外**是刻意的：塞进 else 里的话，"门动了 49 次却仍报可算"
    # 会从 if 分支溜过去 —— 而那正是 P14-W 修掉的那个缺陷本身。
    # （第一版就写错了缩进，M9 变异没打死，才发现整段被吞进了上面的 if 里。）
    if version >= SCHEMA_WITH_GATE_ACTED:
        gate_acted_reason = "post-correction rate" in reach_reason
        if rebases > 0 and reach_measured:
            raise Failure(
                f"V5 门已复位 {rebases} 次，却仍给出 limit_reach_s={reach_s}"
                "（观测速率已被修正污染，这个数描述的是残差，不是注入的扰动）"
            )
        if rebases > 0 and not gate_acted_reason:
            raise Failure(
                f"V5 门已复位 {rebases} 次，reach 却按别的理由判不可算："
                f"{reach_reason!r}（正确理由是 kGateActedReason）"
            )
        if rebases == 0 and gate_acted_reason:
            raise Failure(
                "V5 门一次都没复位，reach 却说是'修完之后剩下的速率'（开关传反了）"
            )


def check_rebase_trace(
    data: dict[str, Any],
    pace: dict[str, Any],
    notes: list[str],
) -> None:
    """X 组：每一笔复位发生在什么时候（P14-X，`/35` 起）。

    P14-W 量到过一笔 104.71 ms 的修正（迟滞带 5 ms），三轮里只有一轮有。
    算术给出唯一自洽的解释 —— 启动期盲区里钟往前跳了一下，盲区结束时一次还清 ——
    但当时证据没有时刻，所以它停在"解释"。这一组判的就是那个解释站不站得住。
    """
    trace = data.get("drift_rebase_trace")
    if not isinstance(trace, dict):
        raise Failure("/35 起 drift_rebase_trace 必须存在")

    health = pace.get("clock_health") or {}
    rebases = int(health.get("drift_rebases", 0))
    gate = data.get("drift_gate") or {}
    band_ns = int(gate.get("band_ns", 0))
    frame_ns = int(pace.get("frame_duration_ns") or 0)

    recent = trace.get("recent")
    if not isinstance(recent, list):
        raise Failure("X1 drift_rebase_trace.recent 必须是数组（没有也要给空数组）")

    # X1：三处计数必须互相解释。`rebases` 是总数、`sampled` 是环里留下的、
    # `len(recent)` 是真正印出来的 —— 三者对不上，说明有一处漏写或重复写。
    if int(trace.get("rebases", -1)) != rebases:
        raise Failure(
            f"X1 trace.rebases={trace.get('rebases')} 与 clock_health.drift_rebases="
            f"{rebases} 不一致"
        )
    expected_sampled = min(rebases, 8)
    if int(trace.get("sampled", -1)) != expected_sampled:
        raise Failure(
            f"X1 trace.sampled={trace.get('sampled')}，按 rebases={rebases} 应为 "
            f"{expected_sampled}"
        )
    if len(recent) != expected_sampled:
        raise Failure(
            f"X1 recent 有 {len(recent)} 笔，sampled 说 {expected_sampled} 笔"
        )

    if rebases == 0:
        # X0：没复位过就必须是干净的空。有样本就是幽灵。
        if float(trace.get("max", {}).get("amount_ns", 0)) != 0 or recent:
            raise Failure("X0 一次都没复位，trace 里却有样本")
        return

    # X2：样本自身自洽 —— 时间升序、幅度为正、且不超过修之前看到的领先量。
    previous_at = -1
    for index, sample in enumerate(recent):
        at_ns = int(sample["at_ns"])
        amount_ns = int(sample["amount_ns"])
        lead_before_ns = int(sample["lead_before_ns"])
        if at_ns <= previous_at:
            raise Failure(f"X2 recent[{index}].at_ns={at_ns} 不是严格升序（上一笔 {previous_at}）")
        previous_at = at_ns
        if amount_ns <= 0:
            raise Failure(f"X2 recent[{index}].amount_ns={amount_ns} 非正")
        if amount_ns > lead_before_ns:
            # 修正只还"超出目标的那部分"，永远不可能多于当时看到的领先量。
            raise Failure(
                f"X2 recent[{index}] 修了 {amount_ns} ns，却只看到 {lead_before_ns} ns 的领先"
            )

    # X3：最大那一笔与 `clock_health` 的同一个数必须相等 —— 两处各算一遍就是两个真相。
    max_amount = int(trace["max"]["amount_ns"])
    if max_amount != int(health.get("drift_max_correction_ns", 0)):
        raise Failure(
            f"X3 trace.max.amount_ns={max_amount} 与 clock_health."
            f"drift_max_correction_ns={health.get('drift_max_correction_ns')} 不一致"
        )

    # X4（本组的意义）：单笔远大于迟滞带时，那一轮必须**有一次可查的钟事件**。
    #
    # 这条判据的第一版写的是「必须落在启动期（≤3 s）」，被 P14-X 的真机数据推翻：
    # 两轮大修正（146.75 ms @24.43 s 与 104.71 ms）都不在启动期，而它们与六轮正常轮
    # 的唯一差别是 `extrapolated_frames = 1` 且 `monotonic_rejects = 1`。
    # 于是判据改成「大修正必须伴随钟的不连续」—— 它守的是已证实的规律，
    # 而不是一个被推翻的猜测。
    oversized_ns = max_amount
    if frame_ns > 0 and oversized_ns > band_ns + frame_ns:
        health = pace.get("clock_health") or {}
        events = (int(health.get("extrapolated_frames", 0))
                  + int(health.get("monotonic_rejects", 0))
                  + int(health.get("jump_rebases", 0)))
        # P14-Y：**"钟事件"的定义在 /36 变宽了**。这一卡证明"位置台阶"也是一种钟事件
        # （`position_step` / `backward_clamped`），而它们不算在那三个计数里。
        # 不把它们算进来的话，P14-Y 自己的正向对照（注入 +120 ms 台阶）会被 X4 判红 ——
        # 判据把"有事件"说成"没事件"，这比漏判更糟。
        # 更细的一层仍由 Y3 守：大修正必须有一笔**够大**的 step 解释，不只是"有事件"。
        disc = data.get("drift_discontinuity") or {}
        events += int(disc.get("stepped", 0)) + int(disc.get("clamped", 0))
        if events == 0:
            raise Failure(
                f"X4 单笔修正 {oversized_ns / 1e6:.2f} ms"
                f"（迟滞带 {band_ns / 1e6:.1f} ms + 一帧）发生在第 "
                f"{int(trace['max']['at_ns']) / 1e9:.2f} s，"
                "但这一轮的 extrapolated_frames / monotonic_rejects / jump_rebases 全是 0"
                " —— 大修正必须有可查的钟事件，否则就是门自己在造偏差"
            )
    notes.append(
        f"复位轨迹：共 {rebases} 笔，最大 {max_amount / 1e6:.2f} ms 落在 "
        f"{int(trace['max']['at_ns']) / 1e9:.2f} s"
    )


def check_discontinuity(data: dict, pace: dict, notes: list[str]) -> None:
    """Y 组：钟的不连续那本账（P14-Y，`/36` 起）。

    X 组证明了"大修正与钟事件同轮出现"，但只能解释约一帧；146.75 ms 里剩下的
    113 ms 当时没有证据 —— 因为轨迹只记了**修正**，没记那一刻的原始位置与墙钟。
    这一组判的就是那本账本身对不对：它记的每一笔，`Σstep` 必须等于 `offset`，
    分类计数必须等于 `events`，而"大修正"必须能在同一轮里找到一笔够大的台阶。
    """
    block = data.get("drift_discontinuity")
    if not isinstance(block, dict):
        raise Failure("/36 起 drift_discontinuity 必须存在")

    events = int(block.get("events", -1))
    offset_ns = int(block.get("offset_ns", 0))
    sampled = int(block.get("sampled", -1))
    sampled_offset_ns = int(block.get("sampled_offset_ns", 0))
    recent = block.get("recent")
    if not isinstance(recent, list):
        raise Failure("Y0 drift_discontinuity.recent 必须是数组（没有也要给空数组）")

    by_kind = {
        "extrapolated": int(block.get("extrapolated", 0)),
        "monotonic_rejected": int(block.get("rejected", 0)),
        "backward_clamped": int(block.get("clamped", 0)),
        "jump_rebased": int(block.get("jumped", 0)),
        "position_step": int(block.get("stepped", 0)),
    }

    # Y0：一次事件都没有 ⇒ 干净的空。有样本就是幽灵账。
    if events == 0:
        if offset_ns != 0 or sampled != 0 or recent or any(by_kind.values()):
            raise Failure("Y0 一次事件都没有，账里却有东西")
        notes.append("不连续账：零事件")
        return

    # Y2：分类计数必须解释 `events`，环的长度必须解释 `sampled`。
    if events != sum(by_kind.values()):
        raise Failure(
            f"Y2 events={events} 与五个分类之和 {sum(by_kind.values())} 不一致：{by_kind}"
        )
    expected_sampled = min(events, 8)
    if sampled != expected_sampled or len(recent) != expected_sampled:
        raise Failure(
            f"Y2 sampled={sampled}、recent={len(recent)} 笔，按 events={events} 应为 "
            f"{expected_sampled}"
        )

    # Y1：环内 `Σstep` 必须等于 `sampled_offset_ns`；事件不超过环长时，两者都等于总账。
    ring_sum = 0
    previous_at = -1
    for index, sample in enumerate(recent):
        at_ns = int(sample["at_ns"])
        step_ns = int(sample["step_ns"])
        if at_ns <= previous_at:
            raise Failure(f"Y2 recent[{index}].at_ns={at_ns} 不是严格升序（上一笔 {previous_at}）")
        previous_at = at_ns
        if sample.get("kind") not in by_kind:
            raise Failure(f"Y2 recent[{index}].kind={sample.get('kind')!r} 不在白名单里")
        ring_sum += step_ns
    if ring_sum != sampled_offset_ns:
        raise Failure(
            f"Y1 环内 Σstep={ring_sum} 与 sampled_offset_ns={sampled_offset_ns} 不一致"
            " —— 环回绕时没把被挤走那一笔扣回来"
        )
    if events <= 8 and sampled_offset_ns != offset_ns:
        raise Failure(
            f"Y1 events={events} ≤ 8，环内累计 {sampled_offset_ns} 必须等于总账 {offset_ns}"
        )

    # Y4：`max` 必须是环里 |step| 最大的那一笔。不更新它，读者会盯着一个小的。
    max_step = int(block["max"]["step_ns"])
    worst = max((abs(int(s["step_ns"])) for s in recent), default=0)
    if abs(max_step) != worst:
        raise Failure(
            f"Y4 max.step_ns={max_step}，但环里绝对值最大的是 {worst}"
            " —— max 没跟着更新"
        )

    # Y3（本组的意义）：单笔大修正必须能被同一轮的一笔台阶解释。
    #
    # 阈值照旧从证据里读（band + 一帧），不硬编码；"解释"定义为
    # `|step| ≥ 修正 − (带 + 帧)`，即扣掉正常波动后剩下的量必须有出处。
    gate = data.get("drift_gate") or {}
    band_ns = int(gate.get("band_ns", 0))
    frame_ns = int(pace.get("frame_duration_ns") or 0)
    health = pace.get("clock_health") or {}
    correction_ns = int(health.get("drift_max_correction_ns", 0))
    if frame_ns > 0 and correction_ns > band_ns + frame_ns:
        need_ns = correction_ns - (band_ns + frame_ns)
        worst_step = max((abs(int(s["step_ns"])) for s in recent), default=0)
        if worst_step < need_ns:
            raise Failure(
                f"Y3 单笔修正 {correction_ns / 1e6:.2f} ms，扣掉迟滞带 + 一帧后仍有 "
                f"{need_ns / 1e6:.2f} ms 没有出处；环里最大台阶只有 "
                f"{worst_step / 1e6:.2f} ms"
            )

    notes.append(
        f"不连续账：{events} 笔，累计 {offset_ns / 1e6:.2f} ms，"
        f"最大 {max_step / 1e6:.2f} ms（{block['max'].get('kind')}）"
    )


def verify(path: Path) -> tuple[str, list[str], str]:
    """判一份证据。返回 `(摘要, 说明, 速率结局)`。

    速率结局只有三种：`judged`（判得动，且在带内）/ `waived`（判得动，越界但已登记）
    / `unjudged`（判不动，理由在说明里）。**`unjudged` 不等于通过**，主程序单独数它。

    其余所有判据（四项闭合、分布复算、钟分档、恒等式）都只有两种结局：过，或者抛
    `Failure`。它们对自己要判的东西没有"看不清"这一说。
    """
    notes: list[str] = []
    data = json.loads(path.read_text(encoding="utf-8"))
    version = schema_version(data.get("schema"))

    missing = [key for key in REQUIRED_TOP_LEVEL if key not in data]
    if missing:
        raise Failure(f"缺必含字段：{missing}")

    # `av_drift` 只有在能被测量时才该带分布；不可测量时必须是"带理由的缺席"，
    # 而不是一堆 0 —— 一堆 0 会被读成"零偏差"，那是最坏的一种假证据。
    drift = data["av_drift"]
    if "measurable" in drift:
        measurable = bool(drift["measurable"])
    else:
        # `/1`–`/2` 没有这一格（它是后来才加的）。**不能按 False 默认**：
        # `p12c/superseded/p12c8-av-r1-sync.json` 有 180 个样本、带完整分布，
        # 却因为缺这一格被判成"不可测量却没说理由" —— 于是
        # "字段还没出生"和"实现偷懒没填"长成同一句话，而修法完全不同。
        #
        # 按"有没有样本"推断，并把推断本身说出来（证据要能自证，不能默默补一个默认值）。
        measurable = int(drift.get("samples", 0)) != 0
        notes.append(
            f"av_drift.measurable 缺格（/{version} 还没有这一格），按 samples="
            f"{drift.get('samples', 0)} 推断为{'可测' if measurable else '不可测'}"
        )
    # 收尾摘要要报一份分布 —— 登记第一份被复算过的，而不是回头去顶层摸 `drift['p50']`
    # （`/3` 起那一格搬进了口径块，摸不到会抛 KeyError，而这本该只是一行摘要）。
    primary_block: tuple[str, dict[str, Any]] | None = None
    if measurable:
        for where, block in distribution_blocks(drift, version):
            check_distribution(where, block)
            if primary_block is None:
                primary_block = (where, block)
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
    #
    # P14-S：缺这一格时**要说出来**，不能默默按 0 —— 那是 P14-R 在 `S2` 上刚修过
    # 的同一个错（"缺格会被下游读成 0"）。`/1` 的早期五份（`p12c8-av-*`）真的没有
    # 这一格：字段是 `/1` 生命周期中途才加的。按 0 判是对的，但不能不说。
    if "frames_discarded_no_surface" in video:
        discarded = int(video["frames_discarded_no_surface"])
    else:
        discarded = 0
        notes.append(
            f"账目：`video.frames_discarded_no_surface` 缺格（/{version} 的这一份里没有），"
            f"四项闭合按 0 判 —— 它当时的闭合差本身就是 0，所以这个 0 不是假设出来的"
        )
    if released != received:
        raise Failure(f"frames_released={released} != frames_received={received}（有输出槽没归还）")

    # ── P14-R：EOS 那一帧只在"自然结束"时存在 ──
    #
    # `frames_received` 把末尾那个 EOS 缓冲也算进去了（它不是画面、不进节拍，
    # 但占一个输出槽，必须 release），所以闭合式多一个 `+1`。
    # **被打断的会话根本没走到 EOS** —— 那个 `+1` 不存在。
    #
    # 于是这条式子必须知道"这一轮是怎么结束的"。`end_reason` 就是那个答案：
    # 只有 `completed` 才有 EOS 帧，其余（`interrupted` / `failed`）没有。
    #
    # 为什么不用 `completed` 布尔值直接判：`completed` 与 `error` 之间还有一格
    # （停了、没报错、也没跑完），那一格的 `completed` 是 false —— 数值上碰巧对，
    # 但读代码的人会以为"没跑完所以减 1"，而真实规则是"**没有 EOS 帧**所以减 1"。
    # 两句话在真机上通常同真同假，在 EOS 之后才被打断的那一轮里就会分叉。
    end_reason = data.get("end_reason")
    if version >= SCHEMA_WITH_END_REASON:
        if end_reason not in ("completed", "interrupted", "failed"):
            raise Failure(f"end_reason={end_reason!r} 不在三值里"
                          f"（/{version} 必须写这一格）")
    else:
        # `/1`–`/28` 没有这一格。按 `completed` 推出等价结论，
        # 而不是默认 `+1` —— 默认会让一份这一区间的中断证据悄悄少算一帧。
        end_reason = "completed" if data.get("completed") is True else "interrupted"
    eos_frame = 1 if end_reason == "completed" else 0

    if rendered + dropped + discarded + eos_frame != received:
        raise Failure(
            f"frames_rendered({rendered}) + frames_dropped_late({dropped}) + "
            f"frames_discarded_no_surface({discarded}) + {eos_frame}"
            f"（end_reason={end_reason!r}） != "
            f"frames_received({received})"
            + ("（末尾 EOS 缓冲没算对）" if eos_frame
               else "（这一轮没有 EOS 帧，所以不加 1；差多少就是多少帧没记账）")
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

    # ── P14-S：两条跨度，只有一条是时间 ──
    timeline = data["timeline"]
    nominal_span = nominal_span_of(timeline, version)
    wall_span = int(timeline["present_wall_span_ns"])
    correction = int(timeline.get("lead_correction_ns", 0))
    corrected = int(timeline.get("corrected_wall_span_ns", wall_span + correction))
    if corrected != wall_span + correction:
        raise Failure(
            f"corrected_wall_span_ns({corrected}) != present_wall_span_ns({wall_span}) "
            f"+ lead_correction_ns({correction})"
        )

    # ── P14-T：钟在墙上多走了多少 ──
    #
    # `elapsed_span_ns` 是**钟**走了多远、`arrive_mono_span_ns` 是**墙**走了多远，
    # 同一批呈现帧、同一段窗口（P13-R 为四跨度恒等式印的）。两者之差就是
    # "这只钟比墙快了多少"，而它**从 `/16` 起一直是可算的** —— 只是没有人减。
    #
    # 代价写在 `152-p14s-status.md` §八.2：4 份越界被登记成"成因未查"，
    # 而答案就在同一个块里相邻的两格。这一格现在有了名字（`/31`），
    # 判据也从此要求它解释越界。
    pace = data["pace"]
    # `/16` 之前没有这两个跨度（P13-R 才加），领先量**补不出来**。
    # 那时钉 0 而不是"跳过"：后面的判据要一个 int，而 `None` 会在三处各判一次。
    spans_available = version >= SCHEMA_WITH_MEASURED_SPAN
    clock_lead = (
        int(pace["elapsed_span_ns"]) - int(pace["arrive_mono_span_ns"])
        if spans_available else 0
    )
    if version >= SCHEMA_WITH_CLOCK_LEAD:
        if "clock_lead_ns" not in pace:
            raise Failure(
                f"/{version} 的报告里没有 `pace.clock_lead_ns`。那一版它必须印出来 —— "
                f"这一格是 `/30` 那 4 份越界唯一的名字，缺了它读者还得自己减"
            )
        if int(pace["clock_lead_ns"]) != clock_lead:
            raise Failure(
                f"clock_lead_ns 对不上：报告写 {pace['clock_lead_ns']}，"
                f"`elapsed_span_ns({pace['elapsed_span_ns']}) − "
                f"arrive_mono_span_ns({pace['arrive_mono_span_ns']})` = {clock_lead}。"
                f"这是纯派生量，对不上就是有一格被改过"
            )
    # 领先量非 0 ⇒ `av_drift` 的分位数**不能**当音画偏差读：它量的是"视频相对这只钟"，
    # 不含"钟相对墙"。1% 是提示线，不是判据线 —— 判据线只有一条（`T2`）。
    if spans_available and corrected > 0 and abs(clock_lead) * 100 > corrected:
        notes.append(
            f"钟领先 {clock_lead / 1e6:.3f} ms（{clock_lead / corrected * 100:.3f}% 墙钟）："
            f"这一段的 `av_drift` 少算了这一格，**不能当音画偏差读**"
        )

    # 恒等式（本卡新增）：名义跨度 = (交出去的帧数 − 1) × 名义帧时长。
    #
    # 这一条是 216 份证据**全部**满足、却从来没有人写下来的关系 —— 而整个"播放速率"
    # 都建在它上面。写下来才有意义：手改过 `nominal_span_ns` 的报告，过去没有任何
    # 判据会红，因为另一个数（`nominal_rate`）跟着改就能自洽。
    frame_duration = int(data["pace"]["frame_duration_ns"])
    handed_off = rendered + dropped + discarded
    expected_nominal = (handed_off - 1) * frame_duration if handed_off > 1 else 0
    if nominal_span != expected_nominal:
        raise Failure(
            f"名义跨度对不上帧数：报告写 {nominal_span} ns，"
            f"(rendered {rendered} + dropped {dropped} + dns {discarded} − 1) "
            f"× 帧时长 {frame_duration} = {expected_nominal} ns"
            f"（差 {nominal_span - expected_nominal} ns）。"
            f"这一格不是测量值，是乘法结果 —— 对不上就是有人改过它"
        )

    nominal_rate = nominal_rate_of(timeline, version)
    if nominal_span > 0 and corrected > 0:
        recomputed = nominal_span / float(corrected)
        if abs(recomputed - nominal_rate) > 1e-6:
            raise Failure(f"nominal_rate 对不上：证据写 {nominal_rate:.6f}，"
                          f"重算得 {recomputed:.6f}")
    # ⚠️ `nominal_rate` **不判区间**。
    #
    # 它的分子是帧数换成的秒，不是媒体时间。P14-S 之前这里是 `[0.98, 1.02]`，
    # 判的是"墙钟有没有配得上这么多帧的名义时长"—— 一个有意义、但**不是**
    # "有没有按实时播放"的命题。两项混在一起，才让 15 份证据看起来像"跑偏了"。
    # 现在区间给 `media_rate`，这一格只留恒等式。

    # 实测跨度 / 墙钟 —— 这才是"媒体时间 ÷ 时间"。
    media_span, media_frames, unjudged_reason = media_span_of(data, version)
    media_verdict = "unjudged"
    media_rate = 0.0
    if unjudged_reason:
        notes.append(f"不可判（媒体速率）：{unjudged_reason}")
    else:
        # ── T2：媒体盈余必须由钟解释 ──
        #
        # 这一条是**新**的，也是本卡真正的收获。`due = mono + (pts − anchor − elapsed)`
        # 且 `present ≈ due`（lateness 中位数 125 µs），于是
        #
        #     实测 pts 跨度 − 墙钟 ≡ 钟领先量
        #
        # 全仓 110 份带实测跨度的证据**逐份成立**，最大偏差 189 µs（一帧的 1.1%）。
        # 它抓的不是 `/30` 那 4 份（它们正满足这一条——"钟解释得了"的那一类），
        # 而是**以后出现的"视频真跑快了"**：那种情况盈余会有、领先量没有，
        # 这一条立刻红，且报错指向视频而不是钟。
        #
        # 没有它，T3 的归因就是空的：任何越界都能写一句"钟领先"糊过去。
        if spans_available and int(pace.get("clock_supplied_frames", 0)) >= 2 \
                and frame_duration > 0:
            surplus = media_span - corrected
            if abs(surplus - clock_lead) > frame_duration:
                raise Failure(
                    f"媒体盈余对不上钟：实测 pts {media_span} ns − 修正墙钟 {corrected} ns "
                    f"= {surplus} ns，而钟领先量只有 {clock_lead} ns（差 "
                    f"{surplus - clock_lead} ns，超过一帧 {frame_duration} ns）。"
                    f"两种读法：① 墙钟被截短了（首末呈现帧不是整段的首末）；"
                    f"② 帧真的交付得比 pts 早（不是钟的账）。这一条不许用「钟快」糊过去"
                )
        # ── T3：门开着就不许让它跑过去 ──
        #
        # `drift_gate` 的作用就是把领先量夹回门限附近（P13-AC：越过 `limit`
        # 进入收敛中，修到 `limit − band` 才退出）。真机 58 份门开的轮次，
        # 领先量最大 **44.938 ms**、门限 45 ms —— 一条不越。
        #
        # 所以"门开着 + 领先量超门限 + 一帧"只有一个读法：**门没起作用**。
        # 这一条是本卡唯一能真正判红的归因判据：它不判"跑得快不快"（那是区间的事），
        # 判的是"门有没有在干活"。
        gate = data.get("drift_gate") or {}
        gate_enabled = gate.get("enabled")
        gate_limit = int(gate.get("limit_ns") or 0)
        # P14-Z：**台阶不还**的那一臂，门是被要求不要动的（`step_repay = false`）。
        # 这时领先量本来就该停在台阶量级 —— 判据必须把 `|offset|` 加进上界，
        # 否则它会把"按要求不动"判成"门坏了"。两臂的差别要写在判据里，
        # 不然同一个数字在 A 臂是故障、在 B 臂是设计。
        repay = gate.get("step_repay")
        offset_ns = abs(int((data.get("drift_discontinuity") or {}).get("offset_ns") or 0))
        lead_bound = gate_limit + frame_duration
        if repay is False:
            lead_bound += offset_ns
        if gate_enabled is True and gate_limit > 0 and abs(clock_lead) > lead_bound:
            raise Failure(
                f"门开着却让它跑过去了：钟领先 {clock_lead / 1e6:.3f} ms，"
                f"门限 {gate_limit / 1e6:.3f} ms（+ 一帧 {frame_duration / 1e6:.3f} ms）。"
                f"门的作用就是把领先量夹回门限附近 —— 门在 `drift_gate.enabled=true` 时"
                f"不起作用，比没有门更糟：报告上看不出它坏了"
            )
        media_rate = media_span / float(corrected)
        if version >= SCHEMA_WITH_MEDIA_RATE:
            claimed_media = float(timeline["media_rate"])
            if abs(claimed_media - media_rate) > 1e-6:
                raise Failure(f"media_rate 对不上：证据写 {claimed_media:.6f}，"
                              f"重算得 {media_rate:.6f}")
        if RATE_LOW <= media_rate <= RATE_HIGH:
            media_verdict = "judged"
        else:
            key = evidence_key(path)
            # 归因要带上"这一轮的门是什么状态"：门关着是**对照组的预期行为**，
            # 门那一块还不存在则是"病出生在药之前"。两种都不该被读成"今天还这样"。
            if gate_enabled is False:
                gate_state = "门关着（对照组）"
            elif gate_enabled is None:
                gate_state = f"/{version} 还没有 `drift_gate` 这一块（门是 P13-AD 才出生的）"
            else:
                gate_state = "门开着（已由 T3 单独判过，走到这里说明领先量在门限内）"
            entry = KNOWN_OUT_OF_BAND.get(key)
            lead_ms = clock_lead / 1e6
            pct = clock_lead / corrected * 100 if corrected else 0.0
            if entry is None:
                # 到这里 T2 已经过了，所以"钟解释得了"是**判出来**的，不是猜的。
                # 于是这条报错能说清修哪儿：跑得快的是钟，不是视频。
                raise Failure(
                    f"媒体速率 {media_rate:.6f} 偏离实时"
                    f"（实测 pts {media_span} ns / 修正墙钟 {corrected} ns，{media_frames} 帧），"
                    f"超出 [{RATE_LOW}, {RATE_HIGH}]。归因：钟领先 {lead_ms:.3f} ms"
                    f"（占墙钟 {pct:.3f}%），与媒体盈余 {(media_span - corrected) / 1e6:.3f} ms "
                    f"一致 ⇒ 跑得快的是钟，不是视频。{gate_state}。这一份没登记："
                    f"要么补进 KNOWN_OUT_OF_BAND（带上它的领先量），"
                    f"要么先查清那只钟为什么领先"
                )
            reason, expected_lead = entry
            # 指纹比 `T2` 严一个数量级：`T2` 判的是**归因对不对**（一帧的容差已经够宽），
            # 这里判的是**这份文件还是不是当初登记的那一份**。一帧的容差会让
            # "领先量被改了 10 ms" 悄悄通过 —— 而 10 ms 足够让 `media_rate` 从 1.0434
            # 变到 1.0420，登记的那句话就不成立了。
            if abs(expected_lead - clock_lead) > FINGERPRINT_TOLERANCE_NS:
                raise Failure(
                    f"登记过期：钟领先量变了（登记 {expected_lead} ns，实算 {clock_lead} ns，"
                    f"差 {clock_lead - expected_lead} ns，超过指纹容差 "
                    f"{FINGERPRINT_TOLERANCE_NS} ns）。证据被换过，或者这一轮的钟换了个"
                    f"毛病 —— 两种都得重新看。原文：{reason}"
                )
            media_verdict = "waived"
            notes.append(
                f"已知越界（已登记）：{reason}；实算 media_rate={media_rate:.6f}，"
                f"钟领先 {lead_ms:.3f} ms（{pct:.3f}%）"
            )

    # ── U 组：参考系的第二份（P14-U，`/32` 起）──
    #
    # 上面所有判据判的都是"视频相对这只钟"。而 `due` 就是用这只钟算的，
    # 所以那一整套对"钟自己跑偏"是结构性失明的（P14-T 的真机证据：
    # `p13r-4k60-r3` 上钟比墙多走 426.7 ms，同块 `p50` 仍是 125 µs）。
    #
    # U 组判的是"报告有没有把第二份说对"。五条，全部是**逐位**关系，
    # 不给容差 —— 它们判的是同一批数在三个地方的一致性，不是测量精度。
    if version >= 32:
        check_reference_shift(data, pace, clock_lead, frame_duration, notes)

    # ── V 组：门限的量纲（P14-V，`/33` 起）──
    #
    # `limit_ns` 是 45 ms，每一份里都一样，所以它读起来像"容差恒定"。
    # 实际它守的是累积漂移 = 速率误差 × 时长：同一个 45 ms 在 6.3 s 上等价
    # 7 138 ppm，在 1 800 s 上等价 25 ppm —— 25 ppm 已经是普通晶振的精度。
    # V 组只判这个换算有没有算对，不判 45 ms 该不该改。
    if version >= SCHEMA_WITH_GATE_SCALE:
        check_gate_scale(data, pace, notes)
    if version >= SCHEMA_WITH_REBASE_TRACE:
        check_rebase_trace(data, pace, notes)
    if version >= SCHEMA_WITH_DISCONTINUITY:
        check_discontinuity(data, pace, notes)
    if version >= SCHEMA_WITH_STEP_REPAY:
        gate = data.get("drift_gate") or {}
        if not isinstance(gate.get("step_repay"), bool):
            raise Failure("/37 起 drift_gate.step_repay 必须是布尔（哪一臂跑的要写在自己身上）")
    if version >= SCHEMA_WITH_SPAN_BASIS:
        # AA1：只要这一轮交过帧，跨度就必须有人喂 —— 否则又是"全 0 看起来像没动"。
        span_frames = int(pace.get("span_frames", 0))
        basis = pace.get("span_basis")
        if basis not in ("presented", "scheduled", "none", "mixed"):
            raise Failure(f"/38 起 span_basis={basis!r} 不在白名单里")
        presented = int(pace.get("presented", 0))
        if presented > 0 and span_frames == 0:
            raise Failure(
                f"AA1 presented={presented} 却没有任何一帧喂过跨度（span_frames=0）"
                " —— 四个跨度全 0 不可读作'没动'"
            )

    if data["completed"] and data["error"]:
        raise Failure("completed=true 却带着非空 error")

    summary = (
        f"{path.name}: OK  /{version}  frames={rendered} dropped={dropped} "
        f"clock={clock_source} fallback={data['timestamp_fallback_count']} "
        f"xrun={data['xrun_count']}"
    )
    if measurable and primary_block is not None:
        where, block = primary_block
        summary += (
            f" | {where} p50/p95/p99/max = "
            f"{block['p50']}/{block['p95']}/{block['p99']}/{block['max']} ns"
        )
    else:
        summary += " | av_drift 不可测量（理由已记录）"
    if media_verdict == "unjudged":
        summary += " | 媒体速率不可判"
    else:
        summary += f" | media_rate={media_rate:.6f}"
    return summary, notes, media_verdict


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2

    exit_code = 0
    seen: dict[str, str] = {}
    unjudged = 0
    waived = 0
    superseded = 0
    for raw in argv[1:]:
        path = Path(raw)
        # `superseded/` 里是**已作废**的证据（被后来的轮次取代）。
        #
        # 跳过它们不是"判不过就不判"：`p12c/superseded/p12c8-av-r1-sync.json`
        # 顶层写 `clock_source=system`，而 180 帧走的全是音频档 ——
        # 它**正是因为自相矛盾才被归档的**。拿它当判据输入，等于拿一个
        # 已知的错去考判据，然后得到"判据很严"的错觉。
        #
        # 但也不能默默跳过：汇总行必须报出跳了几份，
        # 否则"扫了 226 份"会让人以为这 226 份都作数。
        if "superseded" in path.parts:
            superseded += 1
            continue
        try:
            summary, notes, verdict = verify(path)
            print(summary)
            for note in notes:
                print(f"    · {note}")
            seen[evidence_key(path)] = verdict
            if verdict == "unjudged":
                unjudged += 1
            elif verdict == "waived":
                waived += 1
        except Failure as failure:
            print(f"{path.name}: FAIL  {failure}")
            exit_code = 1
        except (KeyError, ValueError, TypeError) as error:
            print(f"{path.name}: FAIL  结构不对：{error!r}")
            exit_code = 1

    # 登记表不许腐烂：一份登记过的越界，如果这次跑出来是「在带内」，
    # 那要么证据被换了、要么判据被放水了 —— 两种都得有人看一眼。
    for name, (reason, _lead) in KNOWN_OUT_OF_BAND.items():
        if name not in seen:
            continue
        if seen[name] == "judged":
            print(f"{name}: FAIL  登记过期：它这一轮在带内了，"
                  f"把 KNOWN_OUT_OF_BAND 里那条删掉（原文：{reason}）")
            exit_code = 1
        elif seen[name] == "unjudged":
            print(f"{name}: 注意  登记表里有它，但这一轮它不可判（判据没走到）")

    print(f"—— 合计 {len(seen)} 份：媒体速率 判得动 {len(seen) - unjudged}、"
          f"其中越界已登记 {waived}、不可判 {unjudged}")
    # 总收口只认这两个前缀（见 `verify_p12_all.run_sub`）。写一行是**义务**：
    # 覆盖率塌掉的时候，每一层都还会报"通过"，唯一能看出塌掉的就是这一行。
    # 版本区间**从常量算**，不写死字面量：写死过一版「/1–/33」，
    # P14-W/X 把上限推到 /35 之后这行还在报 /33 —— 而它正是"覆盖率塌掉"的唯一告警行。
    print(f"覆盖：{len(seen)} 份 sync 证据（/{MIN_SCHEMA_VERSION}–/{MAX_SCHEMA_VERSION}）")
    if unjudged:
        print(f"不适用：{unjudged} 份判不了媒体速率（报告里没有实测 pts 跨度，"
              f"逐份理由见上；**这不是通过**）")
    if waived:
        print(f"不适用：{waived} 份媒体速率越界但已登记在 KNOWN_OUT_OF_BAND")
    if superseded:
        print(f"跳过：{superseded} 份在 `superseded/` 里（已作废的证据，不作为判据输入）")
    return exit_code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
