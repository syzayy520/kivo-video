#!/usr/bin/env python3
"""P12 证据总收口（F4）：一次跑完 235 份证据，回答"P12 整体还算不算数"。

    python tools/verify_p12_all.py                # 全量收口
    python tools/verify_p12_all.py --self-test    # 变异自检（证明它能打红）
    python tools/verify_p12_all.py --json out.json

四层，每层各自能被证伪：

  层 1 结构    全部证据可解析、schema 在白名单、有实测时间戳、无占位符
  层 2 卡片    12 组不变量（判据抄自各卡验收文档，不新造）
  层 3 红线    跨全组的四条"不说瞎话"：零静默失败 / 零无解释 / 零含糊 / 恒真字段告警
  层 4 复用    调 6 个既有校验器，**不重写判据**（两套判据必然分叉）

退出码：0 = 全部通过；1 = 有失败；2 = 自检未抓到全部变异（校验器本身不合格）。

设计见 `docs/design/backend-core/53-p12f4-design.md`。
"""
from __future__ import annotations

import argparse
import copy
import glob
import json
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EVIDENCE = ROOT / "docs" / "evidence"

# `av_drift` 带 `quantiles.<口径>` 结构的那几版。
#
# **一处定义，两处使用**：`sync_sample_count()` 从这里取"样本数在哪儿"，
# `check_sync()` 从这里取"是不是分块结构"。曾经这两处各写一遍元组，
# 结果 P13-F 加了 `/4` 只改了白名单、两处判据还认 `/3` ——
# 那是同一个病（两张表只更新一张）在同一个文件里的第三次发作。
SYNC_QUANTILE_BLOCK_SCHEMAS = (
    "kivo.p12c.sync/3",  # P13-E：分位数收进口径块
    "kivo.p12c.sync/4",  # P13-F：多一格 worst
    "kivo.p12c.sync/5",  # P13-G：抽稀块要 ≥2 样本且跨度 >0 才成立
    "kivo.p12c.sync/6",  # P13-H：多一格 media_span_ns（observed_hz 的累计分母）
    "kivo.p12c.sync/7",  # P13-I：trend 自带窗口分布；truncated 改成段口径
    "kivo.p12c.sync/8",  # P13-J：trend 给斜率配标准误与置信带；窗口中位数改精确
    "kivo.p12c.sync/9",  # P13-K：SE 换成 HAC（Newey–West），旧值改名 _iid；半程诊断
    "kivo.p12c.sync/10",  # P13-L：累积漂移 + 额度判读（看得见 ≠ 要紧）
    "kivo.p12c.sync/11",  # P13-M：段边界锚点重建；多出版段内最大空洞
    "kivo.p12c.sync/12",  # P13-N：p50 精度（可能范围 / 桶内样本数 / 桶宽）
    "kivo.p12c.sync/13",  # P13-O：半程各自 HAC 标准误 + 三态；replace 掉只比符号的布尔
    "kivo.p12c.sync/14",  # P13-P：最差帧的 due_step + 前后邻帧（手慢了 vs 表快了）
    "kivo.p12c.sync/15",  # P13-Q：两侧的跨帧累积（缓慢位移看得见）
    "kivo.p12c.sync/16",  # P13-R：四个跨度 + 最差帧的钟读数（due 为什么整体前移）
    "kivo.p12c.sync/17",  # P13-S：累积漂移门（只拦"持续超前"）
    "kivo.p12c.sync/18",  # P13-AB：探针的时钟台阶注入（clock_injection 三格）
    "kivo.p12c.sync/19",  # P13-AC：限速收敛（drift_max_correction / drift_corrected_total）
    "kivo.p12c.sync/20",  # P13-AD：速率注入 + 门开关（clock_injection.rate_ppm / drift_gate）
)

# ── schema 白名单 ──────────────────────────────────────────────────────────
# 硬编码是刻意的摩擦：新增卡片必须显式加进来，让"格式变更"被看见一次。
# 藏起来自动发现，只会让某天悄悄多出一种 schema 而没人注意。
KNOWN_SCHEMAS = {
    "kivo.p12a.capabilities/1",
    "kivo.p12b.decode/1",
    "kivo.p12c.aaudio-probe/1",
    "kivo.p12c.aaudio-probe/2",
    "kivo.p12c.audio-file-probe/1",
    "kivo.p12c.decode/2", "kivo.p12c.decode/3", "kivo.p12c.decode/4",
    "kivo.p12c.decode/5", "kivo.p12c.decode/6", "kivo.p12c.decode/7",
    "kivo.p12c.decode/8", "kivo.p12c.decode/9", "kivo.p12c.decode/10",
    "kivo.p12c.decode/11", "kivo.p12c.decode/12", "kivo.p12c.decode/13",
    # P14-A：新增 `metrics` —— CPU 占比 / 堆分配次数 / 输出缓冲归属。
    # 三项都带可用性与理由，`calls_steady = 0` 旁边必须有 `hook_installed`。
    "kivo.p12c.decode/14",
    # P14-B：`metrics.alloc` 增线程分桶（threads / thread_count / overflow_calls）。
    "kivo.p12c.decode/15",
    "kivo.p12c.sync/1",
    # P13-D：`av_drift` 的分位数改由"按媒体时间抽稀到 20 Hz"的分布给出，
    # 并新增 sampling / per_frame / trend 三块。`/1` 的证据不重写（重写等于伪造），
    # 两个版本同时接受；D1–D5 的判据只在 `/2` 上成立（见 tools/verify_p13d_drift.py）。
    "kivo.p12c.sync/2",
    # P13-E：`av_drift` 下不再有裸分位数，一律挂 `quantiles.<口径>`，
    # 并由 `sampling.cross_frame_rate_set` 指出唯一可比的那一份。
    # `/2` 保留白名单只是为了旧证据仍能被层 1 接受（它们无法 retrofit）——
    # 但注意：`/2` 曾**完全没被层 2 校验过**（分发里只写了 `/1`），见 `card_checks`。
    "kivo.p12c.sync/3",
    # P13-F：`quantiles.<口径>` 新增 `worst`（index / of / media_ns / lateness_ns）——
    # `max` 只说最差多大，说不出在片头还是片尾，而这两者对结论的影响完全相反。
    # `/3` 的证据不重写，两个版本同时接受；F 判据只在 `/4` 上成立。
    "kivo.p12c.sync/4",
    # P13-G：抽稀块要成立，至少 2 个样本且跨度为正 —— 1 个样本的块
    # 每个分位数都相同，长得像正常数据（`68-p13g-design.md` §二）。
    # `/4` 及以前的证据不重写，新门槛只对 `/5` 生效。
    "kivo.p12c.sync/5",
    # P13-H：`sampling` 新增 `media_span_ns`（整场累计跨度），`observed_hz`
    # 改以它为分母 —— 旧口径拿窗口跨度（封顶 102.35 s）除累计样本数，
    # 超过 102.4 s 之后商随片长线性增长（`70-p13h-design.md` §一）。
    # `/5` 及以前的证据不重写；H 判据只在 `/6` 上成立。
    "kivo.p12c.sync/6",
    # P13-I：`trend` 自带 `window_quantiles`（趋势描述的那批点自带分布），
    # 并把含糊的 `segment_samples` 拆成 `segment_accepted` + `window_sample_count`；
    # `truncated` 改判"本段接受数 > 容量"—— 上一版拿整场累计数比容量，
    # seek 之后会谎报环形覆盖（`72-p13i-design.md` §一）。
    # `/6` 及以前的证据不重写；I 判据只在 `/7` 上成立。
    "kivo.p12c.sync/7",
    # P13-J：`trend` 多出 `slope_ppm_std_error` / `slope_ci_low_ppm` /
    # `slope_ci_high_ppm` / `slope_detectable` / `window_median_exact` /
    # `window_median_fallback_windows`。两件事一起改的：
    # ① 八窗中位数上一版走 0.25 ms 桶中心，真机上全部等于 125000 ——
    #    `delta_median_ns` 与 `slope_from_windows_ppm` 恒 0，"没漂移"和
    #    "尺子上只有一个刻度"长得一样；
    # ② `slope_ppm` 在 `r2 = 5.5e-05` 之下照样印六位小数，没有误差棒
    #    （`74-p13j-design.md` §一）。`/7` 及以前的证据不重写；
    #    J 判据只在 `/8` 上成立。
    "kivo.p12c.sync/8",
    # P13-K：`slope_ppm_std_error` **换模型**（Newey–West / Bartlett），名字没变；
    # 旧口径原地改名 `slope_ppm_std_error_iid`，另加 `hac_lags` /
    # `hac_lag1_autocorrelation` / `hac_estimated` / 膨胀比，以及半程诊断
    # `slope_first_half_ppm` / `slope_second_half_ppm` / `slope_halves_agree`。
    # 起因是 `/8` 自己造的假阳性：r3（35 s / 700 点）报 `detectable = true`
    # 而那条线 `r2 = 0.007` —— 独立同分布假设把 SE 压小了
    # （`76-p13k-design.md` §一）。`/8` 及以前的证据不重写；K 判据只在 `/9` 上成立。
    "kivo.p12c.sync/9",
    # P13-L：`slope_detectable` 后面接上 `drift_accumulated_ns` /
    # `drift_accumulated_upper_ns` / `drift_budget_ns` / `drift_verdict`。
    # 起因是一句被读错了两张卡的话：`detectable = true` 被当成"漂移要紧" ——
    # 它只说"与 0 分得开"。反方向的漏报同样真实：CI 含 0 时远端积起来
    # 也可能越过人耳阈值，所以判定用的是**上界**（`78-p13l-design.md` §一）。
    # `/9` 及以前的证据不重写；L 判据只在 `/10` 上成立。
    "kivo.p12c.sync/10",
    # P13-M：`segments` 后面接上 `segment_largest_gap_cells` /
    # `segment_contiguous`，`segments` 的语义收窄成"一次锚点生命周期算一段"。
    # 起因是 P13-L 撞出的一份真机缺陷证据：在 30 s 处向前跳到 40 s，
    # 报告照样写 `segments = 1`、`span = 112.367 s` —— pts 前进推不出"没换时间轴"，
    # 向前 seek 必有一次漏切（`80-p13m-design.md` §一）。
    # `/10` 及以前的证据不重写；M 判据只在 `/11` 上成立。
    "kivo.p12c.sync/11",
    # P13-N：每个 `quantiles.<口径>` 的 `p50` 后面接上 `p50_range_low_ns` /
    # `p50_range_high_ns` / `p50_bucket_samples` / `bucket_width_ns`；
    # `trend` 里接上 `median_of_window_medians_ns`。
    # 起因是同一份证据里两个 p50 相差 66%（桶中心 125 000 vs 精确值 74 835），
    # 而 `/11` 的 JSON 里没有任何字段能阻止读者把它读成"两份测量打架"。
    # 它们不打架，是同一批点的两种精度 —— 缺的是让 p50 自己说出可能范围
    # （`82-p13n-design.md` §一）。`/11` 及以前的证据不重写；N 判据只在 `/12` 上成立。
    "kivo.p12c.sync/12",
    # `/13`（P13-O）：`slope_halves_*` 16 格 + 三态 `slope_halves_relation`。
    # `slope_halves_agree` 保留原语义（同号），降为历史字段 —— 删它会让旧证据
    # 无法对读，改语义会让同一字段名在两版证据里指两件事。
    "kivo.p12c.sync/13",
    # `/14`（P13-P）：`pace.due_step_frames` / `due_step_supplied` 两格，
    # 以及 `quantiles.per_frame.worst` 里的 `due_step_ns` + 前后各几帧的 `lateness`。
    # 目的只有一个：让 `Δlateness = present_step − due_step` 这条恒等式可复算 ——
    # 88.231 ms 倒在"手慢了"还是"表快了"上，旧字段分不出来（`86-p13p-design.md` §一）。
    "kivo.p12c.sync/14",
    # `/15`（P13-Q）：`pace` 五格整场账（first/last/Σpresent/Σdue/残差）+
    # `worst` 三格累积账。单帧 `due_step` 只看得到"一步"，看不到"每步都小、累积很大"——
    # `p13d-4k60-r1` 的 88 → 5 用六帧正是这种（`87-p13p-status.md` §三）。
    "kivo.p12c.sync/15",
    # `/16`（P13-R）：`pace` 五格（mono/elapsed/due/pts 四个跨度 + 正常分支帧数）+
    # `worst` 四格钟读数。`due_span == mono_span + pts_span − elapsed_span` 让
    # "due 为什么整体前移"只剩两个可能：钟比墙快，或帧比 pts 到得早。
    "kivo.p12c.sync/16",
    # `/17`（P13-S）：`pace.clock_health.drift_rebases` —— 累积漂移门的触发次数。
    # 与 `jump_rebases` 分开：那个是"一次跳 1 秒"，这个是"每帧只差一点、攒到 45 ms"。
    "kivo.p12c.sync/17",
    # `/18`（P13-AB）：顶层 `clock_injection` 三格。被注入过的证据与真实现场
    # 在字段上本来完全一样，这三格是唯一的区分 —— 缺了它就没法判断
    # 那个 `clock_ahead` 是设备跑偏还是探针按的按钮。
    "kivo.p12c.sync/18",
    # `/19`（P13-AC）：`clock_health` 多两格修正幅度。只数 `drift_rebases` 分不出
    # "一帧削平 45 ms"和"二十三帧各摊 2 ms"，而前者正是 P13-F 抱怨的尖峰形状。
    "kivo.p12c.sync/19",
    # `/20`（P13-AD）：`clock_injection.rate_ppm` 与顶层 `drift_gate`。
    # 前者区分"台阶"与"速率"两种注入（真实事件是后者）；后者让"门关"对照组
    # 不会与"门判断沉默"在证据上混为一谈。
    "kivo.p12c.sync/20",
    "kivo.p12d5.present-timing/1",
    "kivo.p12e.color-probe/1",
    "kivo.p12e.hdr-probe/1", "kivo.p12e.hdr-probe/2", "kivo.p12e.hdr-probe/3",
    "kivo.p12e3.layer-dataspace/1",
    "kivo.p12e4.frame-luma/1",
    "kivo.p12f.negative/1",
    "kivo.p13a.framerate/1",
}

# 时间戳字段：任取其一即可，用来区分"真机产物"与"手写示意片段"。
TIMESTAMP_KEYS = ("generated_at_unix_ms", "captured_at_host_ms", "tag")

# 早期音频探针（P12-C 的 C1/C2/C3）写证据时还没有"必须打时间戳"这条规矩。
# 它们有 device 段与自洽的数值，明显是真机产物，但确实缺时间戳。
# **不补**：补一个当前时间等于伪造。降为告警并在文档里记数量。
TIMESTAMP_EXEMPT_SCHEMAS = {"kivo.p12c.aaudio-probe/1", "kivo.p12c.aaudio-probe/2",
                            "kivo.p12c.audio-file-probe/1"}
PLACEHOLDERS = ("TODO", "PLACEHOLDER", "FIXME")

# 三态字段：出现 null / 空串就是"含糊"，而含糊比错更糟。
TRISTATE_KEYS = (("color", "verdict"), ("color", "hdr", "kind"),
                 ("source", "encryption", "verdict"))

# P13-C 修掉的 bug：`audio.opened` / `started` 从未被赋值，恒为 false。
# 下面这些是**修复之前**产生的证据 —— 判据字段（av_drift / 帧数）是有效的，
# 只有这两个标志坏。重跑它们要 8 轮真机，且不改变任何结论，所以豁免并明示。
# **新证据不在表里，必须真的 opened=true。**
AUDIO_FLAG_DEBT = {
    "p12c8-av-at-r1-sync.json",
    "p12c8-av-cpu-r1-sync.json",
    "p12c8-av-cpu-r2-sync.json",
    "p12c8-av-cpu-r3-sync.json",
    "p12d4-c8reg-r1-sync.json",
    "p12d4-c8reg-r2-sync.json",
    "p12d4-c8reg-r3-sync.json",
    "p12d4-d1reg-r1-sync.json",
}


@dataclass
class Finding:
    layer: int
    subject: str
    rule: str
    detail: str

    def render(self) -> str:
        return f"[层{self.layer} FAIL] {self.subject}\n           {self.rule}: {self.detail}"


@dataclass
class Report:
    findings: list[Finding] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    counts: dict[str, int] = field(default_factory=dict)

    def fail(self, layer: int, subject: str, rule: str, detail: str) -> None:
        self.findings.append(Finding(layer, subject, rule, detail))


# ── 读入 ──────────────────────────────────────────────────────────────────

def iter_evidence(root: Path) -> list[Path]:
    """所有证据文件，跳过 `superseded/` 与 `emulator/`。

    - `superseded/`：留档的反例，不是"现在对不对"的判据。
    - `emulator/`：模拟器产物。它只能验证 A 类（提取器契约），
      B/C 类（时序、丢帧、HDR 显示）在模拟器上测出来的数字是假的 ——
      混进来会让"全绿"变成假的。物理隔离，不靠命名约定。
    """
    out = []
    for path in sorted(root.rglob("*.json")):
        if "superseded" in path.parts or "emulator" in path.parts:
            continue
        out.append(path)
    return out


def rel_name(path: Path) -> str:
    """展示用文件名。自检时传的是虚拟路径，不在仓库里 —— 不能让它抛异常。"""
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def dig(node, path: tuple[str, ...]):
    for key in path:
        if not isinstance(node, dict) or key not in node:
            return None
        node = node[key]
    return node


class DuplicateKeyError(ValueError):
    """JSON 里出现重复键。Python 默认取最后一个，等于**静默丢数据**。"""


def strict_object(pairs):
    """`json.loads` 的 object_pairs_hook：发现重复键就抛。

    为什么必须查：P12-F3B 在 `decode_stats_renderer` 里另起了一个 `"source"` 键，
    于是每份 decode 证据都有两个 `source` —— 解析器取后者，
    `source.mime/width/height/duration_us` 全部消失。
    直到 P13-A 播 4K 时读 `source.width` 抛 `KeyError` 才暴露。
    一个 `grep -c '"source"'` 就能发现的事，却活了 6 张卡。
    """
    seen = {}
    for key, value in pairs:
        if key in seen:
            raise DuplicateKeyError(f"duplicate key: {key}")
        seen[key] = value
    return seen


def collect_strings(node, acc: list[str]) -> None:
    if isinstance(node, str):
        acc.append(node)
    elif isinstance(node, dict):
        for value in node.values():
            collect_strings(value, acc)
    elif isinstance(node, list):
        for value in node:
            collect_strings(value, acc)


# ── 层 1：结构 ────────────────────────────────────────────────────────────

def layer1_structure(path: Path, payload: dict, report: Report) -> None:
    name = rel_name(path)

    schema = payload.get("schema")
    if not schema:
        report.fail(1, name, "schema 缺失", "证据必须声明 schema")
        return
    if schema not in KNOWN_SCHEMAS:
        report.fail(1, name, "schema 不在白名单",
                    f"{schema!r} 未登记；新增卡片要显式加进 KNOWN_SCHEMAS")

    if not any(key in payload for key in TIMESTAMP_KEYS):
        if schema in TIMESTAMP_EXEMPT_SCHEMAS:
            report.counts["no_ts"] = report.counts.get("no_ts", 0) + 1
        else:
            report.fail(1, name, "缺实测时间戳",
                        f"需要 {'/'.join(TIMESTAMP_KEYS)} 之一，否则无法区分真机产物与手写片段")

    strings: list[str] = []
    collect_strings(payload, strings)
    for token in PLACEHOLDERS:
        if any(token in s for s in strings):
            report.fail(1, name, "含占位符", f"出现 {token}，草稿不能当证据")
            break


# ── 层 2：卡片不变量 ──────────────────────────────────────────────────────

def check_frames_closed(payload: dict, name: str, report: Report) -> None:
    counters = payload.get("counters") or {}
    received = counters.get("frames_received")
    rendered = counters.get("frames_rendered")
    if isinstance(received, int) and isinstance(rendered, int):
        # 允许 1 帧在途（交出与计数之间有一个观察窗口）。
        if rendered > received + 1:
            report.fail(2, name, "帧数不闭合",
                        f"rendered={rendered} > received={received}（上屏比收到还多）")
    dropped = counters.get("frames_dropped_late")
    if isinstance(dropped, int) and dropped < 0:
        report.fail(2, name, "丢帧数为负", f"frames_dropped_late={dropped}")


def check_hdr_consistency(payload: dict, name: str, report: Report) -> None:
    hdr = dig(payload, ("color", "hdr"))
    if not isinstance(hdr, dict):
        return
    kind = hdr.get("kind")
    applied = dig(hdr, ("dataspace", "applied"))
    if kind in ("hdr10", "hlg"):
        if applied is not True:
            report.fail(2, name, "HDR 源没设 dataspace",
                        f"kind={kind} 但 dataspace.applied={applied!r}")
    elif kind == "sdr":
        # SDR 不设 dataspace 是**正确行为**（E2 §五），设了才是问题。
        if applied is True:
            report.fail(2, name, "SDR 源误设 dataspace",
                        "SDR 轨道不应设置 HDR dataspace")
    elif kind is None:
        report.fail(2, name, "hdr.kind 缺失", "HDR 段必须给出三态之一")


def check_seek(payload: dict, name: str, report: Report) -> None:
    seek = payload.get("seek")
    if not isinstance(seek, dict):
        return
    requests = seek.get("requests")
    if not isinstance(requests, int) or requests < 1:
        return

    total = seek.get("total_us")
    budget = seek.get("budget_us") or 150_000
    if isinstance(total, int) and total > budget:
        report.fail(2, name, "seek 超预算",
                    f"total_us={total} > budget_us={budget}（G8）")

    av = seek.get("av")
    if isinstance(av, dict) and av.get("applicable") is True:
        gap = av.get("gap_us")
        limit = av.get("limit_us") or 40_000
        if isinstance(gap, int) and abs(gap) > limit:
            report.fail(2, name, "音画落点超限",
                        f"gap_us={gap} 绝对值 > limit_us={limit}")
        if av.get("pass") is not True:
            report.fail(2, name, "音画判据未通过",
                        f"av.pass={av.get('pass')!r} reason={av.get('reason')!r}")

    # F2 的两个坑：audio 重启让节拍器进 kFreeRun，接着整段追赶。
    pace = payload.get("pace") or {}
    for key, label in (("catch_up_frames", "追赶帧"), ("fallback_frames", "回退帧")):
        container = pace.get("clock_mix") if key == "fallback_frames" else pace
        value = (container or {}).get(key)
        if isinstance(value, int) and value > 0:
            report.fail(2, name, f"seek 后出现{label}",
                        f"{key}={value}；F2A 的判据要求 seek 轮为 0")


def check_encryption(payload: dict, name: str, report: Report) -> None:
    enc = dig(payload, ("source", "encryption"))
    if not isinstance(enc, dict):
        return
    verdict = enc.get("verdict")
    if verdict not in ("clear", "unknown", "encrypted"):
        report.fail(2, name, "加密判定含糊",
                    f"verdict={verdict!r} 必须是 clear/unknown/encrypted 之一")
        return
    if verdict == "encrypted":
        report.fail(2, name, "加密轨却出现在播放证据里",
                    "加密应在 open 阶段被拒（rc=12），不该产生 decode 证据")
    if verdict in ("unknown", "encrypted") and not enc.get("reason"):
        report.fail(2, name, "加密判定无理由", f"verdict={verdict} 但 reason 为空")


def check_tonemap(payload: dict, name: str, report: Report) -> None:
    tonemap = dig(payload, ("color", "hdr", "tonemap"))
    if not isinstance(tonemap, dict):
        return
    if tonemap.get("implemented") is True:
        report.fail(2, name, "声称已实现 tone mapping",
                    "E4 结论是未实现；证据若说 true，说明代码或判据被改过")
    if not tonemap.get("reason"):
        report.fail(2, name, "tonemap 无理由", "不实现就必须写清为什么")


# 顶层出现就算"裸分位数"的键。`per_frame` 也在里面 —— 那是 `/2` 的旧位置。
BARE_QUANTILE_KEYS = ("samples", "p50", "p95", "p99", "max", "histogram", "per_frame")


def sync_sample_count(drift: dict, schema: str) -> object:
    """这一轮采到多少分位样本 —— **版本不同，位置不同**。

    `/1` 与 `/2`：`av_drift.samples`（不分口径，所以 P13-E 把它挪了）。
    `/3` 起：`quantiles.<口径>.samples`，优先抽稀那一份（跨帧率可比的那份）。
    `/5` 起抽稀块可能整块缺席（样本不足），那时只有 `per_frame`——顺着往后找即可。
    """
    if schema in SYNC_QUANTILE_BLOCK_SCHEMAS:
        quantiles = drift.get("quantiles") or {}
        for mode in ("media_time_decimated", "per_frame"):
            block = quantiles.get(mode) or {}
            value = block.get("samples")
            if isinstance(value, int):
                return value
        return None
    return drift.get("samples")


def check_sync(payload: dict, name: str, report: Report,
               schema: str = "kivo.p12c.sync/1") -> None:
    drift = payload.get("av_drift") or {}
    # `/3` 与 `/4` 共用 `quantiles.<口径>` 结构；`/4` 只是多了一格 `worst`。
    is_v3 = schema in SYNC_QUANTILE_BLOCK_SCHEMAS

    # 跨字段一致性（P13-C 加的）：**单个字段看不出问题，两个字段放一起才看得出来**。
    #
    # 背景：`audio.opened` / `audio.started` 从来没被赋值过（`AvSyncStats` 有字段，
    # 但 `AudioPlayback` 从不写它），于是每份 sync 证据都写 `opened: false`，
    # 而同一份证据里 `clock_source: audio_timestamp`、`av_drift.samples: 180`。
    # 一个**恒为 false 的字段**比缺字段更危险 —— 它看起来是个合理的观测值。
    audio = payload.get("audio") or {}
    clock_source = payload.get("clock_source") or ""
    samples = sync_sample_count(drift, schema)
    has_audio_evidence = (clock_source == "audio_timestamp") or (
        isinstance(samples, int) and samples > 0)
    if has_audio_evidence:
        base = name.replace("\\", "/").rsplit("/", 1)[-1]
        # `opened` 与 `started` 都必须在 —— 缺一个就说明音频那一段的记录不可信。
        # P13-C 之前这两个字段从来没写对过：`opened` 从未赋值，`started` 被 stop() 清掉。
        broken = [k for k in ("opened", "started") if audio.get(k) is not True]
        if broken:
            if base in AUDIO_FLAG_DEBT:
                report.counts["audio_debt"] = report.counts.get("audio_debt", 0) + 1
            else:
                report.fail(2, name, "音频证据自相矛盾",
                            f"clock_source={clock_source!r} samples={samples!r} "
                            f"但 {', '.join(f'audio.{k}={audio.get(k)!r}' for k in broken)}")

    if drift.get("measurable") is False and not drift.get("unavailable_reason"):
        report.fail(2, name, "音画漂移不可测却无理由",
                    "measurable=false 必须带 unavailable_reason")

    if drift.get("measurable") is True:
        if is_v3:
            # `/3` 不再有 `samples` 这个键，判据改看"有没有按口径分块的分布"。
            # 只判 `samples` 是否存在，会让新证据每一份都红 —— 那不是发现，是没跟上。
            if not isinstance(drift.get("quantiles"), dict) or not drift.get("quantiles"):
                report.fail(2, name, "可测却没有分位数",
                            "quantiles 必须是按口径分块的非空对象")
        else:
            samples = drift.get("samples")
            if not isinstance(samples, int) or samples <= 0:
                report.fail(2, name, "可测却没有样本", f"samples={samples!r}")

    if is_v3:
        # E1 的粗筛：详细复算在 tools/verify_p13d_drift.py，这里只保证
        # "不带口径的分位数"根本进不了总收口。
        bare = [key for key in BARE_QUANTILE_KEYS if key in drift]
        if bare:
            report.fail(2, name, "裸分位数",
                        f"av_drift 顶层出现 {bare}：分位数必须挂在 quantiles.<口径> 下")


def check_audio_probe(payload: dict, name: str, report: Report) -> None:
    open_ok = dig(payload, ("open", "ok"))
    start_ok = dig(payload, ("start", "ok"))
    if open_ok is False:
        report.fail(2, name, "AAudio 打不开",
                    f"open.ok=false message={dig(payload, ('open', 'message'))!r}")
    if start_ok is False:
        report.fail(2, name, "AAudio 起不来",
                    f"start.ok=false message={dig(payload, ('start', 'message'))!r}")
    ratio = dig(payload, ("stats", "render_ratio"))
    if isinstance(ratio, float) and ratio < 0.95:
        report.fail(2, name, "渲染比不足", f"render_ratio={ratio}（应接近 1.0）")


def check_capabilities(payload: dict, name: str, report: Report) -> None:
    summary = payload.get("summary") or {}
    if not isinstance(summary.get("decoder_records"), int) or summary["decoder_records"] <= 0:
        report.fail(2, name, "能力矩阵为空", f"decoder_records={summary.get('decoder_records')!r}")
    if not payload.get("codecs"):
        report.fail(2, name, "codec 列表为空", "codecs 必须非空")


def check_layer_dataspace(payload: dict, name: str, report: Report) -> None:
    layer = payload.get("layer_dataspace") or {}
    ds_name = layer.get("name")
    if not ds_name:
        report.fail(2, name, "图层 dataspace 缺失", "没能从 SurfaceFlinger 取到 dataspace")
        return
    # SDR 轮必须是 BT709；HDR 轮必须是 BT2020 + PQ。名字比对，不比整数（E3 §四 第 2 条）。
    tag = str(payload.get("tag", ""))
    if "sdr" in tag.lower():
        if "709" not in ds_name:
            report.fail(2, name, "SDR 轮色域不对", f"tag={tag} dataspace={ds_name}")
    elif ds_name not in ("BT2020_ITU_PQ", "BT2020_PQ", "BT2020_ITU_HLG", "BT2020_HLG"):
        report.fail(2, name, "HDR 轮色域不对", f"tag={tag} dataspace={ds_name}")


def check_negative(payload: dict, name: str, report: Report) -> None:
    if not payload.get("reason"):
        report.fail(2, name, "负向证据无 reason", "负向用例必须说明是哪一类故障")
    if payload.get("stage") is None:
        report.fail(2, name, "负向证据无 stage", "要说清在哪个阶段失败的")


def check_framerate(payload: dict, name: str, report: Report) -> None:
    """P13-A 帧率能力：只查"这份证据有没有资格被引用"，不查数值合不合理。"""
    records = payload.get("record_count")
    if not isinstance(records, int) or records <= 0:
        report.fail(2, name, "帧率证据为空", f"record_count={records!r}")
    if payload.get("rejected_count"):
        # 有被拒的行就说明采集或解析出了问题 —— 不能当作"部分有效"。
        report.fail(2, name, "有记录解析失败", f"rejected_count={payload['rejected_count']}")
    verdict = payload.get("verdict") or {}
    if not verdict:
        report.fail(2, name, "没有 verdict 段", "证据必须直接回答能不能，而不是只给一堆行")
    for key, entry in verdict.items():
        answer = (entry or {}).get("answer")
        if answer not in ("yes", "no", "unknown"):
            report.fail(2, name, "verdict 取值非法", f"{key}.answer={answer!r}")


def card_checks(payload: dict, name: str, report: Report) -> None:
    schema = payload.get("schema", "")
    if schema == "kivo.p12a.capabilities/1":
        check_capabilities(payload, name, report)
    elif schema == "kivo.p12b.decode/1":
        counters = payload.get("counters") or {}
        if not dig(payload, ("outcome", "completed")):
            report.fail(2, name, "P12-B 没跑完", "outcome.completed=false")
        if not isinstance(counters.get("frames_rendered"), int) or counters["frames_rendered"] <= 0:
            report.fail(2, name, "P12-B 零渲染", "frames_rendered 必须 > 0")
    elif schema.startswith("kivo.p12c.decode/"):
        check_frames_closed(payload, name, report)
        check_hdr_consistency(payload, name, report)
        check_seek(payload, name, report)
        check_encryption(payload, name, report)
        check_tonemap(payload, name, report)
    elif schema.startswith("kivo.p12c.sync/"):
        # 曾经这里只写了 `/1`，于是 `/2` 的证据**完全跳过了层 2**：
        # 能在白名单里、能过解析，但"可测却没有样本""不可测却无理由"一条都没查。
        # 加 schema 却忘了加分发，等于给新证据开了一张免检通行证。
        check_sync(payload, name, report, schema)
    elif schema in ("kivo.p12c.aaudio-probe/1", "kivo.p12c.aaudio-probe/2"):
        check_audio_probe(payload, name, report)
    elif schema == "kivo.p12c.audio-file-probe/1":
        # 负向样本（无音轨/坏文件）本来就不该解到 EOS —— 判据对它们不适用。
        negative = bool(payload.get("failure")) or "no-audio-track" in name
        if not negative and dig(payload, ("decode", "reached_eos")) is not True:
            report.fail(2, name, "音频没解到 EOS", "reached_eos 必须为 true")
    elif schema.startswith("kivo.p12e.hdr-probe"):
        if payload.get("scale") and not payload.get("scope"):
            report.fail(2, name, "探针没写适用范围", "scope 必须非空（结论不自动适用于 Surface 路径）")
    elif schema == "kivo.p12e3.layer-dataspace/1":
        check_layer_dataspace(payload, name, report)
    elif schema == "kivo.p12f.negative/1":
        check_negative(payload, name, report)
    elif schema == "kivo.p13a.framerate/1":
        check_framerate(payload, name, report)


# ── 层 3：跨组红线 ────────────────────────────────────────────────────────

def layer3_red_lines(path: Path, payload: dict, report: Report) -> None:
    name = rel_name(path)

    # 1. 零静默失败
    #
    # **中途快照要排除**：D3 的 30min 长稳每 60s 拍一份 `-t####.json`，
    # 那些快照本来就是"还没跑完"，`completed=false` 是正确值。
    # 把它们当静默失败，会让一条正确的设计看起来像 bug。
    is_midrun_snapshot = bool(re.search(r"-t\d+\.json$", path.name))
    if is_midrun_snapshot:
        report.counts["midrun"] = report.counts.get("midrun", 0) + 1
    if not is_midrun_snapshot and dig(payload, ("outcome", "completed")) is False:
        if not dig(payload, ("outcome", "error")):
            report.fail(3, name, "静默失败", "completed=false 但 error 为空")
    for ok_path, msg_path in ((("open", "ok"), ("open", "message")),
                              (("start", "ok"), ("start", "message"))):
        if dig(payload, ok_path) is False and not dig(payload, msg_path):
            report.fail(3, name, "静默失败", f"{'.'.join(ok_path)}=false 但 message 为空")

    # 2. 零无解释
    for flag, reason in ((("av_drift", "measurable"), ("av_drift", "unavailable_reason")),
                         (("seek", "av", "applicable"), ("seek", "av", "reason")),
                         (("seek", "pass"), ("seek", "reason"))):
        if dig(payload, flag) is False and not dig(payload, reason):
            report.fail(3, name, "无解释的否定",
                        f"{'.'.join(flag)}=false 但 {'.'.join(reason)} 为空")

    # 3. 零含糊
    for key_path in TRISTATE_KEYS:
        holder = dig(payload, key_path[:-1])
        if isinstance(holder, dict):
            value = holder.get(key_path[-1])
            if value is None or value == "":
                report.fail(3, name, "三态字段含糊", f"{'.'.join(key_path)}={value!r}")

    # 4. 恒真字段（告警，不失败）—— 见 collect_seek_landings


def collect_seek_landings(payload: dict, acc: list[tuple[int, int]]) -> None:
    seek = payload.get("seek")
    if isinstance(seek, dict) and isinstance(seek.get("requests"), int) and seek["requests"] >= 1:
        target, landing = seek.get("target_us"), seek.get("landing_us")
        if isinstance(target, int) and isinstance(landing, int):
            acc.append((target, landing))


# ── 层 4：复用既有校验器 ──────────────────────────────────────────────────

def run_sub(name: str, argv: list[str], report: Report) -> None:
    proc = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        tail = (proc.stdout or proc.stderr).strip().splitlines()[-3:]
        report.fail(4, name, "子校验器失败",
                    f"exit={proc.returncode} :: " + " | ".join(tail))
    report.counts["sub"] = report.counts.get("sub", 0) + (0 if proc.returncode else 1)


def layer4_reuse(report: Report) -> None:
    py = sys.executable
    style = {"cwd": ROOT, "capture_output": True, "text": True}  # noqa: F841

    sync_files = sorted(glob.glob("docs/evidence/p12c/*sync*.json"))
    if sync_files:
        run_sub("verify_p12c_sync", [py, "tools/verify_p12c_sync.py", *sync_files], report)
    else:
        report.fail(4, "verify_p12c_sync", "找不到证据", "docs/evidence/p12c/*sync*.json 为空")

    d5_files = sorted(glob.glob("docs/evidence/p12d5/*.json"))
    if d5_files:
        run_sub("verify_p12d5_present", [py, "tools/verify_p12d5_present.py", *d5_files], report)
    else:
        report.fail(4, "verify_p12d5_present", "找不到证据", "docs/evidence/p12d5/*.json 为空")

    layer_files = sorted(glob.glob("docs/evidence/p12e3/*-layer.json")) + \
        sorted(glob.glob("docs/evidence/p12e4/*-layer.json"))
    if layer_files:
        run_sub("verify_p12e3_layer", [py, "tools/verify_p12e3_layer.py", *layer_files], report)
    else:
        report.fail(4, "verify_p12e3_layer", "找不到证据", "p12e3/p12e4 的 *-layer.json 为空")

    run_sub("verify_p12f3_errors",
            [py, "tools/verify_p12f3_errors.py", "docs/evidence/p12f3"], report)

    run_sub("verify_p12d_multi ratio",
            [py, "tools/verify_p12d_multi.py", "ratio",
             "--baseline", "docs/evidence/p12d/p12d2-baseline-r*.json",
             "--multi", "docs/evidence/p12d/p12d2-multi-r*.json"], report)
    paced_files = sorted(glob.glob("docs/evidence/p12d/p12d2-paced-*.json"))
    if paced_files:
        # `--files` 收的是**单个 glob 字符串**（脚本内部展开），不是多个路径。
        run_sub("verify_p12d_multi paced",
                [py, "tools/verify_p12d_multi.py", "paced",
                 "--files", "docs/evidence/p12d/p12d2-paced-*.json",
                 "--target-fps", "30"], report)
    else:
        report.fail(4, "verify_p12d_multi paced", "找不到证据",
                    "docs/evidence/p12d/p12d2-paced-*.json 为空")

    # P13-E 起接进总收口。之前 `verify_p13d_drift.py` 只能手动跑 ——
    # 一个"要靠人记得执行"的判据，等于没有判据：证据可以一直躺在盘上没人复算。
    #
    # 这里只查**结构**（E1–E3、F1–F3 + 分位数逐位复算），不带 D1/D2：
    # D1/D2 是"30fps 与 60fps 同源对照"的跨组结论，需要同一批证据一起喂；
    # 总收口按目录逐份遍历，凑不齐一对就会误判成失败。
    # 需要 D1/D2 时手动跑：`python tools/verify_p13d_drift.py docs/evidence/p13d/*-sync.json`。
    #
    # P13-F 的教训：这一段原来只扫 `docs/evidence/p13d/`，于是 p13f 的 5 份证据
    # **一份都没被 F 判据看过** —— 端到端植入一条 `worst = max + 1` 的坏证据，
    # 总收口照样报"四层通过 / 可交付"。这与 P13-E 抓到的"加 schema 忘了加分发"
    # 是同一个病：**白名单与分发是两张表，加了一张忘了另一张**。
    # 现在按目录模式扫，新增证据目录只要符合 `p1*f` 就自动纳入。
    p13_files = sorted(glob.glob("docs/evidence/p13*/*-sync.json"))
    if p13_files:
        run_sub("verify_p13d_drift", [py, "tools/verify_p13d_drift.py", *p13_files], report)
    else:
        report.fail(4, "verify_p13d_drift", "找不到证据", "docs/evidence/p13*/*-sync.json 为空")

    # P13-P：恒等式 / 邻帧长度 / 归类的复算。同样按目录扫 ——
    # 上一卡的教训是"白名单与分发是两张表"，这一处只留一条路径。
    if p13_files:
        run_sub("verify_p13p_episode",
                [py, "tools/verify_p13p_episode.py", *p13_files], report)

    # P13-AE：门的标定。判据是"触发次数 == ppm × 时长 / band 的算术预测"，
    # 外加关闸对照与"除首次外每次 ≤ band"。
    #
    # 不并进 `verify_p13d_drift`：那个问"抽稀后分位数自不自洽"，
    # 这个问"门动作了几次、幅度对不对"。两张判据合一个脚本，
    # 改其中一条就要重跑另一条的容差，早晚互相将就。
    ae_files = sorted(glob.glob("docs/evidence/p13ae/*-sync.json"))
    if ae_files:
        run_sub("verify_p13ae_gate",
                [py, "tools/verify_p13ae_gate.py", *ae_files], report)
    else:
        report.fail(4, "verify_p13ae_gate", "找不到证据", "docs/evidence/p13ae/*-sync.json 为空")

    # P14-A：三把尺子的自证（CPU 有效 / 钩子装上 / 缓冲归零 / 账目一致）。
    # 只喂 decode 证据 —— 那条判据看的是 `metrics` 块，sync 文件里没有。
    p14a_decode = sorted(f for f in glob.glob("docs/evidence/p14a/*.json")
                         if not f.endswith("-sync.json"))
    if p14a_decode:
        run_sub("verify_p14a_metrics",
                [py, "tools/verify_p14a_metrics.py", *p14a_decode], report)
    else:
        report.fail(4, "verify_p14a_metrics", "找不到证据",
                    "docs/evidence/p14a/*.json 为空")

    # P13-AF：跨臂比较（R2 极差 / R5 同臂复现）。单臂的次数判据复用
    # `verify_p13ae_gate`，这里只加"三臂之间不许差太多"那一条 ——
    # 一个判据只写一处，否则两套判据必然分叉。
    p13af_files = sorted(glob.glob("docs/evidence/p13af/*-sync.json"))
    if p13af_files:
        run_sub("summarize_p13af",
                [py, "tools/summarize_p13af.py", *p13af_files], report)
    else:
        report.fail(4, "summarize_p13af", "找不到证据",
                    "docs/evidence/p13af/*-sync.json 为空")

    run_sub("verify_p12d3_soak",
            [py, "tools/verify_p12d3_soak.py",
             "--mem", "docs/evidence/p12d/p12d3-soak-r1-mem.csv",
             "--snapshots", "docs/evidence/p12d/p12d3-soak-r1-t*.json",
             "--final", "docs/evidence/p12d/p12d3-soak-r1.json",
             "--sync", "docs/evidence/p12d/p12d3-soak-r1-sync.json"], report)


# ── 主流程 ────────────────────────────────────────────────────────────────

def run_all(root: Path) -> tuple[Report, dict]:
    report = Report()
    payloads: dict[Path, dict] = {}

    paths = iter_evidence(root)
    report.counts["files"] = len(paths)

    for path in paths:
        try:
            payload = json.loads(path.read_text(encoding="utf-8"),
                                  object_pairs_hook=strict_object)
        except DuplicateKeyError as exc:
            report.fail(1, rel_name(path), "JSON 重复键（静默丢数据）", str(exc))
            continue
        except json.JSONDecodeError as exc:
            report.fail(1, rel_name(path), "不可解析", str(exc))
            continue
        if not isinstance(payload, dict):
            report.fail(1, rel_name(path), "不是对象", type(payload).__name__)
            continue
        payloads[path] = payload

    landings: list[tuple[int, int]] = []
    for path, payload in payloads.items():
        layer1_structure(path, payload, report)
        card_checks(payload, rel_name(path), report)
        layer3_red_lines(path, payload, report)
        collect_seek_landings(payload, landings)

    # 恒真字段告警：全部 landing 精确等于 target ⇒ 这个字段无法证伪。
    # F1 就踩过（seek_to 恒返回请求值）。那时所有 evidence 都"完全命中"，
    # 直到 F2A 出现 4 992 000 才暴露出来。
    if landings and all(t == l for t, l in landings):
        report.warnings.append(
            f"全部 {len(landings)} 条 seek 证据的 landing_us 都精确等于 target_us："
            "这个字段在当前证据集里无法证伪（F1 曾因 seek_to 恒返回请求值栽过）")

    layer4_reuse(report)
    summary = {
        "files": report.counts.get("files", 0),
        "findings": len(report.findings),
        "warnings": len(report.warnings),
        "sub_verifiers_ok": report.counts.get("sub", 0),
    }
    return report, summary


# ── 自检：证明它能打红 ────────────────────────────────────────────────────

MUTATIONS = [
    ("删掉 schema", 1, lambda d: d.pop("schema", None)),
    ("schema 换成未知", 1, lambda d: d.__setitem__("schema", "kivo.unknown/9")),
    ("塞一个 TODO", 1, lambda d: d.__setitem__("note", "TODO")),
    ("静默失败", 3, lambda d: (d.__setitem__("outcome", {"completed": False, "error": ""}))),
    ("不可测却无理由", 3, lambda d: d.__setitem__("av_drift", {"measurable": False,
                                                               "unavailable_reason": ""})),
    ("HDR 没设 dataspace", 2, lambda d: d.__setitem__("color", {
        "hdr": {"kind": "hdr10", "dataspace": {"applied": False}}})),
    ("seek 超预算", 2, lambda d: d.__setitem__("seek", {
        "requests": 1, "total_us": 999_999, "budget_us": 150_000})),
    ("声称实现了 tonemap", 2, lambda d: d.__setitem__("color", {
        "hdr": {"kind": "sdr", "dataspace": {"applied": False},
                "tonemap": {"implemented": True, "reason": ""}}})),
]


def self_test(root: Path) -> int:
    """对每份 decode 证据轮流施加 8 个变异，确认对应的层能抓到。"""
    paths = iter_evidence(root)
    base = None
    for path in paths:
        payload = json.loads(path.read_text(encoding="utf-8"))
        if str(payload.get("schema", "")).startswith("kivo.p12c.decode/"):
            base = payload
            break
    if base is None:
        print("自检失败：找不到任何 decode 证据作为基底")
        return 2

    print("=== 校验器自检（8 个变异必须全部被抓到）===")
    missed = 0
    for label, layer, mutate in MUTATIONS:
        payload = copy.deepcopy(base)
        mutate(payload)
        report = Report()
        layer1_structure(Path("selftest.json"), payload, report)
        card_checks(payload, "selftest.json", report)
        layer3_red_lines(Path("selftest.json"), payload, report)
        caught = any(f.layer == layer for f in report.findings)
        print(f"  {'ok  ' if caught else 'MISS'} {label}（期望层{layer}）"
              f" -> 实际层 {sorted({f.layer for f in report.findings})}")
        if not caught:
            missed += 1

    # 重复键只能在 **文本层** 构造：dict 里同名赋值只是覆盖。
    # 这个变异必须单独做，否则会得到一个"假通过"的自检。
    text_caught = False
    try:
        json.loads('{"source": {"mime": "a"}, "source": {"encryption": {}}}',
                   object_pairs_hook=strict_object)
    except DuplicateKeyError:
        text_caught = True
    print(f"  {'ok  ' if text_caught else 'MISS'} 重复 source 键（文本层，期望层1）")
    if not text_caught:
        missed += 1

    total = len(MUTATIONS) + 1
    print(f"\n{total - missed}/{total} 个变异被抓到")
    return 0 if missed == 0 else 2


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="P12 证据总收口")
    parser.add_argument("--root", default=str(EVIDENCE))
    parser.add_argument("--json", default=None, help="把总表写成 JSON")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)

    root = Path(args.root)
    if not root.is_absolute():
        root = ROOT / root

    if args.self_test:
        return self_test(root)

    report, summary = run_all(root)

    print("=== P12 证据总收口 ===")
    print(f"证据文件      {summary['files']} 份（不含 superseded/）")
    for layer, title in ((1, "结构层"), (2, "卡片层"), (3, "红线层"), (4, "复用层")):
        hits = [f for f in report.findings if f.layer == layer]
        print(f"层{layer} {title:<6} {'通过' if not hits else f'{len(hits)} 条失败'}")

    # 豁免要显式列出：**任何"放过了什么"都必须能被看见**，
    # 否则"全绿"就是靠隐藏换来的，那比红更危险。
    exempt = []
    if report.counts.get("no_ts"):
        exempt.append(f"{report.counts['no_ts']} 份早期音频探针证据无时间戳"
                      "（P12-C 早期格式，补时间等于伪造，故豁免）")
    if report.counts.get("audio_debt"):
        exempt.append(f"{report.counts['audio_debt']} 份 P13-C 之前的 sync 证据 "
                      "audio.opened 恒 false（该字段从未被赋值的 bug，已在 P13-C 修掉）")
    if report.counts.get("midrun"):
        exempt.append(f"{report.counts['midrun']} 份长稳中途快照的 completed=false 不计为静默失败"
                      "（它们本来就没跑完）")
    if exempt:
        print("\n--- 豁免明细（不是失败，但要知道范围）---")
        for line in exempt:
            print(f"  ~ {line}")

    if report.warnings:
        print("\n--- 告警（不影响结论，但要看一眼）---")
        for line in report.warnings:
            print(f"  ! {line}")

    if report.findings:
        print("\n--- 失败明细 ---")
        for finding in report.findings:
            print(finding.render())
        print(f"\n结论：P12 **不可交付**（{len(report.findings)} 条失败）")
    else:
        exempt_total = (report.counts.get("no_ts", 0) + report.counts.get("midrun", 0) +
                report.counts.get("audio_debt", 0))
        suffix = f"，含 {exempt_total} 项已声明的豁免" if exempt_total else ""
        print(f"\n结论：P12 可交付（{summary['sub_verifiers_ok']} 个子校验器全部 exit 0{suffix}）")

    if args.json:
        target = Path(args.json)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(json.dumps({
            "summary": summary,
            "warnings": report.warnings,
            "findings": [f.__dict__ for f in report.findings],
            "exemptions": {k: report.counts.get(k, 0) for k in ("no_ts", "midrun")},
        }, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"机读总表已写入 {args.json}")

    return 0 if not report.findings else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
