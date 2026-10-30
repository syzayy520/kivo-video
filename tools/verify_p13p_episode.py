#!/usr/bin/env python3
"""复算「迟到尖峰是手慢了还是表快了」（`kivo.p12c.sync/14` 起）。

这张卡不是再分析一次 88 ms，而是让下面这条恒等式对每一份证据都成立、可复算：

    lateness[k] − lateness[k−1] = present_step[k] − due_step[k]
    present_step[k] = present_jitter 的第 k 个样本
    due_step[k]     = due[k] − (due[k−1] + frame_duration)      ← P13-P 新增的字段

于是「手慢了」（present 侧）与「表快了」（due 侧）不必靠猜：`present_jitter` 的
直方图给 `present_step` 封了顶，`due_step` 一出版，两侧各占多少就只剩一道减法。

本脚本守五条：
  P1 接线：`due_step_supplied == presented`。小于时 `due_step_frames == 0`
          **不可读作"没位移"** —— 那是"没接线"。
  P2 邻帧长度：`prev = min(2, index)`、`next = min(8, of − 1 − index)`。
          多一个少一个都说明记录器与直方图的样本数不同源。
  P3 邻帧上界：prev / next 的每个值都不超过 `lateness_ns`（并列允许相等）。
          超过就意味着"最差"不是最差，极值比较写错了口径。
  P4 恒等式：`present_step = due_step + (lateness − prev[-1])` 必须落在
          `present_jitter` 的 [min, max] 之内；并按 0.75 规则归到
          `present_side` / `due_side` / `mixed`。
  P5 计数自洽：`|due_step_ns| > frame_duration` ⇒ `due_step_frames ≥ 1`；
          `due_step_frames ≤ presented`。

用法：
    python tools/verify_p13p_episode.py docs/evidence/p13p/*-sync.json
    python tools/verify_p13p_episode.py --self-test
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

SCHEMA_WITH_EPISODE = {f"kivo.p12c.sync/{n}" for n in range(14, 40)}
# `/15` 起多三格最差帧的累积账与五格整场账（P13-Q）。
SCHEMA_WITH_CUMULATIVE = {f"kivo.p12c.sync/{n}" for n in range(15, 40)}
# `/16` 起多四个跨度与最差帧的钟读数（P13-R）。
SCHEMA_WITH_SPANS = {f"kivo.p12c.sync/{n}" for n in range(16, 40)}
# `/18` 起多 `clock_injection` 三格（P13-AB）：探针注入的台阶。
SCHEMA_WITH_INJECTION = {f"kivo.p12c.sync/{n}" for n in range(18, 40)}
# `/19` 起多两格修正幅度（P13-AC）：迟滞带。
SCHEMA_WITH_SLEW = {f"kivo.p12c.sync/{n}" for n in range(19, 40)}
# `/20` 起多 `clock_injection.rate_ppm` 与 `drift_gate` 三格（P13-AD）。
SCHEMA_WITH_GATE = {f"kivo.p12c.sync/{n}" for n in range(20, 40)}
# 与 `PacePolicy::drift_limit_ns` / `drift_band_ns` **同步**：改策略必须升 schema，
# 否则这条判据会拿旧目标去卡新实现（45 − 5 = 40；`109-p13ac-design.md` §二）。
DRIFT_LIMIT_NS = 45_000_000
DRIFT_BAND_NS = 5_000_000
MAX_BEFORE = 2
MAX_AFTER = 8
# 归档阈值，不是物理常数：只用来把"主因"挑出来，`mixed` 是合法结果。
SIDE_SHARE = 0.75


class Failure(Exception):
    """一条可读的失败。"""


def _reports(path: Path) -> list[dict[str, Any]]:
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001 - 任何解析失败都要带文件名
        raise Failure(f"{path.name}: 无法解析 JSON: {exc}") from exc
    if not isinstance(doc, dict):
        raise Failure(f"{path.name}: 顶层不是对象")
    return [doc]


def classify(delta_ns: int, due_step_ns: int, frame_duration_ns: int) -> str:
    """把一次跳变归到 `present_side` / `due_side` / `mixed` / `flat`。

    `flat`：跳变本身不到一帧，属于噪声，不给结论。
    """
    if frame_duration_ns <= 0 or abs(delta_ns) <= frame_duration_ns:
        return "flat"
    present_part = delta_ns - due_step_ns
    due_part = due_step_ns
    magnitude = abs(delta_ns)
    if abs(due_part) >= SIDE_SHARE * magnitude:
        return "due_side"
    if abs(present_part) >= SIDE_SHARE * magnitude:
        return "present_side"
    return "mixed"


def check_file(path: Path, doc: dict[str, Any]) -> dict[str, Any]:
    schema = doc.get("schema")
    if schema not in SCHEMA_WITH_EPISODE:
        # 旧证据没有这些字段：判"不适用"，不判失败 —— 但必须说出来。
        return {"status": "skip", "why": f"schema {schema} 早于 /14"}

    pace = doc.get("pace") or {}
    presented = pace.get("presented")
    supplied = pace.get("due_step_supplied")
    frames_over = pace.get("due_step_frames")
    duration = pace.get("frame_duration_ns") or 0

    if presented is None or supplied is None or frames_over is None:
        raise Failure(f"{path.name}: /14 缺少 pace.due_step_* 三格")

    # ---- P1 接线 ----
    if supplied != presented:
        raise Failure(
            f"{path.name}: P1 due_step_supplied={supplied} != presented={presented}"
            "（这一路参数没接上，due_step_frames 的 0 不可读作没位移）"
        )

    # ---- P5 计数自洽（先做，后面 P4 要用到 duration）----
    if frames_over > presented:
        raise Failure(f"{path.name}: P5 due_step_frames={frames_over} > presented={presented}")

    quantiles = (doc.get("av_drift") or {}).get("quantiles") or {}
    per_frame = quantiles.get("per_frame") or {}
    worst = per_frame.get("worst")
    if not worst:
        raise Failure(f"{path.name}: P2 per_frame 没有 worst 块")

    index = worst.get("index")
    of = worst.get("of")
    lateness = worst.get("lateness_ns")
    due_step = worst.get("due_step_ns")
    prev = worst.get("prev_lateness_ns")
    nxt = worst.get("next_lateness_ns")
    for name, value in (("index", index), ("of", of), ("lateness_ns", lateness),
                        ("due_step_ns", due_step)):
        if value is None:
            raise Failure(f"{path.name}: worst 缺少 {name}")
    if prev is None or nxt is None:
        raise Failure(f"{path.name}: worst 缺少 prev_lateness_ns / next_lateness_ns")
    if of != per_frame.get("samples"):
        raise Failure(f"{path.name}: worst.of={of} != per_frame.samples={per_frame.get('samples')}")

    # ---- P2 邻帧长度 ----
    want_prev = min(MAX_BEFORE, index)
    want_next = min(MAX_AFTER, of - 1 - index)
    if len(prev) != want_prev:
        raise Failure(f"{path.name}: P2 prev 长度 {len(prev)} != {want_prev}（index={index}）")
    if len(nxt) != want_next:
        raise Failure(
            f"{path.name}: P2 next 长度 {len(nxt)} != {want_next}（index={index}, of={of}）"
        )

    # ---- P3 邻帧上界 ----
    for label, values in (("prev", prev), ("next", nxt)):
        for i, value in enumerate(values):
            if value > lateness:
                raise Failure(
                    f"{path.name}: P3 {label}[{i}]={value} > lateness_ns={lateness}"
                    "（极值比较口径不一致，或新旧最差的尾巴被拼在一起）"
                )

    # ---- P5 后半：due_step 超一帧就必须被计到 ----
    if duration > 0 and abs(due_step) > duration and frames_over < 1:
        raise Failure(
            f"{path.name}: P5 worst.due_step_ns={due_step} 超过帧长 {duration}，"
            f"而 due_step_frames={frames_over}"
        )

    # ---- P4 恒等式 ----
    jitter = doc.get("present_jitter") or {}
    jitter_max = jitter.get("max")
    jitter_min = (jitter.get("histogram") or {}).get("min_ns")
    if jitter_max is None or jitter_min is None:
        raise Failure(f"{path.name}: P4 缺 present_jitter.max / histogram.min_ns")

    side = "flat"
    present_step = None
    dropped_frames = doc.get("frames_dropped")
    if prev and dropped_frames not in (0, None):
        # 丢帧轮：`due_step` 相对的是上一**决策**帧，`lateness` 相对的是上一**呈现**帧，
        # 两边分母不同，P4 的端点在数学上就不该相等（详见 `88-p13q-design.md` §2.3）。
        present_step = None
    elif prev:
        delta = lateness - prev[-1]
        # Δ = present_step − due_step  ⇒  present_step = Δ + due_step
        present_step = delta + due_step
        # 容差：直方图是桶化的（0.25 ms），端点本身有半个桶的不确定。
        tolerance = pace.get("bucket_width_ns") or 250_000
        if present_step > jitter_max + tolerance:
            raise Failure(
                f"{path.name}: P4 present_step={present_step} > jitter.max={jitter_max}"
                "（恒等式不成立：due_step 与 lateness 不同源）"
            )
        if present_step < jitter_min - tolerance:
            raise Failure(
                f"{path.name}: P4 present_step={present_step} < jitter.min={jitter_min}"
            )
        side = classify(delta, due_step, duration)

    # ---- Q1–Q3：跨帧账（P13-Q，仅 /15 起）----
    closure_note = ""
    if schema in SCHEMA_WITH_CUMULATIVE:
        first = pace.get("first_lateness_ns")
        last = pace.get("last_lateness_ns")
        sum_present = pace.get("sum_present_step_ns")
        sum_due = pace.get("sum_due_step_ns")
        gap = pace.get("closure_gap_ns")
        for name, value in (("first_lateness_ns", first), ("last_lateness_ns", last),
                            ("sum_present_step_ns", sum_present),
                            ("sum_due_step_ns", sum_due), ("closure_gap_ns", gap)):
            if value is None:
                raise Failure(f"{path.name}: Q1 /15 缺少 pace.{name}")

        # Q3 锚点不变量：首帧建锚时 `due` 取的是**到达**那一刻，而 `lateness` 记的是
        # **交出**那一刻，中间隔着"醒来到交出"的几微秒。所以它不是 0，而是 µs 级；
        # 超过 1 ms 说明锚点不是在首帧建的（或 due 被改写），整条跨帧账的前提就没了。
        if abs(first) > 1_000_000:
            raise Failure(
                f"{path.name}: Q3 first_lateness_ns={first} 超过 1 ms（锚点不变量被破坏）"
            )

        # Q1 整场账：`(last − first) == Σpresent − Σdue`。
        recomputed = (last - first) - (sum_present - sum_due)
        if recomputed != gap:
            raise Failure(
                f"{path.name}: Q1 closure_gap={gap} 与复算 {recomputed} 不一致"
            )
        dropped = doc.get("frames_dropped")
        if dropped == 0 and abs(gap) > 1_000:
            raise Failure(
                f"{path.name}: Q1 零丢帧时残差应为 0，实得 {gap} ns"
                "（中间有帧没被记账：present_step 漏传或锚点重建）"
            )

        # Q2 最差帧的累积账：`lateness − lateness_at_first == cum_present − cum_due`
        cum_present = worst.get("cumulative_present_step_ns")
        cum_due = worst.get("cumulative_due_step_ns")
        lateness_first = worst.get("lateness_at_first_ns")
        if cum_present is None or cum_due is None or lateness_first is None:
            raise Failure(f"{path.name}: Q2 worst 缺累积三格")
        dropped_q2 = doc.get("frames_dropped")
        if dropped_q2 not in (0, None):
            # 丢帧轮账不平是**已知**的（§2.3），这里只要求它如实出版残差。
            if pace.get("closure_gap_ns", 0) == 0:
                raise Failure(
                    f"{path.name}: Q2 丢了 {dropped_q2} 帧却报残差 0 —— 残差被抹平了"
                )
        elif (lateness - lateness_first) != (cum_present - cum_due):
            raise Failure(
                f"{path.name}: Q2 最差帧累积账不闭合："
                f"{lateness - lateness_first} != {cum_present - cum_due}"
            )
        closure_note = f"gap={gap}"

    span_note = ""
    if schema in SCHEMA_WITH_SPANS:
        mono_span = pace.get("arrive_mono_span_ns")
        elapsed_span = pace.get("elapsed_span_ns")
        due_span = pace.get("due_span_ns")
        pts_span = pace.get("pts_span_ns")
        normal = pace.get("clock_supplied_frames")
        for name, value in (("arrive_mono_span_ns", mono_span), ("elapsed_span_ns", elapsed_span),
                            ("due_span_ns", due_span), ("pts_span_ns", pts_span),
                            ("clock_supplied_frames", normal)):
            if value is None:
                raise Failure(f"{path.name}: R1 /16 缺少 pace.{name}")

        # R1a 接线自证：四格全 0 有两种读法（"没动" / "没接线"），这里把后者判死。
        if normal != presented:
            raise Failure(
                f"{path.name}: R1 clock_supplied_frames={normal} != presented={presented}"
                "（钟读数没接线，四个跨度全是 0，不可读作'没动'）"
            )

        # R1b：正常分支上四个跨度严格闭合。走 free_run / pts 不连续分支的帧不满足这条，
        # 所以只在"这一轮一帧都没降级"时判。
        free_run = (pace.get("clock_health") or {}).get("free_run_frames", 0)
        if free_run == 0:
            recomputed = mono_span + pts_span - elapsed_span
            if abs(due_span - recomputed) > 1_000:
                raise Failure(
                    f"{path.name}: R1 due_span={due_span} != mono+pts−elapsed={recomputed}"
                )
            # R2：最差帧上的同一条件（单帧版），保证"最差帧的 due_step 来源"可复算。
            due_at = worst.get("due_at_ns")
            clock_mono = worst.get("clock_mono_ns")
            clock_elapsed = worst.get("clock_elapsed_ns")
            anchor_pts = worst.get("anchor_pts_ns")
            if None in (due_at, clock_mono, clock_elapsed, anchor_pts):
                raise Failure(f"{path.name}: R2 worst 缺钟读数四格")
            expect_due = clock_mono + worst.get("media_ns", 0) - anchor_pts - clock_elapsed
            if abs(due_at - expect_due) > 1_000:
                raise Failure(
                    f"{path.name}: R2 worst.due_at={due_at} != mono+pts−anchor−elapsed="
                    f"{expect_due}"
                )

        # R3：谁在动 —— 钟比墙快多少、帧比 pts 到得早多少。两个都是 due 前移的来源。
        clock_ahead = elapsed_span - mono_span
        arrival_excess = mono_span - pts_span
        span_note = f"clock_ahead={clock_ahead / 1e6:+.2f}ms arrive_excess={arrival_excess / 1e6:+.2f}ms"

    side_at_worst = ""
    cum_side = ""
    if schema in SCHEMA_WITH_CUMULATIVE:
        cum_present = worst.get("cumulative_present_step_ns", 0)
        cum_due = worst.get("cumulative_due_step_ns", 0)
        side_at_worst = f"cumP={cum_present / 1e6:+.2f} cumD={cum_due / 1e6:+.2f}"
        # 累积口径的主因：与单帧口径同一条 0.75 规则，只是把"这一步"换成"整段"。
        # 两个口径常常给出不同的答案 —— 那正是 P13-Q 存在的理由，不是矛盾。
        total = cum_present - cum_due
        if doc.get("frames_dropped") not in (0, None):
            # 丢帧轮两个分母不同（§2.3），累积账不成立 —— 不给归类，免得读成一个结论。
            cum_side = "invalid(丢帧)"
        elif abs(total) <= (pace.get("frame_duration_ns") or 0):
            cum_side = "flat"
        elif abs(cum_due) >= SIDE_SHARE * abs(total):
            cum_side = "due_side"
        elif abs(cum_present) >= SIDE_SHARE * abs(total):
            cum_side = "present_side"
        else:
            cum_side = "mixed"
        side_at_worst += f" cum={cum_side}"

    # 门的配置优先取证据里的（P13-AD 起出版），老 schema 退回常量。
    gate = doc.get("drift_gate") or {}
    limit_ns = gate.get("limit_ns", DRIFT_LIMIT_NS)
    band_ns = gate.get("band_ns", DRIFT_BAND_NS)
    gate_enabled = gate.get("enabled", True)

    slew_note = ""
    if schema in SCHEMA_WITH_SLEW:
        health = pace.get("clock_health") or {}
        max_correction = health.get("drift_max_correction_ns")
        total_correction = health.get("drift_corrected_total_ns")
        rebases = health.get("drift_rebases", 0)
        for name, value in (("drift_max_correction_ns", max_correction),
                            ("drift_corrected_total_ns", total_correction)):
            if value is None:
                # A1：没有幅度就分不出"一帧削平"和"二十三帧摊平"。
                raise Failure(f"{path.name}: A1 /19 缺少 clock_health.{name}")

        if rebases == 0:
            # A3：没触发就必须两格全 0。非 0 是幽灵修正。
            if max_correction != 0 or total_correction != 0:
                raise Failure(
                    f"{path.name}: A3 drift_rebases=0 却有修正幅度"
                    f"（max={max_correction} total={total_correction}）"
                )
        else:
            # A4 账：累计量不小于单帧最大，也不超过"次数 × 上限"。
            if total_correction < max_correction:
                raise Failure(
                    f"{path.name}: A4 累计修正 {total_correction} < 单帧最大 {max_correction}"
                )
            if total_correction > rebases * max_correction:
                raise Failure(
                    f"{path.name}: A4 累计修正 {total_correction} > "
                    f"{rebases} × 单次最大 {max_correction}"
                )

        # A5 端到端恒等式：注入了多少，就该修掉「注入量 − (门限 − 带)」。
        # 这是唯一一条把**输入**（注入台阶）与**输出**（修正总量）连起来的判据 ——
        # 少了它，三格幅度只能自证，证明不了修的是该修的那一段。
        block = doc.get("clock_injection") or {}
        injected = block.get("offset_ns", 0)
        # A5 只对"纯台阶"成立：速率注入没有"一次还完的总量"可言（它每帧都在变），
        # 所以 `rate_ppm != 0` 的轮次跳过这一条，改由 AD3 判。
        if (injected > 0 and block.get("armed_at_position_ns", -1) >= 0
                and block.get("rate_ppm", 0) == 0):
            expect = injected - (limit_ns - band_ns)
            frame_ns = pace.get("frame_duration_ns") or 0
            # 容差一帧：实测误差是 0.00–0.03 ms（`110-p13ac-status.md` §一），
            # 一帧已是一万倍的余量。放两帧就会漏掉"少修了一半"这种错。
            if abs(total_correction - expect) > max(frame_ns, 1):
                raise Failure(
                    f"{path.name}: A5 修正总量 {total_correction} 与预期 {expect}"
                    f"（注入 {injected} − 目标残差 {limit_ns - band_ns}）"
                    f" 相差超过一帧"
                )
        if max_correction:
            slew_note = f"slew_max={max_correction / 1e6:.2f}ms total={total_correction / 1e6:.2f}ms"

    gate_note = ""
    if schema in SCHEMA_WITH_GATE:
        # G1：门配置必须出版。缺了它，一份"关着门"的对照组和"门判断沉默"的
        # 正常轮在字段上完全一样 —— 这正是 P13-AD 要堵的洞。
        if not isinstance(doc.get("drift_gate"), dict):
            raise Failure(f"{path.name}: G1 /20 缺 drift_gate")
        block = doc.get("clock_injection") or {}
        rate_ppm = block.get("rate_ppm")
        if rate_ppm is None:
            raise Failure(f"{path.name}: G1 /20 缺 clock_injection.rate_ppm")

        rebases = (pace.get("clock_health") or {}).get("drift_rebases", 0)
        ahead = pace.get("elapsed_span_ns", 0) - pace.get("arrive_mono_span_ns", 0)
        frame_ns = pace.get("frame_duration_ns") or 0

        if not gate_enabled:
            # G2 对照组：门关着就一次都不该修。修了说明开关没接上 ——
            # 那会让"门关组"的数字看起来像"门开组"，对照白做。
            if rebases != 0:
                raise Failure(
                    f"{path.name}: G2 drift_gate.enabled=false 却复位了 {rebases} 次"
                )
            # G3 对照组必须真的越线，否则这组什么也没证明。
            if rate_ppm != 0 and ahead <= limit_ns:
                raise Failure(
                    f"{path.name}: G3 门关 + 速率注入，`clock_ahead` 只有 "
                    f"{ahead / 1e6:.2f} ms（未越过门限 {limit_ns / 1e6:.0f} ms）"
                    " —— 这一组没测到东西"
                )
        else:
            if rate_ppm != 0:
                # G4 门开 + 速率注入：门必须追住，且领先量不得越出门限太多。
                if rebases < 1:
                    raise Failure(
                        f"{path.name}: G4 速率 {rate_ppm} ppm 却一次没触发"
                    )
                if ahead > limit_ns + 2 * frame_ns:
                    raise Failure(
                        f"{path.name}: G4 门开着，`clock_ahead` 仍然涨到 "
                        f"{ahead / 1e6:.2f} ms（门限 {limit_ns / 1e6:.0f} ms）"
                    )
        gate_note = f"gate={'on' if gate_enabled else 'off'}"
        if rate_ppm:
            gate_note += f" rate={rate_ppm}ppm"

    injection_note = ""
    if schema in SCHEMA_WITH_INJECTION:
        block = doc.get("clock_injection")
        if not isinstance(block, dict):
            # I1：三格**总是**存在。缺了它，一份被注入过的证据和真实现场
            # 在字段上完全一样 —— 没人能判断那个 `clock_ahead` 是谁按的按钮。
            raise Failure(f"{path.name}: I1 /18 缺 clock_injection")
        offset = block.get("offset_ns")
        arm_after = block.get("arm_after_ns")
        armed_at = block.get("armed_at_position_ns")
        for name, value in (("offset_ns", offset), ("arm_after_ns", arm_after),
                            ("armed_at_position_ns", armed_at)):
            if value is None:
                raise Failure(f"{path.name}: I1 clock_injection 缺 {name}")

        # P13-AD 之后"注入"有两种：台阶（offset）与速率（rate_ppm）。
        # 只按 offset 判会把速率轮误判成"幽灵注入点"。
        rate_ppm_inj = block.get("rate_ppm", 0) or 0
        injected_any = offset != 0 or rate_ppm_inj != 0
        if not injected_any:
            # I2：没注入的一轮不许有"注入点"。非 -1 就是幽灵记录 ——
            # 那会让"这一轮是干净的"这句话失去意义。
            if armed_at != -1:
                raise Failure(
                    f"{path.name}: I2 未注入却记了 armed_at={armed_at}（幽灵注入点）"
                )
        else:
            # I3：给了参数就必须真的注入过。`armed_at == -1` 意味着片源没走到
            # 那个时刻，这一轮**什么都没测到**，不能当成"门没反应"。
            if armed_at < 0:
                raise Failure(
                    f"{path.name}: I3 注入了 {offset} ns 却从未触发"
                    "（片源没走到 arm_after_ns，这一轮什么都没测到）"
                )
            if armed_at < arm_after:
                raise Failure(
                    f"{path.name}: I3 armed_at={armed_at} < arm_after={arm_after}"
                )
        injection_note = f"inject={offset / 1e6:+.0f}ms@{armed_at / 1e6:.2f}ms" if offset else ""

    return {
        "status": "ok",
        "closure": closure_note,
        "injection_note": injection_note,
        "slew_note": slew_note,
        "gate_note": gate_note,
        "side_at_worst": side_at_worst,
        "span_note": span_note,
        "side": side,
        "present_step_ns": present_step,
        "due_step_ns": due_step,
        "lateness_ns": lateness,
        "due_step_frames": frames_over,
        "position": index / max(1, of - 1),
    }


# ---------------------------------------------------------------------------
# 变异自检：每条判据都要能被打红
# ---------------------------------------------------------------------------


def _good_doc() -> dict[str, Any]:
    return {
        "schema": "kivo.p12c.sync/20",
        "clock_injection": {
            "offset_ns": 0,
            "arm_after_ns": 0,
            "armed_at_position_ns": -1,
            "rate_ppm": 0,
        },
        "drift_gate": {"enabled": True, "limit_ns": 45_000_000, "band_ns": 5_000_000},
        "pace": {
            "presented": 480,
            "due_step_supplied": 480,
            "due_step_frames": 3,
            "frame_duration_ns": 16_666_667,
            "first_lateness_ns": 0,
            "last_lateness_ns": 88_231_396,
            "sum_present_step_ns": 18_208_333,
            "sum_due_step_ns": -70_023_063,
            "closure_gap_ns": 0,
            "arrive_mono_span_ns": 8_004_000_000,
            "elapsed_span_ns": 8_004_000_000,
            "due_span_ns": 7_983_500_000,
            "pts_span_ns": 7_983_500_000,
            "clock_supplied_frames": 480,
            "clock_health": {
                "drift_rebases": 21,
                "drift_max_correction_ns": 2_000_000,
                "drift_corrected_total_ns": 40_000_000,
            },
        },
        "av_drift": {
            "quantiles": {
                "per_frame": {
                    "samples": 480,
                    "worst": {
                        "index": 100,
                        "of": 480,
                        "media_ns": 1_666_666_700,
                        "lateness_ns": 88_231_396,
                        # 物理自洽：present_step = Δ − due_step = 88.023 − 70.023 = 18 ms，
                        # 落在 present_jitter 的 [−16.3, +19.6] 之内。
                        "due_step_ns": -70_023_063,
                        "cumulative_present_step_ns": 18_208_333,
                        "cumulative_due_step_ns": -70_023_063,
                        "lateness_at_first_ns": 0,
                        "clock_mono_ns": 1_700_000_000,
                        "clock_elapsed_ns": 1_683_500_000,
                        "anchor_pts_ns": 0,
                        # mono + pts − anchor − elapsed = 1.7e9 + 1.6666667e9 − 0 − 1.6835e9
                        "due_at_ns": 1_683_166_700,
                        "prev_lateness_ns": [125_000, 208_333],
                        "next_lateness_ns": [
                            16_000_000, 15_500_000, 15_000_000, 14_500_000,
                            14_000_000, 13_500_000, 13_000_000, 500_000,
                        ],
                    },
                }
            }
        },
        "present_jitter": {"max": 19_586_281, "histogram": {"min_ns": -16_317_000}},
        "frames_dropped": 0,
    }


def _mutations() -> list[tuple[str, dict[str, Any]]]:
    """(名字, 应该被抓到的坏文档)。"""
    out: list[tuple[str, dict[str, Any]]] = []

    doc = _good_doc()
    doc["pace"]["due_step_supplied"] = 479
    out.append(("P1 少接一帧", doc))

    doc = _good_doc()
    doc["av_drift"]["quantiles"]["per_frame"]["worst"]["next_lateness_ns"] = [1, 2]
    out.append(("P2 next 截短", doc))

    doc = _good_doc()
    doc["av_drift"]["quantiles"]["per_frame"]["worst"]["prev_lateness_ns"] = [125_000]
    out.append(("P2 prev 少一格", doc))

    doc = _good_doc()
    doc["av_drift"]["quantiles"]["per_frame"]["worst"]["next_lateness_ns"] = [99_000_000]
    out.append(("P3 next 超过 max", doc))

    doc = _good_doc()
    # due_step 说 -71 ms，但 lateness 只比前一帧高 1 ms ⇒ present_step 要 -72 ms，
    # 而直方图说全场最快也只有 -16.3 ms ⇒ 恒等式被违反。
    doc["av_drift"]["quantiles"]["per_frame"]["worst"]["lateness_ns"] = 1_208_333
    out.append(("P4 恒等式被违反", doc))

    doc = _good_doc()
    # 报告层忘了把 episode 传进来（P13-P 真撞到过一次：schema 升了、字段没写）。
    del doc["av_drift"]["quantiles"]["per_frame"]["worst"]["due_step_ns"]
    out.append(("P2 缺 due_step（报告层漏接线）", doc))

    doc = _good_doc()
    doc["pace"]["due_step_frames"] = 0
    out.append(("P5 due_step 超帧长却记 0", doc))

    doc = _good_doc()
    doc["pace"]["clock_supplied_frames"] = 0
    out.append(("R1 钟读数没接线", doc))

    doc = _good_doc()
    doc["pace"]["due_span_ns"] += 2_000_000
    out.append(("R1 跨度不闭合", doc))

    doc = _good_doc()
    doc["av_drift"]["quantiles"]["per_frame"]["worst"]["due_at_ns"] += 5_000_000
    out.append(("R2 最差帧 due 不等于定义式", doc))

    doc = _good_doc()
    doc["pace"]["closure_gap_ns"] = 123_456
    out.append(("Q1 残差被改写", doc))

    doc = _good_doc()
    doc["pace"]["first_lateness_ns"] = 5_000
    doc["pace"]["closure_gap_ns"] = 0
    out.append(("Q3 首帧 lateness 非 0", doc))

    doc = _good_doc()
    doc["av_drift"]["quantiles"]["per_frame"]["worst"]["cumulative_due_step_ns"] = -60_023_063
    out.append(("Q2 最差帧累积账不闭合", doc))

    # P13-AD：门配置的两种坏法。
    doc = _good_doc()
    del doc["drift_gate"]
    out.append(("G1 缺 drift_gate", doc))

    doc = _good_doc()
    doc["drift_gate"]["enabled"] = False
    out.append(("G2 门关着却复位了", doc))

    # P13-AC：修正总量与注入量对不上 —— 修的不是该修的那一段。
    doc = _good_doc()
    doc["clock_injection"] = {
        "offset_ns": 80_000_000,
        "arm_after_ns": 2_000_000_000,
        "armed_at_position_ns": 2_190_000_000,
    }
    doc["pace"]["clock_health"]["drift_corrected_total_ns"] = 10_000_000
    out.append(("A5 修正总量与注入量对不上", doc))

    # P13-AB：注入三格的三条坏法。每一条都对应一种"证据说了谎"的方式。
    doc = _good_doc()
    doc["av_drift"]["quantiles"]["per_frame"]["worst"]["due_step_ns"] = -45_000_000
    out.append(("P5 due_step 超帧长却记 0", doc))

    doc = _good_doc()
    del doc["pace"]["clock_health"]["drift_corrected_total_ns"]
    out.append(("A1 缺累计修正", doc))

    doc = _good_doc()
    doc["pace"]["clock_health"]["drift_rebases"] = 0
    out.append(("A3 没触发却有幅度", doc))

    doc = _good_doc()
    del doc["clock_injection"]
    out.append(("I1 缺 clock_injection", doc))

    doc = _good_doc()
    doc["clock_injection"]["armed_at_position_ns"] = 2_000_000_000
    out.append(("I2 未注入却有注入点", doc))

    doc = _good_doc()
    doc["clock_injection"]["offset_ns"] = 80_000_000
    doc["clock_injection"]["arm_after_ns"] = 2_000_000_000
    doc["clock_injection"]["armed_at_position_ns"] = -1
    out.append(("I3 注入了却从未触发", doc))

    return out


def self_test() -> int:
    failures = 0
    path = Path("<self-test>")
    result = check_file(path, _good_doc())
    if result["status"] != "ok" or result["side"] != "due_side":
        failures += 1
        print(f"  FAIL 正例应判 due_side，实得 {result}")
    print("[PASS] 正例：due 侧 70 ms ⇒ due_side")

    for name, doc in _mutations():
        try:
            check_file(path, doc)
        except Failure:
            print(f"[PASS] 变异被抓到：{name}")
            continue
        failures += 1
        print(f"[FAIL] 变异没被抓到：{name}")
    return failures


def main(argv: list[str]) -> int:
    if len(argv) >= 2 and argv[1] == "--self-test":
        failures = self_test()
        print(f"--- 自检失败 {failures} 项 ---")
        return 0 if failures == 0 else 1

    paths = [Path(p) for p in argv[1:]]
    if not paths:
        print("用法：python tools/verify_p13p_episode.py <sync.json...> | --self-test")
        return 2

    failures = 0
    checked = 0
    skipped = 0
    sides: dict[str, int] = {}
    for path in sorted(paths):
        for doc in _reports(path):
            try:
                result = check_file(path, doc)
            except Failure as exc:
                failures += 1
                print(f"[FAIL] {exc}")
                continue
            if result["status"] == "skip":
                skipped += 1
                continue
            checked += 1
            sides[result["side"]] = sides.get(result["side"], 0) + 1
            print(
                f"[ OK ] {path.name}: side={result['side']:<12} "
                f"due_step={result['due_step_ns'] / 1e6:>8.3f} ms "
                f"lateness={result['lateness_ns'] / 1e6:>8.3f} ms "
                f"pos={result['position']:.2f} {result.get('side_at_worst', '')} "
                f"{result.get('span_note', '')} {result.get('injection_note', '')} "
                f"{result.get('slew_note', '')} {result.get('gate_note', '')}"
            )

    print(f"--- 复算 {checked} 份，跳过 {skipped} 份（旧 schema），失败 {failures} 份 ---")
    if checked:
        print("--- 归类：" + "，".join(f"{k}={v}" for k, v in sorted(sides.items())) + " ---")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
