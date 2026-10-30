#!/usr/bin/env python3
"""独立复算抽稀与漂移证据（`kivo.p12c.sync/3` 起，含 `/4` `/5` `/6` `/7`）。

这个脚本存在的理由，是判据 D5 那句"逐位复算"必须真的有人能做到。

证据里带了两把尺：抽稀后的分位数（跨帧率可比）和 8 个窗口的中位数（可复算）。
本脚本从 JSON 里**只读导出过的数**，重放一遍 `SignedHistogram::percentile_ns()`
和 `summarize_drift()` 的窗口回归，再和证据自己写的值比。任何一处对不上就报错 ——
这里不做"看着差不多就算过"，因为"可复算"的全部意义就是误差为 0。

从 P13-E 起多守一条（判据 E1–E3）：**分位数必须在 `quantiles.<口径>` 下**。
`av_drift` 顶层只要出现 `p99` / `samples` / `per_frame` 这类不带口径的键就判红 ——
本卡要消灭的就是"复制一个数、忘了它是哪套口径"这条路。

D5 的坑：C++ 那边回归前把横坐标减去**首样本的媒体时刻** `t0`，否则 6e10 ns 量级
平方后越过 `double` 的 53 位整数精度，斜率会自己漂。本脚本不能让 t 回到绝对量级，
所以统一减去**最小窗口起点**作公共原点 —— 平移不改变斜率，但能把量级压回去。

用法：
    python tools/verify_p13d_drift.py docs/evidence/p13d/*.json
    python tools/verify_p13d_drift.py a.json b.json      # 同时给两份可判 D1/D2
"""

from __future__ import annotations

import json
import math
import statistics
import sys
from pathlib import Path
from typing import Any

# 与 C++ 侧逐字一致。写死而不是从 JSON 读，是为了让"读到的常量"本身也被检查：
# 哪天窗口数被改了而脚本没跟上，这里会报错，而不是悄悄用新常量算出"通过"。
EXPECTED_WINDOW_COUNT = 8
EXPECTED_SAMPLE_INTERVAL_NS = 50_000_000
EXPECTED_TARGET_HZ = 20
MIN_SAMPLES = 30
MIN_SPAN_NS = 30_000_000_000


class Failure(Exception):
    """一条可读的失败。"""


# ---------------------------------------------------------------------------
# 复算：分位数
# ---------------------------------------------------------------------------


def _hist_params(hist: dict[str, Any]) -> tuple[int, int, int]:
    """桶映射的三个常量。缺一不可 —— 缺了就无法把下标还原成时间。"""
    try:
        return (
            int(hist["bucket_width_ns"]),
            int(hist["low_ns"]),
            int(hist["bucket_count"]),
        )
    except KeyError as exc:
        raise Failure(f"histogram 缺少常量字段 {exc}") from exc


def replay_percentile(hist: dict[str, Any], quantile: float) -> int:
    """只吃导出到 JSON 的分布，重放 `SignedHistogram::percentile_ns()`。

    秩取上整、桶心取值、结果钳进 [min, max]。三条都与 C++ 逐行对应。
    """
    width, low, bucket_count = _hist_params(hist)
    count = int(hist["count"])
    if count == 0:
        return 0
    if quantile <= 0.0:
        return int(hist["min_ns"])
    if quantile >= 1.0:
        return int(hist["max_ns"])

    rank = int(quantile * count)
    if float(rank) < quantile * count:
        rank += 1
    if rank < 1:
        rank = 1

    min_ns = int(hist["min_ns"])
    max_ns = int(hist["max_ns"])

    def clamp(value: int) -> int:
        return min(max(value, min_ns), max_ns)

    below = int(hist["below"])
    if rank <= below:
        return min_ns

    accumulated = below
    bins: dict[str, int] = {str(k): int(v) for k, v in hist.get("buckets", {}).items()}
    for index in range(bucket_count):
        accumulated += bins.get(str(index), 0)
        if accumulated >= rank:
            # 桶心：下界 + 下标×桶宽 + 半个桶宽（C++ 用的就是这个点）
            return clamp(low + index * width + width // 2)
    return max_ns


def replay_quantiles(hist: dict[str, Any]) -> dict[str, int]:
    return {
        "p50": replay_percentile(hist, 0.50),
        "p95": replay_percentile(hist, 0.95),
        "p99": replay_percentile(hist, 0.99),
        "max": int(hist["max_ns"]),
        "samples": int(hist["count"]),
    }


# ---------------------------------------------------------------------------
# 复算：窗口斜率（判据 D5）
# ---------------------------------------------------------------------------


def replay_window_slope(trend: dict[str, Any]) -> tuple[float, float]:
    """从 8 个 `(起点, 中位数)` 重算斜率，返回 `(slope_ppm, r2)`。

    与 `summarize_drift()` 的口径逐条对齐：
      - 横坐标取**窗口中心**（起点 + 半窗宽），不是起点 —— 取起点会把斜率
        系统性偏掉半个窗宽；
      - 先减公共原点再平方（见模块 docstring 的精度说明）；
      - 中心化矩 `Sxx = Σt² − (Σt)²/n`，`Sxy = Σty − Σt·Σy/n`。
    """
    span = int(trend["span_ns"])
    starts = list(trend["window_start_ns"])
    medians = list(trend["window_median_ns"])
    if len(starts) != len(medians):
        raise Failure("window_start_ns 与 window_median_ns 长度不一致")
    if len(starts) < 2:
        raise Failure("窗口少于 2 个，斜率不可算（应带 usable=false）")

    width = span // EXPECTED_WINDOW_COUNT
    origin = min(starts)
    xs = [float(start + width // 2 - origin) for start in starts]
    ys = [float(value) for value in medians]

    n = float(len(xs))
    sum_t = sum(xs)
    sum_y = sum(ys)
    sum_tt = sum(x * x for x in xs)
    sum_ty = sum(x * y for x, y in zip(xs, ys))
    sum_yy = sum(y * y for y in ys)

    sxx = sum_tt - sum_t * sum_t / n
    if sxx <= 0.0:
        raise Failure("时间不前进（Sxx <= 0），窗口斜率不可算")
    sxy = sum_ty - sum_t * sum_y / n
    slope_per_ns = sxy / sxx

    mean_y = sum_y / n
    sstot = sum_yy - n * mean_y * mean_y
    if sstot <= 0.0:
        r2 = 0.0
    else:
        ssres = sstot - sxy * sxy / sxx
        r2 = 1.0 - max(ssres, 0.0) / sstot

    return slope_per_ns * 1e6, r2


# ---------------------------------------------------------------------------
# 单文件检查
# ---------------------------------------------------------------------------


class Report:
    def __init__(self, path: Path) -> None:
        self.path = path
        # 显示用的标题，默认取文件名。
        #
        # 为什么不能直接用 `path.name`：`Path` 会把 `/` 当目录分隔符，
        # 于是 `Path("D2 media_time_decimated/p99 比值")` 的 `.name` 只剩
        # "p99 比值" —— 口径前缀被当成目录名吃掉了，而这恰好是本卡唯一的产出。
        # 标签必须是一个纯字符串。
        self.label = path.name
        self.rows: list[tuple[str, bool, str]] = []
        # 「不适用」既不是通过也不是失败：`/1` 证据本来就没有抽稀块，
        # 把它算成失败会让跑整个 evidence 目录的人得到 24 条假红，
        # 而假红比漏报更快地训练人忽略告警。
        self.not_applicable = ""

    def add(self, name: str, ok: bool, detail: str = "") -> bool:
        self.rows.append((name, ok, detail))
        return ok

    @property
    def failed(self) -> list[tuple[str, bool, str]]:
        return [row for row in self.rows if not row[1]]


def load(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


# 三个口径名必须与 `core/pace/sampling_mode.cxx` 逐字一致 ——
# C++ 那份由 `kivo_p13e_sampling_mode_tests` 钉死，这里钉另一半。
MODE_DECIMATED = "media_time_decimated"
MODE_PER_FRAME = "per_frame"
MODE_NOT_MEASURED = "not_measured"
MODES = (MODE_DECIMATED, MODE_PER_FRAME, MODE_NOT_MEASURED)

# `/3`（P13-E）与 `/4`（P13-F）共用 `quantiles.<口径>` 结构；`/4` 只多一格 `worst`。
# `/5`（P13-G）再改一件事：抽稀块要成立，至少要 **2 个样本且跨度为正**。
#   1 个样本的块每个分位数都相同，长得像正常数据（见 `68-p13g-design.md` §二），
#   旧判据 `accepted > 0 ⇒ 块存在` 放它过去。`/5` 起 `kPerFrame` 才是真可达的降级。
# `/6`（P13-H）`sampling` 新增 `media_span_ns`（**整场**累计跨度），
#   `observed_hz` 改以它为分母。`/3`–`/5` 用的是 `trend.span_ns`（**窗口**跨度，
#   封顶 102.35 s），超过 102.4 s 之后商随片长线性增长（见 `70-p13h-design.md` §一）。
# 全接：旧证据不重写（重写等于伪造），新判据只对声明了新版本的那份生效。
#
# `/7`（P13-I）改两处口径：
#   - `truncated` 改成"**本段**接受数 > 容量"。`/6` 及以前是"整场累计 > 容量"，
#     seek 之后会把"换过时间轴"说成"环形覆盖过"（见 `72-p13i-design.md` §一）。
#   - `trend` 自带 `window_quantiles`，删掉含糊的 `segment_samples`，
#     换成 `capacity` / `segment_accepted` / `window_sample_count` 三格。
#
# ── 版本序：单点登记，判据自动跟进 ──
#
# 这些表**原来是各写各的**，于是每升一次 schema 都要记得把每一张表一起加一项。
# 这个项目为此栽过三次（`check_pair` 硬编码 `/3`、`SCHEMA_WITH_WORST` 用 `==` 比较、
# `SCHEMA_WITH_MEDIA_SPAN` 是标量）—— 每次的后果都一样：新证据带着一整块
# 没人验的字段通过总收口，而且**输出里连一行"不适用"都没有**。
#
# `/9`（P13-K）起改成从 `SCHEMA_ORDER` 派生：版本号严格累积（新版本保留旧版本
# 全部字段），所以"从第 k 个版本起生效"可以机械展开。新增一个版本只需在
# `SCHEMA_ORDER` 里加一项，五张表同时跟上 —— 这类漏改从**结构上**不再可能。
SCHEMA_PREFIX = "kivo.p12c.sync"
SCHEMA_ORDER = ("/3", "/4", "/5", "/6", "/7", "/8", "/9", "/10", "/11", "/12", "/13")


def _schemas_from(version: str) -> tuple[str, ...]:
    """从 `version`（含）起的所有 schema。版本登记漏了就在 import 期炸，不静默。"""
    if version not in SCHEMA_ORDER:
        raise ValueError(f"schema {version} 不在 SCHEMA_ORDER 里 —— 新增版本必须登记一次")
    return tuple(SCHEMA_PREFIX + item for item in SCHEMA_ORDER[SCHEMA_ORDER.index(version):])


SUPPORTED_SCHEMAS = _schemas_from("/3")
# `/4`：`quantiles.<口径>` 多一格 `worst`（最差点在片头还是片尾）。
SCHEMA_WITH_WORST = _schemas_from("/4")
# `/5`：抽稀块结构门（≥2 样本且跨度 >0）。1 个样本的块四个分位数全相同，
# 长得像正常数据，旧判据 `accepted > 0 ⇒ 块存在` 放它过去（见 `68-p13g-design.md` §二）。
SCHEMA_WITH_FLOOR = _schemas_from("/5")
# `/6`：`sampling.media_span_ns`（整场累计跨度），`observed_hz` 以它为分母。
# `/3`–`/5` 用 `trend.span_ns`（窗口跨度，封顶 102.35 s），102.4 s 之后商随片长线性增长。
SCHEMA_WITH_MEDIA_SPAN = _schemas_from("/6")
# `/7`：`capacity` / `segment_accepted` / `window_sample_count` / `window_quantiles`；
# `truncated` 改成"本段接受数 > 容量"（seek 之后不再谎报覆盖）。
SCHEMA_WITH_WINDOW = _schemas_from("/7")
# `/8`：斜率误差棒（`slope_ppm_std_error` / CI / `slope_detectable` / 窗口中位数精确性）。
SCHEMA_WITH_CONFIDENCE = _schemas_from("/8")
# `/9`：`slope_ppm_std_error` 换成 HAC（Newey–West）口径，旧值改名 `_iid`，
# 外加 `hac_lags` / `hac_lag1_autocorrelation` / `hac_estimated` / 膨胀比与半程斜率。
SCHEMA_WITH_HAC = _schemas_from("/9")
# `/10`：累积漂移与额度判读（`drift_accumulated_ns` / `_upper_ns` /
# `drift_budget_ns` / `drift_verdict`）。把"看得见"翻译成"要紧不要紧"。
SCHEMA_WITH_BUDGET = _schemas_from("/10")
# `/11`：`segments` 后面接上 `segment_largest_gap_cells` / `segment_contiguous`。
# `segments` 的语义也在这里收窄：一次**锚点生命周期**算一段（由 `FramePacer::reset()`
# 记），不再是"媒体时刻倒退的次数 + 1"（那是从 pts 推断的，向前 seek 必然漏）。
SCHEMA_WITH_SEGMENT_AUTHORITY = _schemas_from("/11")
# `/12`：每个 `quantiles.<口径>` 的 `p50` 后面接上 `p50_range_low_ns` /
# `p50_range_high_ns` / `p50_bucket_samples` / `bucket_width_ns`；
# `trend` 里接上 `median_of_window_medians_ns`。
# 起因是同一份证据里的两个 p50 相差 66%，而 JSON 里没有任何字段能阻止读者
# 把它读成"两份测量打架" —— 一个是桶中心（125 000），一个是精确值（74 835）。
# 它们不打架，是同一批点的两种精度；缺的是**让 p50 自己说出它可能的范围**。
SCHEMA_WITH_QUANTILE_RESOLUTION = _schemas_from("/12")
# `/13`（P13-O）：半程各自带 HAC 标准误与 95% 带，新增 `slope_halves_*` 共 14 格
# 与三态 `slope_halves_relation`；`slope_halves_agree` 降为历史字段。
# 起因：那个布尔在 13 份真机证据里报过 8 次 true，其中 3 份两半程量级差 5 倍以上，
# 最极端的一份两半程都为正、全序列斜率却为负 —— 而它照样说 "agree"。
SCHEMA_WITH_HALVES = _schemas_from("/13")

# 段内允许的最大空洞（格点）。**必须与 `core/pace/pace_drift_series.hpp` 的
# `kMaxContiguousGapCells` 相等** —— 写死在这里的全部意义就是"它被独立核对过一次"。
# 若哪天 C++ 把它改成 40，这里会红；而如果这里改成"从证据里读 gap 再和它自己比"，
# 它就只会跟着一起错（P13-H 的 `observed_hz` 正是这么栽的）。
MAX_CONTIGUOUS_GAP_CELLS = 20

# 分位数直方图的桶宽（P13-N）。**必须与 `pace_histogram.hpp` 的
# `SignedHistogram::kBucketWidthNs` 相等** —— 独立写死才有核对的意义。
# 若哪天 C++ 把桶宽改成 100 µs，这里会红；而如果这里改成"从证据的
# `bucket_width_ns` 读出来再和它自己比"，它就只会跟着一起错
# （P13-H 的 `observed_hz` 正是这么栽的）。
QUANTILE_BUCKET_WIDTH_NS = 250_000

# 环形容量。**必须与 `core/pace/pace_drift_series.hpp` 的 `kCapacity` 相等** ——
# 写死在这里是为了让"读到的常量"也被检查：哪天 C++ 改了容量而判据没跟上，
# 这里会红，而不是悄悄用新常量算出一个"通过"。
EXPECTED_CAPACITY = 2048

# 抽稀块成立的最小样本数。**必须与 `core/pace/sampling_mode.hpp` 的
# `kMinDecimatedSamples` 相等** —— 两处不一致时，一份"证据认为可用、
# 校验器认为不可用"的报告会两边都说自己是对的。
MIN_DECIMATED_SAMPLES = 2

# 出现在 `av_drift` 顶层就算"裸分位数"。`per_frame` 也在里面：
# 那是 /2 的旧位置，P13-E 起它挂在 `quantiles.per_frame` 下。
# 把这些键挡在顶层，是为了让"这份数是什么口径"无法被省略 ——
# 键名不带口径，数字一旦被复制进表格就再也追不回来。
BARE_QUANTILE_KEYS = ("samples", "p50", "p95", "p99", "max", "histogram", "per_frame")


def resolve_path(doc: dict[str, Any], dotted: str) -> Any:
    """按 `a.b.c` 取节点；任一段不在就返回 None。"""
    node: Any = doc
    for part in dotted.split("."):
        if not isinstance(node, dict) or part not in node:
            return None
        node = node[part]
    return node


def check_worst(report: Report, label: str, block: dict[str, Any],
                interval_ns: int, duration_ns: int, segments: int = 1,
                unobserved_cells: int = 0) -> None:
    """F1–F3：`worst` 必须指向**同口径**的 max，且位置自洽（P13-F）。

    `max` 只说最差多大，说不出在片头还是片尾 —— 而这两者对结论的影响完全相反
    （冷启动可忽略 / 片尾收尾不是播放能力问题 / 片中抢占要修）。
    所以 `worst` 不是"多一格的装饰"，它必须能和 `max` 对得上，否则两个数就是两笔账。

    `interval_ns > 0` 时额外查一条**对齐**判据：抽稀口径的样本按 50 ms 绝对格点采，
    所以 `media_ns` 应该约等于 `index × interval`。
    这条抓的是"把直方图的桶下标当成样本下标"那类错位 —— 250000 ns（一个桶宽）
    落在一个 8 秒片源里完全"合法"，只有和 `index` 摆在一起才看得出不对。

    ⚠️ 但这条对齐只在**单段**上成立：`index` 是整场累计（跨段相加），
    而 `media_ns` 是**当前时间轴**上的时刻。seek 之后两者必然不同源 ——
    P13-J 的 seek 轮实测 index=3074 对 media_ns=128.77 s，偏移 −24.9 s，
    正好是那次 55 s → 30 s 的倒退。跨段时这条判据不适用（P13-I 的
    `segments` 字段就是为回答这个问题而加的），硬判只会产出假红，
    而假红比漏报更训练人忽略告警。
    """
    samples = int(block.get("samples", 0))
    worst = block.get("worst")

    # ---- F3：有样本 ⇔ 有 worst ----
    if samples <= 0:
        report.add(f"F3 [{label}] 无样本 ⇒ 无 worst", worst is None,
                   "samples=0 却给出 worst —— 零会被读成「最差发生在第 0 个样本」"
                   if worst is not None else "一份都没有，也就不写位置")
        return
    if not isinstance(worst, dict):
        report.add(f"F3 [{label}] 有样本 ⇒ 必有 worst", False,
                   f"samples={samples} 却没有 worst：位置这一格丢了")
        return
    report.add(f"F3 [{label}] 有样本 ⇒ 必有 worst", True, f"samples={samples}")

    # ---- F1：worst 的值必须与同口径 max 逐位相等 ----
    #
    # 真正抓住"极值与直方图分家"的地方。两条路各自维护（一条扫序列 /
    # 一条 O(1) 比较），算法不同、时机不同，只有这个等式能证明它们算的是同一件事。
    max_ns = int(block.get("max", 0))
    worst_ns = int(worst.get("lateness_ns", 0))
    report.add(f"F1 [{label}] worst = 同口径 max", worst_ns == max_ns,
               f"worst {worst_ns} vs max {max_ns}（差 {worst_ns - max_ns}）")

    # ---- F2：位置自洽 ----
    index = int(worst.get("index", -1))
    of = int(worst.get("of", -1))
    media_ns = int(worst.get("media_ns", -1))

    report.add(f"F2 [{label}] 0 ≤ index < of = samples",
               of == samples and 0 <= index < of,
               f"index={index} of={of} samples={samples}"
               + ("（of 应等于 samples）" if of != samples else ""))

    # 上界放宽 1 秒：片源的起点 pts 不一定是 0。
    upper_ns = duration_ns + 1_000_000_000
    report.add(f"F2 [{label}] media_ns 落在片长内", 0 <= media_ns <= upper_ns,
               f"media_ns={media_ns} 上界 {upper_ns}")

    if interval_ns > 0:
        if segments > 1:
            report.add(f"F2 [{label}] 跨段 ⇒ media_ns 与 index 对齐判据不适用", True,
                       f"segments={segments}：index 是整场累计（含前面几段），"
                       "media_ns 是当前时间轴上的时刻 —— 两者本来就不同源，"
                       "硬判只会得到一条假红")
        else:
            expected_ns = index * interval_ns
            offset_ns = media_ns - expected_ns
            # 容差 = 一个格点，**每个没采到的格点再加一个**。
            #
            # 为什么不是恒定 50 ms：抽稀是"取格点之后的**第一帧**"，30 fps 下
            # 每个样本本来就可以滞后最多一帧（33.3 ms）；而一旦有格点没采到
            # （丢帧 / 调度空洞），后面的样本整段后移，偏移按没采到的格点数累加。
            # P13-K 的 r1 丢了 4 帧（`unobserved_cells = 2`），第 1 个样本就后移
            # 116.7 ms —— 恒定 50 ms 会把它判成"下标与时刻不同源"，而它们其实同源：
            # 那 4 帧是真的没送到，序列老老实实没采。
            #
            # 放宽是**按证据里已有的数**放，不是拍一个更大的常数：
            # 没丢帧的那些轮次容差仍是 50 ms，错位一格的实现照样红。
            tolerance_ns = interval_ns * (1 + unobserved_cells)
            report.add(f"F2 [{label}] media_ns 与 index 对齐",
                       abs(offset_ns) <= tolerance_ns,
                       f"media_ns={media_ns} 期望≈{expected_ns} "
                       f"（实测偏移 {offset_ns / 1e6:.3f} ms，容差 {tolerance_ns / 1e6:.0f} ms，"
                       f"未采格点 {unobserved_cells}）")


def check_decimated_floor(report: Report, mode: str, accepted: int, span: int,
                          decimated_set: Any, per_frame: Any) -> None:
    """G1–G3：抽稀块的最小可用门槛（`/5`）。

    三个判据分开说三件事，合起来才拦得住"看起来像抽稀数据"的那一份：

      G1 抽稀成立时，块必须存在**且够格**（≥2 样本、跨度 > 0）。
         1 个样本时 p50/p95/p99/max 四个数完全相同，而 E1/E2/E3/F1/F2′
         全套都会放它过去 —— 缺 pts 的流正好产出这个形态。
      G2 降级到 `per_frame` 时，抽稀块必须**整块缺席**。
         留着它在，等于承认"我们有抽稀数据，只是不用"。
      G3 `accepted` 不许为了过 G2 被改写。
         "采到了几个"和"这份能不能用"是两件事，都要说。
    """
    has_block = isinstance(decimated_set, dict)

    if mode == MODE_DECIMATED:
        ok = has_block and accepted >= MIN_DECIMATED_SAMPLES and span > 0
        detail = f"accepted={accepted}, span={span}ns, 抽稀块{'在' if has_block else '缺'}"
        if has_block:
            detail += f", samples={decimated_set.get('samples')}"
        report.add(
            f"G1 抽稀成立 ⇒ 块存在且 ≥{MIN_DECIMATED_SAMPLES} 样本、跨度 >0", ok, detail)
        return

    if mode != MODE_PER_FRAME:
        return

    report.add("G2 降级到 per_frame ⇒ 抽稀块整块缺席", not has_block,
               f"mode=per_frame 却仍有 quantiles.{MODE_DECIMATED}："
               f"那是 {accepted} 个样本凑出来的假块" if has_block
               else "抽稀块已缺席")
    report.add("G3 降级成立 ⇒ accepted 确实不够格",
               accepted < MIN_DECIMATED_SAMPLES or span <= 0,
               f"accepted={accepted}, span={span}ns：两个条件都够格却降级了，"
               "说明判据与实现不同步")
    report.add("G2 降级 ⇒ 逐帧那份仍然存在", bool(per_frame),
               "逐帧那份在" if per_frame
               else "逐帧那份也缺：那一轮该报 not_measured，不是 per_frame")


def check_media_span(report: Report, sampling: dict[str, Any], trend: dict[str, Any],
                     accepted: int) -> None:
    """H1–H4：`observed_hz` 的分子分母必须同口径（`/6`）。

    这一组存在的理由不是"更精确"，是**上一版会自己长数**：
    `accepted` 是全场累计，分母却取了 `trend.span_ns`（窗口跨度，封顶 102.35 s）。
    8 s / 35 s 片源上两者相等，所以 27 份旧证据全部通过 ——
    缺陷只在没人跑过的时长里存在，而校验器复算的是同一个除法，
    于是它连"两边一起错"都发现不了（`70-p13h-design.md` §一）。

      H0  `media_span_ns` 必须在。缺了它，读者又要去别处找分母。
      H1  `accepted + unobserved == floor(media_span/interval) + segments`
          —— 单段时是**恒等式**（不是不等式、不是容差）。
          "接受数 + 空格数"与"媒体时间跨过多少格"对不上，只有一个解释：
          两边算的不是同一段时间。
      H2  `observed_hz == accepted × 1e9 / media_span_ns`。报告写错算式。
      H3  未覆盖 ⇒ 累计跨度与窗口跨度**逐位相等**（那时它们本来就该相等）；
          覆盖后 ⇒ 累计跨度必须**大于**窗口跨度。
          这条同时抓"累计没实现"和"累计实现成了窗口"。
      H4  `accepted == 0 ⇒ media_span == 0 ∧ observed_hz == 0`。
          零样本却量到一段跨度，是"没测到"被写成"测到了"。
    """
    interval = EXPECTED_SAMPLE_INTERVAL_NS
    window_span = int(trend.get("span_ns", 0))
    truncated = bool(trend.get("truncated", False))
    segments = int(trend.get("segments", 1))
    observed_hz = float(sampling.get("observed_hz", 0.0))
    unobserved = int(sampling.get("unobserved_cells", 0))

    if "media_span_ns" not in sampling:
        report.add("H0 声明 sampling.media_span_ns", False,
                   "缺这一格，`observed_hz` 的分母就要去别处找 —— 上一版正是这么错的")
        return
    media_span = int(sampling["media_span_ns"])
    report.add("H0 声明 sampling.media_span_ns", True, f"{media_span}ns")

    # ---- H1：恒等式（单段）/ 上界（多段） ----
    #
    # 为什么多段降成上界：每个段的恒等式各自有个 "+1"（首帧无条件接受），
    # 而 `floor(a)+floor(b) <= floor(a+b)`，所以求和后只能保证不超过。
    # 证据里没有分段明细，硬要等式会产出假红 —— 假红比漏报更快训练人忽略告警。
    cells = media_span // interval
    upper = cells + segments
    lhs = accepted + unobserved
    if segments == 1:
        report.add("H1 accepted + unobserved = floor(media_span/interval)+1",
                   lhs == upper,
                   f"{accepted}+{unobserved}={lhs} vs {cells}+1={upper}："
                   "两者不等说明接受数与媒体时间不是同一段（口径混用）")
    else:
        report.add("H1 accepted + unobserved <= floor(media_span/interval)+segments",
                   lhs <= upper,
                   f"{accepted}+{unobserved}={lhs} vs 上界 {upper}（{segments} 段）")

    # ---- H2：节奏不可能超过格点目标 ----
    #
    # 这是 H1 的速率表述，之所以单独列一条，是因为它读起来最直白：
    # 目标是 20 Hz，报出 58 Hz 就没人会信；而报出 20.1 Hz 时谁也不会多看一眼。
    # 上界带一个 `segments × interval/media_span` 的余量 —— 那是**每段各一次**的
    # 首帧无条件接受。这里原本只加了一次，于是 seek 那一轮（2 段、3100 样本）
    # 报了 20.0086 Hz 对上界 20.0065 Hz，被判成"分子分母不同源"——
    # **判据自己犯了它要抓的那个错**（把整场当成一段）。见 `73-p13i-status.md` §六。
    if accepted > 0 and media_span > 0:
        # 精确上界：`accepted ≤ span/interval + segments`，两边同除 span 得到
        # `hz ≤ 1e9/interval + segments·1e9/span`。
        ceiling = 1e9 / interval + segments * 1e9 / media_span
        report.add("H2 observed_hz 不超过格点目标速度",
                   observed_hz <= ceiling * (1.0 + 1e-9),
                   f"{observed_hz:.6f} Hz vs 上限 {ceiling:.6f} Hz（{segments} 段）"
                   "（超了只能是分子分母不同源）")

    # ---- H3：两根尺子的关系 ----
    #
    # 三分支而不是两分支（P13-I）。上一版只有"覆盖/未覆盖"，因为那时
    # `truncated` 被当成"窗口比累计少"的唯一解释 —— 而 seek 同样会让窗口变短。
    # 两个原因必须分开判：覆盖看 `truncated`，跨段看 `segments`。
    # `/7` 起 `truncated` 的判据本身也修了（段口径），所以这里能分开；
    # `/6` 及以前 `truncated` 还是整场口径，多段时它会假真 —— 因此老版本上
    # 一旦 `segments > 1` 就只判一个弱上界，不硬套等式（假红比漏报更快教人忽略告警）。
    if segments == 1 and not truncated:
        report.add("H3 未覆盖且单段 ⇒ 累计跨度 = 窗口跨度",
                   media_span == window_span,
                   f"累计 {media_span}ns vs 窗口 {window_span}ns："
                   "没发生覆盖时两者必然相等，不等说明有一方算错了")
    elif segments == 1:
        report.add("H3 覆盖 ⇒ 累计跨度 > 窗口跨度",
                   media_span > window_span > 0,
                   f"累计 {media_span}ns vs 窗口 {window_span}ns："
                   "相等说明累计口径没实现（或实现成了窗口跨度）")
    else:
        report.add("H3 跨段 ⇒ 累计跨度 ≥ 窗口跨度",
                   media_span >= window_span > 0,
                   f"累计 {media_span}ns vs 窗口 {window_span}ns（{segments} 段）："
                   "累计是各段之和，窗口只有最后一段；相等只可能发生在"
                   "前面的段一个跨度都没贡献时")

    # ---- H4：零样本 ----
    if accepted == 0:
        report.add("H4 零样本 ⇒ 跨度为 0 且 observed_hz 为 0",
                   media_span == 0 and observed_hz == 0.0,
                   f"accepted=0 却报 media_span={media_span}ns、hz={observed_hz:.3f}")


def histograms_identical(a: dict[str, Any], b: dict[str, Any]) -> tuple[bool, str]:
    """两份分布是不是**同一批点**。逐字段 + 逐桶比。

    只比 `count` 等于什么都没比：同样的样本数可以落在完全不同的桶里，
    而桶才是分位数的全部依据。
    """
    for field in ("count", "below", "above", "min_ns", "max_ns",
                  "bucket_width_ns", "low_ns", "bucket_count"):
        if int(a.get(field, -1)) != int(b.get(field, -1)):
            return False, f"{field} {a.get(field)} vs {b.get(field)}"
    bins_a = {str(key): int(value) for key, value in a.get("buckets", {}).items()}
    bins_b = {str(key): int(value) for key, value in b.get("buckets", {}).items()}
    if bins_a != bins_b:
        for key in sorted(set(bins_a) | set(bins_b), key=lambda item: int(item)):
            if bins_a.get(key, 0) != bins_b.get(key, 0):
                return False, f"桶 {key}：{bins_a.get(key, 0)} vs {bins_b.get(key, 0)}"
    return True, ""


def check_window_scope(report: Report, sampling: dict[str, Any], trend: dict[str, Any],
                       quantiles: dict[str, Any], accepted: int) -> None:
    """I0–I6：窗口的身份（`/7`）。

    这一组的存在理由，是上一版有两个数在回答同一个问题而互不知情：
    `truncated` 既想说"环形覆盖过"，又想解释"窗口为什么比累计少"。
    seek 之后它把后者说成前者 —— 一份 `truncated: true / window_sample_count: 1800`
    的证据自相矛盾：覆盖过的环形序列必然满 2048。

    所以判据不是"再算一次减法"，是**给每个原因一个独立的量**，再让它们互相闭合：

      I0  `capacity` / `segment_accepted` / `window_sample_count` 三格必须在，
          且含糊的 `segment_samples` 必须已删除（它同时被读成两种口径）。
      I1  `Σ window_samples == window_sample_count`。
          左边来自 8 趟分窗循环，右边来自 `DriftSeries::size()` ——
          算法不同、路径不同，只有真的数同一批点才会相等。
      I2  `window_sample_count == min(segment_accepted, capacity)`。
          这一条就是报告层的算法本身；校验器从 JSON 重算一遍。
      I3  `truncated ⇔ segment_accepted > capacity`。
          **本卡的核心**：覆盖是每段的事，不是整场的事。
      I4  窗口分布 ≤ 累计分布（子集），且**未覆盖且单段时必须逐桶相同**。
          "不同"要有书面许可：`truncated` 或 `segments > 1`。
      I5  `truncated ⇒ window_sample_count == capacity ∧ accepted > capacity`。
          覆盖过的环形序列必然是满的 —— 这是上一版那句假话的照妖镜。
      I6  `segments == 1 ⇒ segment_accepted == accepted`。
          单段时两个数本来就该相等，不等说明有一个算错了段。
    """
    capacity = int(trend.get("capacity", 0))
    segment_accepted = int(trend.get("segment_accepted", -1))
    window_count = int(trend.get("window_sample_count", -1))
    truncated = bool(trend.get("truncated", False))
    segments = int(trend.get("segments", 1))

    # ---- I0 ----
    missing = [key for key in ("capacity", "segment_accepted", "window_sample_count")
               if key not in trend]
    report.add("I0 trend 声明 capacity / segment_accepted / window_sample_count",
               not missing, f"缺 {missing}" if missing
               else f"capacity={capacity}, 段内 {segment_accepted}, 窗口 {window_count}")
    report.add("I0 含糊的 segment_samples 已删除", "segment_samples" not in trend,
               "它还留着：这个名字同时被读成「这段采了几个」与「窗口里剩几个」"
               if "segment_samples" in trend else "已由 segment_accepted + window_sample_count 取代")
    if missing:
        return  # 下面每条都要这三格，缺了就都是假红

    report.add("I0 capacity 与 C++ 常量一致", capacity == EXPECTED_CAPACITY,
               f"证据 {capacity} vs 脚本 {EXPECTED_CAPACITY}："
               "两边不同步时 min() 会算出第三个答案")

    # ---- I1 ----
    per_window = [int(value) for value in trend.get("window_samples", [])]
    report.add("I1 Σ window_samples = window_sample_count",
               sum(per_window) == window_count,
               f"8 趟分窗合计 {sum(per_window)} vs 序列长度 {window_count} "
               f"（逐窗 {per_window}）：不等说明分窗循环与 size() 数的不是同一批点")

    # ---- I2 ----
    expected_window = min(segment_accepted, capacity)
    report.add("I2 window_sample_count = min(segment_accepted, capacity)",
               window_count == expected_window,
               f"证据 {window_count} vs 重算 min({segment_accepted}, {capacity}) = {expected_window}")

    # ---- I3 ----
    report.add("I3 truncated ⇔ segment_accepted > capacity",
               truncated == (segment_accepted > capacity),
               f"truncated={truncated}，段内 {segment_accepted} / 容量 {capacity}："
               "覆盖是每段的事 —— 拿整场累计数去比容量，seek 之后必然谎报")

    # ---- I4 ----
    window_quantiles = trend.get("window_quantiles")
    whole = (quantiles or {}).get(MODE_DECIMATED)
    if not isinstance(window_quantiles, dict):
        report.add("I4 trend 带 window_quantiles", False,
                   "趋势没带自己那批点的分布，读者只能跨块借数（本卡修的就是这个）")
    else:
        report.add("I4 trend 带 window_quantiles", True,
                   f"samples={window_quantiles.get('samples')}")
        report.add("I4 window_quantiles.samples = window_sample_count",
                   int(window_quantiles.get("samples", -1)) == window_count,
                   f"{window_quantiles.get('samples')} vs {window_count}")
        if isinstance(whole, dict) and window_quantiles.get("histogram") and whole.get("histogram"):
            # 子集：窗口的极值不可能超过整场。抓的是"往窗口分布里掺了别的样本"。
            report.add("I4 窗口 max ≤ 整场 max（子集）",
                       int(window_quantiles.get("max", 0)) <= int(whole.get("max", 0)),
                       f"窗口 {window_quantiles.get('max')} vs 整场 {whole.get('max')}")
            same, detail = histograms_identical(window_quantiles["histogram"], whole["histogram"])
            if segments == 1 and not truncated:
                # 没有覆盖、没有跨段 —— 两份分布本来就是同一批点。
                report.add("I4 未覆盖且单段 ⇒ 两份分布逐桶相同", same,
                           detail or "同一批点，逐桶一致")
            else:
                # 不同是允许的，但**必须能指出原因**。
                cause = "跨段" if segments > 1 else "覆盖"
                report.add(f"I4 两份分布可以不同，但原因已声明（{cause}）",
                           True,
                           f"差异 {'存在' if not same else '不存在'} —— "
                           f"segments={segments}, truncated={truncated}")
        elif not isinstance(whole, dict):
            report.add("I4 整场抽稀分布缺席 ⇒ 跳过逐桶比对", True,
                       "抽稀块不在（per_frame 轮次），没有可比的整场分布")

    # ---- I5 ----
    #
    # 判据第一版写的是 `未覆盖 ⇒ 整场不超过容量`，在 seek 那一轮当场假红：
    # 整场 3100 > 2048，可是一个点都没被覆盖 —— 因为整场是**两段加起来**的。
    # 覆盖只能在段内发生，所以"超没超容量"这个比较也必须在段内做。
    # 这是本卡第二次自己犯下它要抓的错（第一次是 H2 的 +1 只加了一次）。
    if truncated:
        report.add("I5 覆盖 ⇒ 窗口是满的，且段内比容量多",
                   window_count == capacity and segment_accepted > capacity
                   and accepted > capacity,
                   f"窗口 {window_count} / 容量 {capacity} / 段内 {segment_accepted} / "
                   f"整场 {accepted}：覆盖过的环形序列必然满 —— 不满就说明 "
                   "truncated 说的是别的意思")
    else:
        report.add("I5 未覆盖 ⇒ 段内不超过容量", segment_accepted <= capacity,
                   f"段内 {segment_accepted} / 容量 {capacity}"
                   + (f"（整场 {accepted} 更多，那是 {segments} 段加起来的结果，不是覆盖）"
                      if accepted > capacity else ""))

    # ---- I6 ----
    if segments == 1:
        report.add("I6 单段 ⇒ segment_accepted = accepted",
                   segment_accepted == accepted,
                   f"段内 {segment_accepted} vs 整场 {accepted}")
    else:
        report.add("I6 跨段 ⇒ segment_accepted ≤ accepted", segment_accepted <= accepted,
                   f"段内 {segment_accepted} vs 整场 {accepted}（{segments} 段）")


# ── J0–J6：漂移的误差棒（P13-J，仅 `/8`）─────────────────────────────────
#
# 这一组守的核心是一句话：**`0.000` 有两种含义，"没漂移"和"尺子上只有一个刻度"。**
# 上一版把两者都印成 0，而证据里没有任何字段能把它们分开 ——
# 真机三轮、2600 个样本，八个窗口中位数全是 125000，`slope_from_windows_ppm` 全是 0。
#
# 同时它还要守住另一头：`slope_ppm` 的六位小数在 `r2 = 5.5e-05` 之下是噪声，
# 所以 SE 与 CI 必须能在证据里被**独立复算**，不能只是报告层顺手多写两个数。
CONFIDENCE_KEYS = (
    "slope_ppm_std_error",
    "slope_ci_low_ppm",
    "slope_ci_high_ppm",
    "slope_detectable",
    "window_median_exact",
    "window_median_fallback_windows",
)

# 名义 95% 双侧。**必须与 `pace_drift_trend.cxx` 里的系数一致** ——
# 两处不一致时，一份"报告说 CI 是这个、校验器算出另一个"的证据会两边都说自己对。
CI_Z = 1.96
# 证据里这些数都是 `%.6f` 打印的。复算误差上限约 `1.96 × 5e-7 + 5e-7 ≈ 1.5e-6`，
# 留一倍余量。再松就抓不到"用了别的系数"，再紧会被浮点咬。
CI_TOLERANCE = 2e-6


def check_confidence(report: Report, trend: dict[str, Any]) -> None:
    missing = [key for key in CONFIDENCE_KEYS if key not in trend]
    if missing:
        report.add("J0 误差棒字段齐全", False,
                   f"缺 {missing}：`/8` 的证据必须带这些字段，"
                   "缺了说明报告层没跟上 schema 版本")
        return
    # 齐全时也写一行。判据只在"坏的时候"出声，人会以为它本来就不存在 ——
    # 这一组是 `/8` 与 `/7` 的唯一区别，不能让它静默。
    report.add("J0 误差棒字段齐全", True,
               f"{len(CONFIDENCE_KEYS)} 个字段齐（SE / CI 两端 / detectable / "
               "窗口精确性 / 回退窗数）")

    slope = float(trend.get("slope_ppm", 0.0))
    se = float(trend.get("slope_ppm_std_error", 0.0))
    low = float(trend.get("slope_ci_low_ppm", 0.0))
    high = float(trend.get("slope_ci_high_ppm", 0.0))
    detectable = bool(trend.get("slope_detectable", False))
    win_slope = float(trend.get("slope_from_windows_ppm", 0.0))
    exact = bool(trend.get("window_median_exact", True))
    fallback = int(trend.get("window_median_fallback_windows", 0))
    samples = int(trend.get("window_sample_count", 0))
    window_count = int(trend.get("windows", 0))
    medians = [int(value) for value in (trend.get("window_median_ns") or [])]

    # ---- J1 ----
    report.add("J1 标准误有限且非负", math.isfinite(se) and se >= 0.0,
               f"SE={se}：负的 SE 只能是符号写反或对负数开了根号")

    # ---- J2 ----
    expect_low = slope - CI_Z * se
    expect_high = slope + CI_Z * se
    report.add("J2 置信带可由 slope ± 1.96·SE 复算",
               abs(low - expect_low) <= CI_TOLERANCE
               and abs(high - expect_high) <= CI_TOLERANCE,
               f"证据 [{low}, {high}] vs 复算 [{expect_low:.6f}, {expect_high:.6f}]："
               "对不上说明 CI 走的不是同一条路径，那它就不是 slope 的置信带")

    # ---- J3 ----
    #
    # 这条是"小于 X ppm"这句话的全部来源。CI 含 0 而 `slope_detectable` 为真，
    # 等于把噪声包装成漂移 —— 而 `slope_ppm` 那个非零小数正是这么来的。
    if samples < 3:
        report.add("J3 样本 < 3 ⇒ 不许判为可检出", detectable is False,
                   f"samples={samples}, detectable={detectable}："
                   "两个点必过一条直线，任何两点组合的斜率都'非零'")
    elif se > 0.0:
        expect = low > 0.0 or high < 0.0
        report.add("J3 CI 含 0 ⇔ slope_detectable 为假", detectable == expect,
                   f"CI [{low}, {high}]、detectable={detectable}")
    elif se == 0.0:
        expect = slope != 0.0
        report.add("J3 SE=0（残差为零）⇒ 检出与否只看斜率是否为零", detectable == expect,
                   f"slope={slope}、detectable={detectable}")
    else:
        # SE < 0：J1 已经红了。这里不重复判一条会误导的"SE=0"结论 ——
        # 负 SE 下 CI 的两端是交换的，`CI 含 0` 这个说法本身就没有意义。
        report.add("J3 SE<0 ⇒ 检出判定不适用（见 J1）", True,
                   f"SE={se}：先把标准误修成正数，再谈能不能检出")

    # ---- J4 ----
    #
    # `exact` 是布尔、`fallback` 是计数，两者说的是同一件事。
    # 让它们互相约束，比让读者相信其中一个更可靠。
    consistent = (fallback == 0) if exact else (0 < fallback <= window_count)
    report.add("J4 window_median_exact 与回退计数自洽", consistent,
               f"exact={exact}, fallback={fallback}, windows={window_count}")

    # ---- J5 ----
    #
    # 八个中位数全等 ⇒ 窗口回归的 y 全等 ⇒ 斜率必然恰为 0。
    # **只判这一侧**：反过来不成立（对称阶梯可以让斜率≈0 而中位数互不相等）。
    if len(medians) >= 2 and len(set(medians)) == 1:
        report.add("J5 窗口中位数全等 ⇒ 窗口斜率恰为 0", abs(win_slope) < 1e-6,
                   f"中位数全为 {medians[0]}，win_slope={win_slope}："
                   "同一批 y 回归不出非零斜率")

    # ---- J6 ----
    #
    # 本卡最要紧的一条：**存在降级窗时，那个 0 不能被读成"没漂移"。**
    # 降级意味着中位数被量化到 0.25 ms 格点；跨不出一个刻度，回归就只能报 0，
    # 而真实漂移可能是任意小于"一格/跨度"的值。这种证据可以进档案，
    # 但不能用来宣称播放没有漂移 —— 所以在这里红，而不是让它悄悄通过。
    if fallback > 0 and abs(win_slope) < 1e-6:
        report.add("J6 降级窗 + 零窗口斜率 ⇒ 该 0 不可解读", False,
                   f"fallback={fallback}、win_slope={win_slope}："
                   "中位数被量化到 0.25 ms 格点，跨不出刻度就报 0；"
                   "这份证据不能用来宣称'没有漂移'")
    else:
        report.add("J6 窗口斜率的 0 可解读", True,
                   f"fallback={fallback}、win_slope={win_slope}")


# `/9` 起必须齐全的字段。少一个，这一组判据就只能验半套，
# 而"半套绿"比全红更危险 —— 它会让人以为验过了。
HAC_KEYS = (
    "slope_ppm_std_error",          # 自 `/9` 起是 HAC 值（名字没变、语义变了）
    "slope_ppm_std_error_iid",      # 旧口径，留着只为对照
    "slope_ppm_std_error_inflation",
    "hac_lags",
    "hac_lag1_autocorrelation",
    "hac_estimated",
    "slope_first_half_ppm",
    "slope_second_half_ppm",
    "slope_halves_agree",
)


def expected_bartlett_lags(samples: int) -> int:
    """与 `pace_hac.cxx::bartlett_lags` 同一规则：`floor(4·(n/100)^(2/9))`。

    在这里**独立复算**而不是相信证据里的 `hac_lags` —— 这条判据的意义正是
    "带宽是规则算出来的，不是拍出来的"。若哪天 C++ 改了规则而这里没跟上，
    它会红，而不是跟着一起错（P13-H 的 `observed_hz` 就是栽在"校验器复算同一个公式"）。
    """
    if samples <= 0:
        return 0
    return int(4.0 * (samples / 100.0) ** (2.0 / 9.0))


def check_hac(report: Report, trend: dict[str, Any]) -> None:
    """K0–K6：HAC 那一组字段（P13-K，仅 `/9`）。"""
    missing = [key for key in HAC_KEYS if key not in trend]
    if missing:
        report.add("K0 HAC 字段齐全", False,
                   f"缺 {missing}：`/9` 的证据必须带这些字段，"
                   "缺了说明报告层没跟上 schema 版本")
        return
    report.add("K0 HAC 字段齐全", True,
               f"{len(HAC_KEYS)} 个字段齐（HAC SE / IID 旧值 / 膨胀比 / 带宽 / ρ1 / "
               "是否估出 / 半程两侧 / 符号一致）")

    se = float(trend.get("slope_ppm_std_error", 0.0))
    iid = float(trend.get("slope_ppm_std_error_iid", 0.0))
    inflation = float(trend.get("slope_ppm_std_error_inflation", 1.0))
    lags = int(trend.get("hac_lags", 0))
    rho1 = float(trend.get("hac_lag1_autocorrelation", 0.0))
    estimated = bool(trend.get("hac_estimated", False))
    first = float(trend.get("slope_first_half_ppm", 0.0))
    second = float(trend.get("slope_second_half_ppm", 0.0))
    agree = bool(trend.get("slope_halves_agree", False))
    samples = int(trend.get("window_sample_count", 0))

    # ---- K1 ----
    #
    # 带宽必须由样本数算出来。写成"相信证据里的数字"就等于没判：
    # 一个写死成 7 的实现会在 700 点上照样报 7，而它的核其实盖不住那么长的相关。
    expect_lags = expected_bartlett_lags(samples)
    # C++ 侧对 `L ≥ n−2` 有保护（那时 γ_L 只剩一两个乘积项，纯噪声），
    # 复算时要跟着钳，否则短片上会假红。
    expect_lags = min(expect_lags, max(samples - 3, 0))
    report.add("K1 hac_lags 可由样本数复算", lags == expect_lags,
               f"证据 {lags} vs 复算 {expect_lags}（samples={samples}）："
               "带宽是规则算出来的，不是拍出来的")

    # ---- K2 ----
    report.add("K2 ρ1 落在 [−1, 1]", math.isfinite(rho1) and -1.0 <= rho1 <= 1.0,
               f"ρ1={rho1}：一阶自相关按定义不会越界，越界只能是公式写错")

    # ---- K3 ----
    #
    # 膨胀比必须就是两个 SE 相除。它不是一个独立结论，而是那两个数的**读法**；
    # 若它走了另一条路径，读者看到的就是三个互不相干的数。
    #
    # 容差 1e−5 而不是 1e−9：三个数都是 `%.6f` 打印的，`se/iid` 的相对误差
    # 由三条舍入叠加而来（约 3e−6）。再紧会被浮点咬出一批假红 ——
    # P13-J 的 `J2` 就用 2e−6，那是因为它只比一个加减，这里是除法。
    expect_inflation = (se / iid) if iid > 0.0 else 1.0
    report.add("K3 膨胀比 = HAC_SE / IID_SE",
               abs(inflation - expect_inflation) <= 1e-5,
               f"证据 {inflation} vs 复算 {expect_inflation:.6f}："
               "对不上说明膨胀比不是这两个数相除得来的")

    # ---- K4 ----
    #
    # 没估出来时 `slope_ppm_std_error` 用的是 IID 旧值。这条必须成立，
    # 否则证据上"修正过"与"没修正"完全同形 —— 那正是这份档案最怕的沉默。
    if estimated:
        report.add("K4 已估计 ⇒ 可以不同于旧值", True,
                   f"estimated=true，SE {se:.6f} vs IID {iid:.6f}")
    else:
        report.add("K4 未估计 ⇒ SE 沿用 IID 旧值", se == iid,
                   f"estimated=false 而 SE={se} / IID={iid}："
                   "没估出 HAC 却报了个不一样的数，读者无法判断这是哪一个")

    # ---- K5 ----
    #
    # 本卡的**目的判据**：修正只会让带子变宽（inflation ≥ 1 时），
    # 于是它**永远不该造出新的检出**。若 HAC 说"看得见"而 IID 说"看不见"，
    # 那不是修正起了作用，是字段接反了。
    if estimated and inflation >= 1.0:
        half_iid = CI_Z * iid
        iid_low = float(trend.get("slope_ppm", 0.0)) - half_iid
        iid_high = float(trend.get("slope_ppm", 0.0)) + half_iid
        iid_detectable = (iid_low > 0.0) or (iid_high < 0.0)
        detectable = bool(trend.get("slope_detectable", False))
        report.add("K5 变宽的带不许造出新的检出", (not detectable) or iid_detectable,
                   f"inflation={inflation:.3f}、HAC 检出={detectable}、"
                   f"IID 检出={iid_detectable}：宽带里说'看得见'而窄带里说'看不见'，"
                   "只能是字段接反")
    else:
        # 显式写"不适用"，不靠沉默。判据只在坏的时候出声，人会以为它本来就不存在 ——
        # 而这里沉默的原因很具体：带子变窄时它本来就不该管"有没有造出新的检出"，
        # 那种情况下 HAC 说看得见而 IID 说看不见是**正常**的（窄带才看得见）。
        report.add("K5 不适用（膨胀比 < 1，带子变窄）", True,
                   f"estimated={estimated}、inflation={inflation:.3f}："
                   "带子变窄时这条判据没有意义，不是一个'通过'")

    # ---- K6 ----
    #
    # 半程符号是**诊断**，没有阈值也不参与判定；但它的布尔读数必须与两个数自洽。
    # 否则读者会拿一个与自己看到的两个数矛盾的值去做决定。
    expect_agree = (first > 0.0 and second > 0.0) or (first < 0.0 and second < 0.0)
    report.add("K6 半程符号一致性与两侧斜率自洽", agree == expect_agree,
               f"前半 {first:.6f}、后半 {second:.6f}、agree={agree}")


# `/10` 起必须齐全的字段。少一个这一组就只能验半套 ——
# 而"半套绿"比全红更危险，它会让人以为验过了。
HALVES_KEYS = (
    "slope_first_half_std_error_ppm",
    "slope_second_half_std_error_ppm",
    "slope_first_half_ci_low_ppm",
    "slope_first_half_ci_high_ppm",
    "slope_second_half_ci_low_ppm",
    "slope_second_half_ci_high_ppm",
    "slope_halves_gap_ppm",
    "slope_halves_pooled_error_ppm",
    "slope_halves_z",
    "slope_halves_cumulative_gap_ns",
    "slope_first_half_span_ns",
    "slope_second_half_span_ns",
    "slope_halves_resolution_ppm",
    "slope_halves_budget_equivalent_ppm",
    "slope_halves_resolution_sufficient",
    "slope_halves_relation",
)

BUDGET_KEYS = (
    "drift_accumulated_ns",
    "drift_accumulated_upper_ns",
    "drift_budget_ns",
    "drift_verdict",
)

# 额度：45 ms。**必须与 `pace_drift_budget.hpp` 的 `kAudibleThresholdNs` 相等**。
#
# 写死在这里的全部意义就是"它被独立核对过一次"。若哪天 C++ 把 45 改成 40，
# 这里会红 —— 而如果这里改成"从证据里读",它就只会跟着一起错。
# P13-H 的 `observed_hz` 正是栽在"校验器复算同一个公式"上。
AUDIBLE_THRESHOLD_NS = 45_000_000

# 三态。顺序不重要，但**必须只有这三个** —— 报告层写出第四个值时，
# 读者会以为那是一个已知的判定，而实际上它谁也没定义过。
BUDGET_VERDICTS = ("unknown", "within_budget", "over_budget")


def check_halves_relation(report: Report, trend: dict[str, Any]) -> None:
    """O0–O7：两个半程是不是同一条线（P13-O，仅 `/13`）。

    这一组替掉一个说过谎的布尔。判据是 `gap > 1.96 × pooled`，
    也就是"差值相对于差值自身的抽样误差有多大" —— 不是"置信区间端点相交"，
    因为端点相交会被"宽的一侧把窄的一侧整个包住"骗过去。

    三态里 `unknown` 必须与"算不出来"对应：`gap` 与 `pooled` 都是 0 时，
    它不是"一致"，是"没量"。
    """
    missing = [key for key in HALVES_KEYS if key not in trend]
    if missing:
        report.add("O0 半程一致性字段齐全", False,
                   f"缺 {missing}：`/13` 的证据必须带这些字段，"
                   "缺了说明报告层没跟上 schema 版本")
        return
    report.add("O0 半程一致性字段齐全", True, f"{len(HALVES_KEYS)} 个字段齐")

    se1 = float(trend["slope_first_half_std_error_ppm"])
    se2 = float(trend["slope_second_half_std_error_ppm"])
    first = float(trend["slope_first_half_ppm"])
    second = float(trend["slope_second_half_ppm"])
    gap = float(trend["slope_halves_gap_ppm"])
    pooled = float(trend["slope_halves_pooled_error_ppm"])
    z_raw = trend["slope_halves_z"]
    relation = str(trend["slope_halves_relation"])
    resolution = float(trend["slope_halves_resolution_ppm"])
    budget_eq = float(trend["slope_halves_budget_equivalent_ppm"])
    sufficient = bool(trend["slope_halves_resolution_sufficient"])
    cumulative = int(trend["slope_halves_cumulative_gap_ns"])
    span1 = int(trend["slope_first_half_span_ns"])
    span2 = int(trend["slope_second_half_span_ns"])
    verdict = str(trend.get("drift_verdict", ""))

    z = None if z_raw is None else float(z_raw)

    # 复算容差：所有 ppm 都是 `%.6f` 打印的（绝对误差 5e-7）。
    tol = 1e-5

    # ---- O1 ----
    report.add("O1 两侧标准误有限且非负",
               math.isfinite(se1) and math.isfinite(se2) and se1 >= 0.0 and se2 >= 0.0,
               f"se1={se1:.6f} se2={se2:.6f}")

    # ---- O2 ----
    expect_low1 = first - 1.96 * se1
    expect_high1 = first + 1.96 * se1
    expect_low2 = second - 1.96 * se2
    expect_high2 = second + 1.96 * se2
    ci_ok = (
        abs(float(trend["slope_first_half_ci_low_ppm"]) - expect_low1) <= tol * (1 + abs(expect_low1))
        and abs(float(trend["slope_first_half_ci_high_ppm"]) - expect_high1) <= tol * (1 + abs(expect_high1))
        and abs(float(trend["slope_second_half_ci_low_ppm"]) - expect_low2) <= tol * (1 + abs(expect_low2))
        and abs(float(trend["slope_second_half_ci_high_ppm"]) - expect_high2) <= tol * (1 + abs(expect_high2))
    )
    report.add("O2 两侧 CI = slope ± 1.96·SE", ci_ok,
               f"前半 [{expect_low1:.3f}, {expect_high1:.3f}] / 后半 [{expect_low2:.3f}, {expect_high2:.3f}]")

    # ---- O3 ----
    expect_pooled = math.sqrt(se1 * se1 + se2 * se2)
    report.add("O3 pooled = sqrt(se1² + se2²)",
               abs(pooled - expect_pooled) <= tol * (1 + expect_pooled),
               f"证据 {pooled:.6f} vs 复算 {expect_pooled:.6f}")

    # ---- O4 ----
    #
    # `z` 为 null 是合法的：两侧残差都恒为 0 且斜率不同时 C++ 给 +inf，
    # 而 `inf` 不是合法 JSON，报告层写 null（见 av_sync_report.cxx 的注释）。
    # 那种情况只在 `pooled == 0` 时出现 —— 别的 null 都是缺数据。
    if z is None:
        z_ok = pooled == 0.0 and gap > 0.0
        z_detail = f"z=null（pooled={pooled:.6f}, gap={gap:.6f}）—— 只在两侧残差恒 0 时合法"
    elif pooled > 0.0:
        z_ok = abs(z - gap / pooled) <= tol * (1 + abs(z))
        z_detail = f"证据 z={z:.6f} vs 复算 {gap / pooled:.6f}"
    else:
        z_ok = z == 0.0 and gap == 0.0
        z_detail = f"pooled=0 且 gap={gap:.6f} ⇒ z 应为 0，证据 {z}"
    report.add("O4 z = gap / pooled 可复算", z_ok, z_detail)

    # ---- O5 ----
    if relation == "inconsistent":
        rel_ok = (z is None) or z > 1.96
        rel_detail = f"inconsistent：z={z}（null 视作 +inf）"
    elif relation == "consistent":
        rel_ok = (z is not None) and z <= 1.96
        rel_detail = f"consistent：z={z} 应 ≤ 1.96"
    elif relation == "unknown":
        # 未估出时 C++ 保持 SE=0 且不填 gap 以外的误差字段。
        rel_ok = se1 == 0.0 and se2 == 0.0 and pooled == 0.0
        rel_detail = f"unknown：se1={se1} se2={se2} pooled={pooled}（应全为 0，表示没量出来）"
    else:
        rel_ok = False
        rel_detail = f"未知三态取值 {relation!r}"
    report.add("O5 三态与 z 自洽", rel_ok, rel_detail)

    # ---- O6 ----
    #
    # `cumulative_gap_ns` 用两半程**各自的真实跨度**。用"半程时长"会把它算成
    # 另一个数（两段长度由 n/2 的整数除法决定，末尾那个点的归属会让它们差一格）。
    expect_gap_ns = abs(first * span1 / 1e6 - second * span2 / 1e6)
    # 容差：两个斜率各 5e-7 ppm 的打印误差，各自乘自己的跨度，
    # 再加 C++ 侧 `+0.5` 的四舍五入。
    gap_tol = 2.0 + (span1 + span2) * 1e-12
    report.add("O6 累积差 = |first×span1 − second×span2|",
               abs(cumulative - expect_gap_ns) <= gap_tol,
               f"证据 {cumulative} vs 复算 {expect_gap_ns:.1f} ns（跨度 {span1}/{span2}）")

    # ---- O7 ----
    #
    # 两条：① `inconsistent` 必须把 `drift_verdict` 降为 `unknown`
    # （否则那个 `within_budget` 是拿一条错的直线乘出来的）；
    # ② `resolution_sufficient` 必须与两个 ppm 自洽 —— 它是"空口说一致"的
    # 结构防线：`resolution` 比额度还大时，"一致"这个词没有信息量。
    downgrade_ok = relation != "inconsistent" or verdict == "unknown"
    report.add("O7a inconsistent ⇒ drift_verdict = unknown", downgrade_ok,
               f"relation={relation} verdict={verdict}")

    expect_sufficient = budget_eq > 0.0 and resolution <= budget_eq
    report.add("O7b resolution_sufficient 与两个 ppm 自洽",
               sufficient == expect_sufficient,
               f"resolution={resolution:.6f} budget={budget_eq:.6f} "
               f"证据={sufficient} 复算={expect_sufficient}")


def check_budget(report: Report, trend: dict[str, Any]) -> None:
    """L0–L5：累积漂移与额度判读（P13-L，仅 `/10`）。

    这一组守的是一句被读错了两张卡的话：`detectable = true` ≠ 漂移要紧。
    所以它必须能独立把 `ppm` 积回纳秒，并独立复算那个上界 ——
    只相信证据里的三个整数，等于把"可复算"降级成"可抄写"。
    """
    missing = [key for key in BUDGET_KEYS if key not in trend]
    if missing:
        report.add("L0 累积量与额度的字段齐全", False,
                   f"缺 {missing}：`/10` 的证据必须带这些字段，"
                   "缺了说明报告层没跟上 schema 版本")
        return
    report.add("L0 累积量与额度的字段齐全", True,
               f"{len(BUDGET_KEYS)} 个字段齐（点估计 / 保守上界 / 额度 / 三态判定）")

    slope = float(trend.get("slope_ppm", 0.0))
    span = int(trend.get("span_ns", 0))
    low = float(trend.get("slope_ci_low_ppm", 0.0))
    high = float(trend.get("slope_ci_high_ppm", 0.0))
    point = int(trend.get("drift_accumulated_ns", 0))
    upper = int(trend.get("drift_accumulated_upper_ns", 0))
    budget = int(trend.get("drift_budget_ns", 0))
    verdict = str(trend.get("drift_verdict", ""))
    usable = bool(trend.get("usable", False))

    # 复算容差：`slope_ppm` 是 `%.6f` 打印的，误差 5e-7 ppm；
    # 乘 `span/1e6` 之后变成 `span × 5e-13`（102 s 的窗口 ⇒ 0.05 ns）。
    # 再加 `llround` 的半个格点。留一倍余量到 2 ns + 2e-12·span。
    tolerance = 2.0 + abs(span) * 2e-12

    # ---- L1 ----
    #
    # 单位唯一可能错的地方：漏掉那个 1e6（ppm 是 µs/s，ns 是 ns）。
    # 漏掉时累积量会大 100 万倍 —— 一份 0.7 ms 的证据会写成 700 s。
    expect_point = slope * span / 1e6
    report.add("L1 点估计 = slope_ppm × span_ns / 1e6",
               abs(point - expect_point) <= tolerance,
               f"证据 {point} vs 复算 {expect_point:.3f}（跨度 {span} ns）："
               "对不上只能是单位或跨度取错了对象")

    # ---- L2 ----
    #
    # 上界不许比点估计乐观。它是"最坏能坏到哪"，比点估计还小就不叫上界。
    # 这条同时挡住"用点估计当上界"之外的另一种错：把上界算成了半宽。
    report.add("L2 保守上界 ≥ 点估计绝对值", upper >= abs(point),
               f"上界 {upper} vs |点估计| {abs(point)}："
               "上界比点估计还小，读者会以为最坏情况比估计值更好")

    # ---- L3 ----
    #
    # 上界必须来自置信带**离 0 更远**的那一端，不是 `ci_high`。
    # 负斜率时两者不同：`ci_high` 可能就在 0 附近，拿它算出来的上界会小一个数量级。
    farthest = max(abs(low), abs(high))
    expect_upper = farthest * span / 1e6
    report.add("L3 上界 = max(|ci_low|,|ci_high|) × span_ns / 1e6",
               abs(upper - expect_upper) <= tolerance,
               f"证据 {upper} vs 复算 {expect_upper:.3f}"
               f"（CI [{low}, {high}]、跨度 {span} ns）："
               "取 ci_high 而不是两端较远者，会让负斜率的证据凭空乐观")

    # ---- L4 ----
    report.add("L4 额度常数 = 45 ms", budget == AUDIBLE_THRESHOLD_NS,
               f"证据 {budget} vs 期望 {AUDIBLE_THRESHOLD_NS}："
               "额度是判定唯一的尺子，它一变所有历史结论都失去可比性")

    # ---- L5 ----
    #
    # 三态必须与它自己的两个输入（上界、额度）以及可用性自洽。
    # `unknown` 单独一支：它**不是**"上界很小"，是"这份趋势没资格下结论"，
    # 把它和 `within_budget` 混成一个 `false` 就是让失败伪装成通过。
    if verdict not in BUDGET_VERDICTS:
        report.add("L5 三态与上界/额度/可用性自洽", False,
                   f"verdict={verdict!r} 不在 {BUDGET_VERDICTS} 里："
                   "报告层写出了一个没有定义过的判定")
    elif verdict == "unknown":
        report.add("L5 unknown ⇔ 趋势不可用", not usable,
                   f"verdict=unknown、usable={usable}："
                   "趋势可用却判 unknown，是把一份能下结论的证据扔掉")
    elif verdict == "within_budget":
        report.add("L5 within_budget ⇔ 上界未越额度且趋势可用",
                   usable and upper <= budget,
                   f"上界 {upper} vs 额度 {budget}、usable={usable}")
    else:  # over_budget
        report.add("L5 over_budget ⇔ 上界越过额度且趋势可用",
                   usable and upper > budget,
                   f"上界 {upper} vs 额度 {budget}、usable={usable}："
                   "越过额度靠的是上界，不是点估计 —— 用点估计判会让一份"
                   "'可能听得出来'的证据被判成没问题")


def check_segment_authority(report: Report, sampling: dict[str, Any],
                            trend: dict[str, Any]) -> None:
    """M0–M4：段边界的权威是锚点重建（P13-M，仅 `/11`）。

    这一组不是"多验几个字段"，是补一条**判据本身看不出来**的漏洞：
    上一版 `segments` 只由 pts 倒退产生，而向前 seek 时 pts 在**前进**，
    于是两段被拼进同一个回归 —— P13-L 那份缺陷证据报着 `segments = 1`、
    `span = 112.367 s`、`observed_hz = 18.466`，而实际是两段之间隔着十秒空白。

    校验器**无法**从 JSON 判断"这个 1 是不是漏了"（它没有 pts 序列）。
    能做的是把这件事的**痕迹**变成可核对的结构：一段之内最宽的空洞有多宽。
    于是"漏切"不再无声 —— 它会以 `segment_contiguous = false` 或
    `segment_largest_gap_cells` 过大出现在证据里。

      M0  三个字段齐全（`segments` / `segment_largest_gap_cells` / `segment_contiguous`）。
          少一个这一组只能验半套，而"半套绿"比全红更危险。
      M1  `segment_contiguous ⟺ segment_largest_gap_cells ≤ 20`。
          阈值在 Python 侧独立写死，不从证据读 —— 否则它只是复述，不是核对。
      M2  `segment_largest_gap_cells ≤ floor(span_ns / interval)`。
          **单位守卫**：这一格是"格点"，误写成纳秒会立刻越过这个上界。
      M3  `segment_largest_gap_cells ≤ sampling.unobserved_cells`。
          **记账守卫**：最宽的那个洞一定在那笔累计账里。反过来说明这两个数
          各算各的（比如切段时把最大值清了而累计没清，或者反过来）。
      M4  单段时 `segment_largest_gap_cells > 0 ⟺ unobserved_cells > 0`。
          **接线守卫**：只有一段时所有空格都属于这一段，两个数必须同为 0 或同为正。
          多段时它们本就可以不相等（前面几段的空格也算进累计），报"不适用"。
    """
    interval = EXPECTED_SAMPLE_INTERVAL_NS
    missing = [key for key in ("segments", "segment_largest_gap_cells", "segment_contiguous")
               if key not in trend]
    if missing:
        report.add("M0 段边界三字段齐全", False,
                   f"缺 {missing}：`/11` 的证据必须带这些字段，"
                   "缺了说明报告层没跟上 schema 版本")
        return

    segments = int(trend["segments"])
    gap_cells = int(trend["segment_largest_gap_cells"])
    contiguous = bool(trend["segment_contiguous"])
    span_ns = int(trend.get("span_ns", 0))
    unobserved = int(sampling.get("unobserved_cells", 0))

    report.add("M0 段边界三字段齐全", True,
               f"segments={segments}、largest_gap={gap_cells} 格、contiguous={contiguous}")

    # ---- M1 ----
    #
    # 布尔与格数必须互为定义。两者若各算各的，读者就没有办法判断
    # "false 到底是洞太宽，还是这一格根本没接线"。
    expect_contiguous = gap_cells <= MAX_CONTIGUOUS_GAP_CELLS
    report.add("M1 contiguous ⟺ 空洞 ≤ 20 格", contiguous == expect_contiguous,
               f"contiguous={contiguous}、gap={gap_cells} 格、阈值 {MAX_CONTIGUOUS_GAP_CELLS}："
               "两者对不上说明这一格不是由格数判出来的")

    # ---- M2 ----
    #
    # 一条直线能横跨的最宽空洞不可能超过它自己的跨度。
    # 这条是**单位守卫**：把格数写成纳秒（10 s 的洞写成 1e10），
    # 它会立刻越过 `span/interval` 这个上界。
    if interval > 0:
        ceiling_cells = span_ns // interval
        report.add("M2 空洞不超过跨度（单位是格点）", 0 <= gap_cells <= ceiling_cells,
                   f"gap={gap_cells} 格 vs 上界 {ceiling_cells} 格"
                   f"（span={span_ns}ns / interval={interval}ns）："
                   "越界只有两种解释 —— 单位写错，或这一格记的不是本段")

    # ---- M3 ----
    #
    # 最宽的那个洞是累计账里的一部分。`unobserved_cells` 是整场累计（各段相加），
    # 段内最大值不可能超过它 —— 这条抓"切段时清了最大值却漏了累计"这类错配。
    report.add("M3 空洞不超过整场空格总数", gap_cells <= unobserved,
               f"gap={gap_cells} 格 vs unobserved_cells={unobserved}："
               "段内最宽的洞不可能比整场所有洞加起来还宽")

    # ---- M4 ----
    #
    # 单段时两个数必须同零或同正（恒等式级别的接线核对）。
    # 多段时降成"不适用"而不是硬判：前面几段的空格也算进累计，
    # 那时 `gap == 0` 而 `unobserved > 0` 是**正常**的（最后一段一个洞都没有）。
    if segments == 1:
        report.add("M4 单段时空洞与空格总数同零/同正",
                   (gap_cells > 0) == (unobserved > 0),
                   f"segments=1 而 gap={gap_cells}、unobserved={unobserved}："
                   "只有一段时两者必须同时为 0 或同时为正，否则是这一格没接线")
    else:
        report.add("M4 不适用（多段，累计含前面几段）", True,
                   f"segments={segments}：`unobserved_cells` 是整场累计，"
                   f"`gap={gap_cells}` 只描述最后一段，两者本就可以不等")


def locate_bucket(hist: dict[str, Any], q: float) -> tuple[int, int]:
    """从导出的分布复算"第 q 分位数落在哪个桶、那桶多少样本"。

    与 `SignedHistogram::locate()` 逐条对齐：秩取**上整**、下溢整块先跳过、
    桶内累计 `>=` 秩即命中（`>=` 不是 `>`，因为秩正好压在第 k 桶的末位时
    它属于第 k 桶）。三者错一条，复算出的桶号就会偏一格。
    """
    count = int(hist["count"])
    if count <= 0:
        return (-2, 0)  # kNoSamples
    bucket_count = int(hist["bucket_count"])
    if q <= 0.0:
        return (-1, int(hist["below"]))
    if q >= 1.0:
        return (bucket_count, int(hist["above"]))

    rank = int(q * count)
    if rank < q * count:
        rank += 1
    if rank < 1:
        rank = 1

    below = int(hist["below"])
    if rank <= below:
        return (-1, below)
    accumulated = below
    buckets = hist["buckets"]
    for index in range(bucket_count):
        bucket_count_here = int(buckets.get(str(index), 0))
        accumulated += bucket_count_here
        if rank <= accumulated:
            return (index, bucket_count_here)
    return (bucket_count, int(hist["above"]))


def replay_p50_range(hist: dict[str, Any]) -> tuple[int, int]:
    """从导出的分布复算 `p50` 的**可能范围** `[low, high)`。

    与 `SignedHistogram::percentile_range()` 对齐：桶内时取"桶界 ∩ [min, max]"，
    下界取大、上界取 `max + 1`（开界）。落在溢出区或端点时范围退化成那个值本身。
    """
    count = int(hist["count"])
    if count <= 0:
        return (0, 0)
    p50 = replay_percentile(hist, 0.50)
    index, _ = locate_bucket(hist, 0.50)
    if not 0 <= index < int(hist["bucket_count"]):
        return (p50, p50)
    width = int(hist["bucket_width_ns"])
    low_ns = int(hist["low_ns"])
    low = max(low_ns + index * width, int(hist["min_ns"]))
    high = min(low_ns + (index + 1) * width, int(hist["max_ns"]) + 1)
    return (low, high)


def check_quantile_resolution(report: Report, quantiles: dict[str, Any],
                              trend: dict[str, Any]) -> None:
    """N0–N7：分位数必须自己说出精度（P13-N，仅 `/12`）。

    这一组补的漏洞不在数值上，在**读懂方式**上：`quantiles.<口径>.p50` 是直方图
    桶中心，真机上恒为 125 000；而 `first/last_window_median_ns` 是原始值选出的
    真值 74 835。同一份证据里两个 p50 差 66%，而 `/11` 的 JSON 里没有一个字段
    能阻止读者把它读成"两份测量打架"。

    它们不打架 —— 真值确实在 [0, 250 000) 那一桶里，是桶中心没本事说出来。
    但"没本事"这件事必须印在证据上，否则每一轮都要靠人解释一次。

      N0  每个分位块都有四格（范围下界/上界/桶内样本数/桶宽）。
          少一格这一组只能验半套，而"半套绿"比全红更危险。
      N1  `bucket_width_ns == 250000`。在 Python 侧独立写死，不从证据读 ——
          读出来再和它自己比只叫复述。
      N2  `p50 ∈ [range_low, range_high]`（含端点：溢出时两端相等）。
      N3  `0 ≤ p50_bucket_samples ≤ samples`。
      N4  `range_high − range_low ≤ 桶宽`。钳制只会让范围**更窄**；
          宽于一个桶说明这一格不是由桶界与 `[min, max]` 求交得来的。
      N5  `p50_bucket_samples` 等于**从 `histogram` 复算**的那桶计数。
          这是接线守卫：恒填 0、恒填总数、或者填了 `below`，这一条立刻红。
      N6  范围与**从 `histogram` 复算**的范围逐位相等。N5 钉桶号，这条钉交界。
      N7  `median_of_window_medians_ns` 等于 `window_median_ns` 排序后第
          `(n−1)//2` 个。秩口径写成 `n//2` 时（8 个窗取第 5 个而不是第 4 个）
          这条会红 —— 而两个中位数用两套秩，就没法放在同一张表里比了。
    """
    blocks: list[tuple[str, dict[str, Any]]] = []
    if isinstance(quantiles, dict):
        for name, block in quantiles.items():
            if isinstance(block, dict):
                blocks.append((f"quantiles.{name}", block))
    window_quantiles = trend.get("window_quantiles")
    if isinstance(window_quantiles, dict):
        blocks.append(("trend.window_quantiles", window_quantiles))

    if not blocks:
        report.add("N0 有分位块可验", False,
                   "一个分位块都没有：`/12` 的证据至少要有一份分位数")
        return

    required = ("p50_range_low_ns", "p50_range_high_ns",
                "p50_bucket_samples", "bucket_width_ns")
    missing = [f"{label}.{key}" for label, block in blocks
               for key in required if key not in block]
    report.add("N0 分位精度四格齐全", not missing,
               f"缺 {missing}：`/12` 的每个分位块都要带这四格，"
               "缺了说明报告层没跟上 schema 版本" if missing
               else f"{len(blocks)} 个分位块各带四格")
    if missing:
        return

    # ---- N1 ----
    wrong_width = [(label, int(block["bucket_width_ns"])) for label, block in blocks
                   if int(block["bucket_width_ns"]) != QUANTILE_BUCKET_WIDTH_NS]
    report.add("N1 桶宽恒为 250000 ns", not wrong_width,
               f"{wrong_width}：桶宽这一格是**定义**（`kBucketWidthNs`），"
               f"不是观测，恒等于 {QUANTILE_BUCKET_WIDTH_NS}"
               if wrong_width else f"全部等于 {QUANTILE_BUCKET_WIDTH_NS}")

    # ---- N2 ----
    outside = [(label, int(block["p50"]), int(block["p50_range_low_ns"]),
                int(block["p50_range_high_ns"])) for label, block in blocks
               if not int(block["p50_range_low_ns"]) <= int(block["p50"])
               <= int(block["p50_range_high_ns"])]
    report.add("N2 p50 落在它自己声明的范围里", not outside,
               f"{outside}：声明的范围不含它要解释的那个数，"
               "读者只会认定整份证据不可信" if outside
               else "每个 p50 都在自己的 [low, high] 内")

    # ---- N3 ----
    bad_share = [(label, int(block["p50_bucket_samples"]), int(block["samples"]))
                 for label, block in blocks
                 if not 0 <= int(block["p50_bucket_samples"]) <= int(block["samples"])]
    report.add("N3 桶内样本数不超过总样本数", not bad_share,
               f"{bad_share}：落在一个桶里的样本不可能多于全部样本" if bad_share
               else "全部 ≤ samples")

    # ---- N4 ----
    too_wide = [(label, int(block["p50_range_high_ns"]) - int(block["p50_range_low_ns"]))
                for label, block in blocks
                if int(block["p50_range_high_ns"]) - int(block["p50_range_low_ns"])
                > QUANTILE_BUCKET_WIDTH_NS]
    report.add("N4 可能范围不宽于一个桶", not too_wide,
               f"{too_wide}：与 `[min, max]` 求交只会让范围变窄，"
               f"宽于 {QUANTILE_BUCKET_WIDTH_NS} ns 说明这一格不是求和交得来的"
               if too_wide else "全部 ≤ 一个桶宽")

    # ---- N5 / N6：从分布复算 ----
    #
    # 这两条是这一组里唯一"不信任何手填数字"的部分：桶号、桶计数、范围
    # 全部从 `histogram.buckets` 重算一遍。`/11` 及以前根本做不到这件事 ——
    # 那时证据里只有 `p50` 一个数，没有桶号，复算方无从核对它落在哪儿。
    replayable = [(label, block) for label, block in blocks
                  if isinstance(block.get("histogram"), dict)
                  and int(block["histogram"].get("count", 0)) > 0]
    unverified = [label for label, block in blocks
                  if not (isinstance(block.get("histogram"), dict)
                          and int(block["histogram"].get("count", 0)) > 0)]
    if unverified:
        report.add("N5 从分布复算桶内样本数", True,
                   f"{unverified} 的样本数为 0，没有桶可复算（空分布谈不上精度）")
        report.add("N6 从分布复算可能范围", True, f"{unverified} 同上")
    else:
        bad_bucket = []
        bad_range = []
        for label, block in replayable:
            hist = block["histogram"]
            _, expect_samples = locate_bucket(hist, 0.50)
            if int(block["p50_bucket_samples"]) != expect_samples:
                bad_bucket.append((label, int(block["p50_bucket_samples"]), expect_samples))
            expect_low, expect_high = replay_p50_range(hist)
            if (int(block["p50_range_low_ns"]), int(block["p50_range_high_ns"])) != \
                    (expect_low, expect_high):
                bad_range.append((label,
                                  (int(block["p50_range_low_ns"]),
                                   int(block["p50_range_high_ns"])),
                                  (expect_low, expect_high)))
        report.add("N5 从分布复算桶内样本数", not bad_bucket,
                   f"{bad_bucket}（证据值 vs 从 `histogram` 复算）："
                   "对不上说明这一格不是由分布算出来的" if bad_bucket
                   else "全部与分布复算一致")
        report.add("N6 从分布复算可能范围", not bad_range,
                   f"{bad_range}：范围必须等于「桶界 ∩ [min, max]」" if bad_range
                   else "全部与分布复算逐位相等")

    # ---- N7 ----
    medians = trend.get("window_median_ns")
    reference = trend.get("median_of_window_medians_ns")
    if not isinstance(medians, list) or not medians:
        report.add("N7 整批参照与窗口中位数自洽", True,
                   "这一轮没有窗口中位数数组，无整批参照可验")
    elif reference is None:
        report.add("N7 整批参照与窗口中位数自洽", True,
                   "`median_of_window_medians_ns` 缺席（有窗退回桶中心时不出版），"
                   "符合 `/12` 的规则")
    else:
        ordered = sorted(int(value) for value in medians)
        expected = ordered[(len(ordered) - 1) // 2]
        report.add("N7 整批参照与窗口中位数自洽", int(reference) == expected,
                   f"证据 {int(reference)} vs 排序后第 {(len(ordered) - 1) // 2} 个 {expected}："
                   "秩口径必须是 (n−1)//2，与 `percentile_ns(0.50)` 的上中位一致"
                   if int(reference) != expected
                   else f"{len(ordered)} 个窗口中位数 → 第 {(len(ordered) - 1) // 2} 个 = {expected}")


def check_file(path: Path) -> tuple[Report, dict[str, Any] | None]:
    report = Report(path)
    try:
        doc = load(path)
    except Exception as exc:  # noqa: BLE001 - 解析失败本身就是一条结论
        report.add("JSON 可解析", False, str(exc))
        return report, None

    schema = doc.get("schema", "")
    if schema not in SUPPORTED_SCHEMAS:
        # `/1` 没有抽稀块，`/2` 有抽稀但分位数还挂在顶层（键名不带口径）。
        # 都不报失败也不报通过 —— 报"不适用"，并说明原因。
        # 判据 D1–D5 是对 `/3` 起的结构说的，拿它去判旧版只会产出一批假红。
        report.not_applicable = (
            f"schema 是 {schema!r}：D/E/F 判据只适用于 {SUPPORTED_SCHEMAS}"
            "（/2 的分位数在顶层、键名不带口径，已由 /3 取代）"
        )
        return report, doc

    report.add("schema 受支持", True, schema)

    drift = doc.get("av_drift", {})

    # ---- E1：顶层不许有裸分位数 ----
    bare = [key for key in BARE_QUANTILE_KEYS if key in drift]
    report.add(
        "E1 av_drift 顶层无裸分位数",
        not bare,
        f"发现 {bare}：分位数必须挂在 quantiles.<口径> 下" if bare
        else "没有不带口径就能拿到的 p99",
    )

    sampling = drift.get("sampling", {})
    if not sampling:
        report.add("带 sampling 块", False, "sync/3 必须声明采样口径，否则分位数不可比")
        return report, doc
    report.add("带 sampling 块", True)

    mode = sampling.get("mode", "")
    report.add("sampling.mode 合法", mode in MODES, f"mode={mode!r}")

    quantiles = drift.get("quantiles")
    report.add("带 quantiles 块", isinstance(quantiles, dict),
               f"实际 {type(quantiles).__name__}: 一份分位数都没有时也必须是空对象，不是缺键")

    # ---- E2：cross_frame_rate_set 必须指到真实存在的那一份 ----
    #
    # 这一格是"能不能进 30fps vs 60fps 那张表"的唯一答案。它有三种合法取值：
    #   - 抽稀成立 → 指向 quantiles.media_time_decimated，且该路径必须解析得到；
    #   - 抽稀不成立 → null（逐帧的数不可比，写 "per_frame" 就是给假答案）；
    #   - 不可测 → null。
    if "cross_frame_rate_set" not in sampling:
        report.add("E2 声明 cross_frame_rate_set", False, "缺这一格，读者只能猜哪份能比")
    else:
        pointer = sampling["cross_frame_rate_set"]
        if mode == MODE_DECIMATED:
            target = f"quantiles.{MODE_DECIMATED}"
            resolved = isinstance(resolve_path(drift, target), dict)
            if pointer != target:
                detail = f"cross_frame_rate_set={pointer!r}，应为 {target!r}"
            elif not resolved:
                detail = f"{target} 解析不到"
            else:
                detail = f"{target} 可解析且存在"
            report.add("E2 抽稀成立 ⇒ 指向抽稀那一份", pointer == target and resolved, detail)
        else:
            report.add("E2 无可比口径 ⇒ 为 null", pointer is None,
                       f"mode={mode!r} 却声称 {pointer!r} 可跨帧率比较")

    if mode == MODE_NOT_MEASURED:
        # 这一轮 av_drift 本来就不可测（没音轨 / 时钟是 system / 走了 surface_at_time）。
        # 重要的是**别把空序列说成"没有漂移"**，也别给一份空分布让平均值算得下去。
        report.add("not_measured ⇒ measurable=false", not bool(drift.get("measurable", True)),
                   f"measurable={drift.get('measurable')}")
        report.add("not_measured ⇒ quantiles 为空对象", quantiles == {},
                   f"quantiles={quantiles!r}：不可测时不许有分位数，空对象才说明「一份都没有」")
        report.add("not_measured ⇒ 带 unavailable_reason",
                   bool(str(drift.get("unavailable_reason", ""))),
                   "不可测却不说原因，读者会把它当成「没有漂移」")
        report.add("not_measured ⇒ 也带 reason",
                   bool(str(sampling.get("reason", ""))),
                   "口径名为 not_measured 时必须解释为什么")
        return report, doc

    # ---- 抽稀自洽性 ----
    observed = int(sampling.get("frames_observed", 0))
    accepted = int(sampling.get("accepted", 0))
    skipped = int(sampling.get("skipped", 0))
    report.add("frames_observed = accepted + skipped",
               observed == accepted + skipped,
               f"{observed} vs {accepted}+{skipped}={accepted + skipped}")

    report.add("sampling.interval_ns 是 50ms",
               int(sampling.get("interval_ns", 0)) == EXPECTED_SAMPLE_INTERVAL_NS,
               f"实际 {sampling.get('interval_ns')}")
    report.add("sampling.target_hz 是 20",
               int(sampling.get("target_hz", 0)) == EXPECTED_TARGET_HZ,
               f"实际 {sampling.get('target_hz')}")

    trend = drift.get("trend", {})
    span = int(trend.get("span_ns", 0))
    # P13-H：分母的版本差异。
    # `/6` 起 `observed_hz` 与结构门都用 `sampling.media_span_ns`（**整场累计**）；
    # `/3`–`/5` 用的是 `trend.span_ns`（**窗口**跨度，封顶 102.35 s）。
    # 两个口径混用正是本卡修的缺陷，在 8 s / 35 s 片源上恰好相等，所以旧证据全绿。
    has_media_span = schema in SCHEMA_WITH_MEDIA_SPAN
    rate_span = int(sampling.get("media_span_ns", 0)) if has_media_span else span
    # `/5` 起"降级"不再等价于 `accepted == 0`：1 个样本也降级。
    # 旧写法 `else: accepted == 0` 会把合法的单样本降级判成失败 ——
    # 本卡的正例变异 M4 第一次跑就撞上了（同一张表只改了一半的第四次发作）。
    min_accepted = MIN_DECIMATED_SAMPLES if schema in SCHEMA_WITH_FLOOR else 1
    if accepted >= min_accepted and rate_span > 0:
        expected_hz = accepted * 1e9 / rate_span
        actual_hz = float(sampling.get("observed_hz", 0.0))
        ok = abs(actual_hz - expected_hz) <= max(1e-6, 0.01 * expected_hz)
        report.add("observed_hz = accepted / span", ok,
                   f"证据 {actual_hz:.3f} vs 重算 {expected_hz:.3f}")
    else:
        # 抽稀不成立时报告层写 0.0（span 为 0 时无从除）。这里断言它确实是 0，
        # 而不是无条件通过 —— "跳过"和"通过"在报告里长得一样，但后者是假判据。
        actual_hz = float(sampling.get("observed_hz", 0.0))
        report.add("抽稀不成立 ⇒ observed_hz 为 0", actual_hz == 0.0,
                   f"accepted={accepted}, span={rate_span}ns, observed_hz={actual_hz:.3f}："
                   "抽稀没成立却报了个节奏")

    # ---- H1–H4：`observed_hz` 的分子分母同口径（`/6`）----
    # 只对 `/6` 判：`/3`–`/5` 的分母是窗口跨度，拿新判据去判它们会产出一批假红
    # （那些证据的片长都在 102.4 s 以内，所以旧口径碰巧对 —— 但"碰巧对"不是"对"）。
    if has_media_span:
        check_media_span(report, sampling, trend, accepted)

    # ---- 抽稀那一份：有样本就必须在，没样本就必须不在 ----
    #
    # `/5` 起门槛变了：不是"有没有样本"，而是"样本够不够证明媒体时间在推进"。
    # 旧判据 `accepted > 0 ⇒ 块必须存在` 在 `/3` `/4` 上是够的，但那是因为
    # 那两版根本没有"1 个样本"这种形态被认真想过 —— 缺 pts 的流会产出它，
    # 而它长得完全合法（见 `68-p13g-design.md` §二）。
    # `/3` `/4` 继续用旧判据：旧证据不重写，也不拿新门槛去判它们。
    decimated_set = (quantiles or {}).get(MODE_DECIMATED)
    # `per_frame` 必须在这里就取出来：G2 要问"降级时逐帧那份在不在"，
    # 而它的渲染块在下面。两处取同一个值，不各取一次。
    per_frame = (quantiles or {}).get(MODE_PER_FRAME, {})
    if schema in SCHEMA_WITH_FLOOR:
        # 结构门问的是"媒体时间在这批样本上有没有推进"——这是**整场**问题，
        # 所以 `/6` 起收累计跨度。`/5` 只有窗口跨度可用，那时两者恒等。
        check_decimated_floor(report, mode, accepted, rate_span, decimated_set, per_frame)
        if isinstance(decimated_set, dict):
            if decimated_set.get("histogram"):
                replay = replay_quantiles(decimated_set["histogram"])
                for key in ("p50", "p95", "p99", "max", "samples"):
                    report.add(f"抽稀[{MODE_DECIMATED}] {key} 可复算",
                               int(decimated_set.get(key, -1)) == replay[key],
                               f"证据 {decimated_set.get(key)} vs 重放 {replay[key]}")
            report.add(f"{MODE_DECIMATED}/samples = sampling.accepted",
                       int(decimated_set.get("samples", -1)) == accepted,
                       f"{decimated_set.get('samples')} vs {accepted}")
    elif accepted > 0:
        report.add("E3 抽稀有样本 ⇒ 抽稀那一份存在", isinstance(decimated_set, dict),
                   f"accepted={accepted}，"
                   + ("抽稀那一份在" if isinstance(decimated_set, dict)
                      else "却没有 quantiles.media_time_decimated"))
        if isinstance(decimated_set, dict):
            if decimated_set.get("histogram"):
                replay = replay_quantiles(decimated_set["histogram"])
                for key in ("p50", "p95", "p99", "max", "samples"):
                    report.add(f"抽稀[{MODE_DECIMATED}] {key} 可复算",
                               int(decimated_set.get(key, -1)) == replay[key],
                               f"证据 {decimated_set.get(key)} vs 重放 {replay[key]}")
            report.add(f"{MODE_DECIMATED}/samples = sampling.accepted",
                       int(decimated_set.get("samples", -1)) == accepted,
                       f"{decimated_set.get('samples')} vs {accepted}")
    else:
        report.add("E3 抽稀无样本 ⇒ 抽稀那一份缺席", decimated_set is None,
                   "accepted=0 却给出抽稀分位数 —— 那是拿空分布凑数")

    # ---- per_frame 必须一直在 ----
    # （`per_frame` 在上面取过了 —— 每个口径块只解析一次，
    #   两处各取一次是最容易长出"一处判 A 一处判 B"的写法。）
    report.add("保留 quantiles.per_frame", bool(per_frame),
               "逐帧那一份在" if per_frame
               else "G6 的 over_budget_frames 必须逐帧数，抽稀会漏掉恰好落空的那几帧")
    if per_frame.get("histogram"):
        replay_pf = replay_quantiles(per_frame["histogram"])
        for key in ("p50", "p99", "max", "samples"):
            report.add(f"逐帧[{MODE_PER_FRAME}] {key} 可复算",
                       int(per_frame.get(key, -1)) == replay_pf[key],
                       f"证据 {per_frame.get(key)} vs 重放 {replay_pf[key]}")

    # ---- trend 的可用性声明必须自洽 ----
    usable = bool(trend.get("usable", False))
    reason = str(trend.get("unusable_reason", ""))
    report.add("usable=false ⇒ reason 非空", usable or bool(reason),
               "不可用却不说理由，读者会把 0 读成「没有漂移」")
    report.add("usable=true ⇒ reason 为空", (not usable) or reason == "",
               f"usable=true 却带理由 {reason!r}")

    if usable:
        # ---- D3 ----
        report.add("D3 跨度 ≥ 30 s", span >= MIN_SPAN_NS,
                   f"span={span / 1e9:.3f}s")
        # 样本数在 `/7` 换了键名（P13-I）：`segment_samples` 同时被读成两种口径，
        # 一删为二。旧证据继续读旧键 —— 旧证据不重写，也不拿新键去判它们。
        samples_key = "window_sample_count" if schema in SCHEMA_WITH_WINDOW else "segment_samples"
        report.add("D3 样本 ≥ 30", int(trend.get(samples_key, 0)) >= MIN_SAMPLES,
                   f"{samples_key}={trend.get(samples_key)}")

        # ---- D4 ----
        #
        # 判据原文是"符号一致，且 |slope_ppm| 差 ≤ 20% 或 ≤ 5 ppm（取大者）"。
        # `5 ppm` 这个下限本身就说明：**低于 5 ppm 的数与 0 不可分辨**。
        # 于是"同号"在两边都贴着零的时候没有定义 —— 真机上就撞到了：
        # 8 个窗口中位数全部等于 125000（= 一个量化步长），窗口斜率恒为 0.000，
        # 而逐点斜率 1.39 ppm。拿"乘积 > 0"去判，会把一条纯噪声序列判成失败。
        #
        # 所以先用 5 ppm 的下限筛一遍符号，只有两边都超出可分辨范围时才要求同号。
        slope = float(trend.get("slope_ppm", 0.0))
        window_slope = float(trend.get("slope_from_windows_ppm", 0.0))
        floor_ppm = 5.0
        below_floor = abs(slope) <= floor_ppm or abs(window_slope) <= floor_ppm

        if below_floor:
            # 低于分辨率就不谈符号。但要**写清是"没测出来"而不是"测出为零"**：
            # 中位数分辨率 ≈ 一个桶宽 / 片长，本片长下它就是几 ppm 的量级。
            resolution_ppm = 250_000.0 * 1e9 / max(span, 1)
            report.add(
                "D4 两道斜率同号",
                True,
                f"不适用：|slope|={abs(slope):.3f} / |windows|={abs(window_slope):.3f} ppm "
                f"低于 {floor_ppm:.0f} ppm 的可分辨下限（本片长分辨率≈{resolution_ppm:.1f} ppm）",
            )
        else:
            same_sign = slope * window_slope > 0
            report.add("D4 两道斜率同号", same_sign,
                       f"slope={slope:.3f} windows={window_slope:.3f}")
        tolerance = max(0.20 * abs(slope), floor_ppm)
        close = abs(slope - window_slope) <= tolerance
        report.add("D4 两道斜率同量级", close,
                   f"差 {abs(slope - window_slope):.3f} ppm，容差 {tolerance:.3f}")

        # ---- D5 ----
        try:
            recomputed, recomputed_r2 = replay_window_slope(trend)
            delta = abs(recomputed - window_slope)
            # 两边都是 double 的同一套中心化矩，只差一个平移，误差应在 1e-6 量级。
            budget = max(1e-3, 1e-6 * max(1.0, abs(window_slope)))
            report.add("D5 窗口斜率逐位复算", delta <= budget,
                       f"证据 {window_slope:.9f} vs 重算 {recomputed:.9f}（差 {delta:.3e}）")
            report.add("D5 窗口 r2 与逐点 r2 同口径",
                       abs(recomputed_r2 - float(trend.get("r2", 0.0))) <= 1.0,
                       f"窗口 {recomputed_r2:.4f} vs 证据逐点 {trend.get('r2')}")
        except Failure as exc:
            report.add("D5 窗口斜率逐位复算", False, str(exc))

    # ---- I0–I6：窗口的身份（P13-I，仅 `/7`）----
    # 放在 trend 可用性之后：它要读 `window_samples` / `window_quantiles`，
    # 而那些字段与 `usable` 无关 —— 短片段同样可能有窗口，只是斜率不可信。
    if schema in SCHEMA_WITH_WINDOW:
        check_window_scope(report, sampling, trend, quantiles or {}, accepted)

    # ---- J0–J6：漂移的误差棒（P13-J，仅 `/8`）----
    #
    # 放在 I 之后：它读 `window_median_ns` / `window_sample_count`，
    # 而那些字段是 `/7` 起才有的。J 判据本身只对声明了 `/8` 的报告生效 ——
    # 旧证据不重写（重写等于伪造），也不能拿新判据去判旧格式（那只会产出假红）。
    if schema in SCHEMA_WITH_CONFIDENCE:
        check_confidence(report, trend)

    # ---- K0–K6：换一把承认自相关的尺子（P13-K，仅 `/9`）----
    #
    # 放在 J 之后：K 判据验的是"SE 从哪来"（带宽、ρ1、膨胀比、字段接线），
    # 前提是 J 已经把 SE / CI / detectable 三者的关系验过一遍。
    if schema in SCHEMA_WITH_HAC:
        check_hac(report, trend)

    # ---- L0–L5：看得见 ≠ 要紧（P13-L，仅 `/10`）----
    #
    # 放在 K 之后：它读 `slope_ppm` / `slope_ppm_std_error` / `slope_ci_*`，
    # 那些字段的可信度已经由 J、K 两组判过一遍；这里只负责"积回纳秒"这一步。
    #
    # 先判 `slope_ppm` 在不在：没有 drift 块（无音轨 / 未采样）时 `trend` 是空字典，
    # 那种证据谈不上累积量 —— 报"不适用"而不是让 L0 报一串缺失字段。
    if schema in SCHEMA_WITH_BUDGET:
        if "slope_ppm" in trend:
            check_budget(report, trend)
        else:
            report.add("L 累积量判据适用", True,
                       f"不适用：本轮是 {schema}，但 `trend` 块为空（无音轨 / 未采样）"
                       "—— 没有斜率就无从累积，这不是一个'通过'")

    # ---- M0–M4：段边界的权威是锚点重建（P13-M，仅 `/11`）----
    #
    # 放在 L 之后：它读 `segments` / `span_ns`，而这两个数的可信度
    # 由 H（跨度口径）与 I（窗口身份）已经验过一遍；这里只负责
    # "段切在哪儿、段内有没有洞"这件事。
    if schema in SCHEMA_WITH_SEGMENT_AUTHORITY:
        if "segments" in trend:
            check_segment_authority(report, sampling, trend)
        else:
            report.add("M 段边界判据适用", True,
                       f"不适用：本轮是 {schema}，但 `trend` 块没有 `segments`"
                       "（无音轨 / 未采样）—— 没有段就没有段边界")

    # ---- O0–O7：两个半程是不是同一条线（P13-O，仅 `/13`）----
    #
    # 放在 M 之后、N 之前：它读 `slope_first_half_ppm` / 两边 SE / 两段跨度，
    # 而斜率的可信度由 J、K 两组已经验过；这里只负责"两个拟合相容吗"这件事。
    if schema in SCHEMA_WITH_HALVES:
        if "slope_halves_relation" in trend:
            check_halves_relation(report, trend)
        else:
            report.add("O 半程一致性判据适用", True,
                       f"不适用：本轮是 {schema}，但 `trend` 块没有 `slope_halves_relation`"
                       "（无音轨 / 未采样）—— 没有斜率就没有半程")

    if schema in SCHEMA_WITH_QUANTILE_RESOLUTION:
        check_quantile_resolution(report, quantiles, trend)

    # ---- F：最差那一下在哪儿（P13-F，仅 `/4`）----
    #
    # 放在最后是因为它依赖 quantiles 两个口径块都已就位。
    # `/3` 不查 F：`worst` 是 `/4` 新增的字段，旧证据不重写（重写等于伪造）。
    if schema in SCHEMA_WITH_WORST:
        duration_ns = int((doc.get("source") or {}).get("duration_us", 0)) * 1000
        if isinstance(decimated_set, dict):
            check_worst(report, MODE_DECIMATED, decimated_set,
                        int(sampling.get("interval_ns", 0)), duration_ns,
                        int(trend.get("segments", 1)),
                        int(sampling.get("unobserved_cells", 0)))
        if isinstance(per_frame, dict) and per_frame:
            # 逐帧口径没有"绝对格点"这回事（帧间隔随帧率变），
            # 传 interval=0 显式跳过"media_ns 与 index 对齐"那条。
            check_worst(report, MODE_PER_FRAME, per_frame, 0, duration_ns)
    else:
        report.add("F 判据适用", True,
                   f"不适用：本轮是 {schema}，`worst` 是 sync/4 新增的字段（旧证据不重写）")

    return report, doc


# ---------------------------------------------------------------------------
# 跨文件：D1 / D2
# ---------------------------------------------------------------------------


def frame_rate_of(doc: dict[str, Any]) -> float:
    duration = int(doc.get("pace", {}).get("frame_duration_ns", 0))
    return (1e9 / duration) if duration > 0 else 0.0


def check_pair(reports: list[Report], docs: list[dict[str, Any]]) -> None:
    """D1 / D2：只比**同源同片长同分辨率、只差帧率**的两组。

    为什么不能"把所有 30fps 放一桶、所有 60fps 放一桶"：第一版就是那么写的，
    结果 640x360 的 35 s 长片被混进 30fps 桶、4K 被混进 60fps 桶，
    算出来的 p99 比值 1.00x 是个混合物的比值，谁都不代表。
    这张卡存在的理由就是"跨帧率可比"，**比对本身更不能混口径**。
    """
    decimated = [
        doc
        for doc in docs
        # 这里曾经硬编码 `schema == "kivo.p12c.sync/3"`
        # —— P13-F 把证据升到 `/4` 之后，D1/D2 的配对池**整批为空**，
        # 而那两卡的证据从来没被跨组判据看过一次。
        # 这是本项目第三次撞"两张表只更新一张"：前两次是
        # `card_checks` 漏分发（P13-E）、层 4 漏目录（P13-F）。
        # 改成问白名单，新增 schema 只要登记一次。
        if doc.get("schema") in SUPPORTED_SCHEMAS
        # `cross_frame_rate_set` 存在且指向抽稀那一份，才允许进对比 ——
        # 这里不自己判断 mode 字符串，而是问证据"你说哪一份可比"，
        # 再回去核那条路径是否真的存在。两处各判一次，迟早有一处写反。
        and doc.get("av_drift", {})
        .get("sampling", {})
        .get("cross_frame_rate_set") == f"quantiles.{MODE_DECIMATED}"
    ]

    # 分组键：帧率 + 片长 + 分辨率。同一个键里只差"跑了第几轮"。
    by_shape: dict[tuple[int, int, int], dict[int, list[dict[str, Any]]]] = {}
    for doc in decimated:
        video = doc.get("source", {}).get("video", {})
        shape = (
            int(doc.get("source", {}).get("duration_us", 0)),
            int(video.get("width", 0)),
            int(video.get("height", 0)),
        )
        by_shape.setdefault(shape, {}).setdefault(round(frame_rate_of(doc)), []).append(doc)

    for (duration, width, height), by_fps in sorted(by_shape.items()):
        if len(by_fps) < 2:
            continue
        slow = min(by_fps)
        fast = max(by_fps)
        slow_docs = by_fps[slow]
        fast_docs = by_fps[fast]
        label = f"{width}x{height} {duration / 1e6:.0f}s"

        def rate(items: list[dict[str, Any]]) -> float:
            values = []
            for doc in items:
                sampling = doc["av_drift"]["sampling"]
                accepted = int(sampling["accepted"])
                # 分母也走累计口径（P13-I）。这里原本只有 `trend.span_ns` ——
                # 报告层在 P13-H 修掉的正是这个除法，而**校验器自己没跟着改**：
                # 130 s 的证据在这里会算出 25.4 Hz，两组都错成一样，
                # 于是 D1 的比值照样通过。第七次"两张表只改一张"。
                span = int(sampling.get("media_span_ns", 0))
                if span <= 0:
                    span = int(doc["av_drift"]["trend"]["span_ns"])
                if span > 0:
                    values.append(accepted * 1e9 / span)
            return sum(values) / len(values) if values else 0.0

        # ---- D1 ----
        slow_rate = rate(slow_docs)
        fast_rate = rate(fast_docs)
        relative = abs(fast_rate - slow_rate) / max(slow_rate, fast_rate, 1e-9)
        d1 = Report(Path("D1"))
        d1.label = f"D1 抽稀节奏（{label}，{slow} vs {fast}fps）"
        d1.add(
            f"D1 {slow}fps 与 {fast}fps 的采样率相对差 ≤ 5%",
            relative <= 0.05,
            f"{slow_rate:.2f} Hz vs {fast_rate:.2f} Hz（差 {relative * 100:.2f}%）",
        )
        reports.append(d1)

        # ---- D2 ----
        #
        # 本卡唯一"允许失败"的判据：若抽稀后 60fps 的 p99 仍是 30fps 的 4 倍，
        # 说明 P13-C 那句"采样密度"解释错了，真实原因在别处。
        # 判据失败不是改判据，是在状态卡里承认解释错了。
        def p99_of(doc: dict[str, Any], mode: str) -> int:
            return int(doc["av_drift"]["quantiles"][mode]["p99"])

        slow_p99 = statistics.median([p99_of(d, MODE_DECIMATED) for d in slow_docs])
        fast_p99 = statistics.median([p99_of(d, MODE_DECIMATED) for d in fast_docs])
        raw_slow = statistics.median([p99_of(d, MODE_PER_FRAME) for d in slow_docs])
        raw_fast = statistics.median([p99_of(d, MODE_PER_FRAME) for d in fast_docs])
        ratio = fast_p99 / slow_p99 if slow_p99 > 0 else float("inf")
        # 判据名与输出一律带口径前缀：这张表存在的理由就是"不混口径"，
        # 那它自己更不该写出一个不带口径的 p99。
        d2 = Report(Path("D2"))
        d2.label = f"D2 {MODE_DECIMATED}/p99 比值（{label}，{slow} vs {fast}fps）"
        d2.add(
            f"D2 {MODE_DECIMATED}/p99：{fast}fps ≤ {slow}fps 的 2x",
            ratio <= 2.0,
            f"{fast_p99 / 1e6:.3f} ms / {slow_p99 / 1e6:.3f} ms = {ratio:.2f}x"
            f"（{MODE_PER_FRAME}/p99 {raw_fast / 1e6:.3f} / {raw_slow / 1e6:.3f} ms = "
            f"{raw_fast / raw_slow if raw_slow else 0:.2f}x，仅作对照、不参与判定）",
        )
        reports.append(d2)


# ---------------------------------------------------------------------------
# 入口
# ---------------------------------------------------------------------------


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2

    reports: list[Report] = []
    docs: list[dict[str, Any]] = []
    for raw in argv[1:]:
        path = Path(raw)
        if not path.exists():
            print(f"✗ 找不到文件：{path}")
            return 2
        report, doc = check_file(path)
        reports.append(report)
        if doc is not None:
            docs.append(doc)

    check_pair(reports, docs)

    failed = 0
    skipped = 0
    for report in reports:
        if report.not_applicable:
            print(f"\n– {report.label}：不适用（{report.not_applicable}）")
            skipped += 1
            continue
        status = "✓" if not report.failed else "✗"
        print(f"\n{status} {report.label}")
        for name, ok, detail in report.rows:
            mark = "  ✓" if ok else "  ✗"
            line = f"{mark} {name}"
            if detail:
                line += f" —— {detail}"
            print(line)
        failed += len(report.failed)

    print()
    if skipped:
        print(f"另有 {skipped} 份不适用（不是失败）。")
    if failed:
        print(f"结论：{failed} 项未通过。")
        return 1
    print("结论：全部通过。")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
