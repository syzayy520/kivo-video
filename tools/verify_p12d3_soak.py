#!/usr/bin/env python3
"""P12-D3 长稳测试的独立校验。

读两样东西，全部来自实测，不接受手填：

    soak-mem.csv                主机侧 `dumpsys meminfo` 采样（PSS）
    p12d3-soak-r1-t####.json    应用内每 60 s 写的完整 decode/6 快照

判据（`29-p12d3-design.md` §五）：

    内存增长 < 5 MB      ← 暖机后 5 点中位数 与 末 5 点中位数 之差
    frames_dropped 无累积 ← 逐分钟快照全程 0（或斜率 ≈ 0）
    帧数闭合 / 播完 54000 帧

用法：

    python tools/verify_p12d3_soak.py \
        --mem build/avtmp/soak-mem.csv \
        --snapshots "docs/evidence/p12d/p12d3-soak-r1-t*.json" \
        --final docs/evidence/p12d/p12d3-soak-r1.json \
        --max-growth-mb 5
"""

from __future__ import annotations

import argparse
import csv
import glob
import json
import statistics
import sys
from pathlib import Path

IN_FLIGHT_SLACK = 16
"""中途快照允许的在途缓冲数。

取 16 而不是 3：BufferQueue 通常 3–4 槽，但快照是在收帧线程之外抢的一瞬，
撞上 `dequeue` 已返回、`release` 还没调的窗口并不罕见。给到 16 是为了
"绝不放过大问题（比如漏交几十个槽），也不把正常在途当故障"。
"""

WARMUP_SAMPLES = 5
"""判据用的窗口：前 5 个样本算"暖机后水位"，末 5 个算"稳定后水位"。

为什么不用单点首末：PSS 有 ±5MB 的自然抖动（GC / page cache / ashmem 回收），
单点相减会把抖动当成增长或下降。5 点中位数是压抖动的最小代价。
"""


class Failure(Exception):
    pass


def read_mem(path: Path, window: tuple[int, int] | None = None,
             quiet: bool = False) -> list[tuple[int, int, int]]:
    """读内存采样，可按时间窗裁剪。

    **必须裁窗口。** 采样器是主机侧独立跑的，它不知道应用什么时候停；
    应用一停，codec / Surface / 解码线程全部释放，PSS 会从 ~105MB 掉到 ~60MB。
    把这几行算进"末段水位"，会得出"内存下降 45MB"这种荒唐结论 ——
    那测的是停机，不是长稳。
    """
    rows: list[tuple[int, int, int]] = []
    dropped = 0
    with path.open(encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            try:
                epoch_ms = int(row["epoch_ms"])
                entry = (int(row["elapsed_s"]), int(row["pss_total_kb"]), epoch_ms)
            except (KeyError, ValueError):
                continue
            if window is not None and not (window[0] <= epoch_ms <= window[1]):
                dropped += 1
                continue
            rows.append(entry)
    if not rows:
        raise Failure(f"{path} 里没有可用的内存样本")
    if dropped:
        print(f"（窗口裁剪：丢弃 {dropped} 个采样点，它们不在播放期内）")

    # ── 再裁一次：切掉"停机"那一段 ──
    #
    # 播放结束的一瞬间，codec / Surface / 解码线程全部释放，PSS 从 ~105MB 掉到 ~60MB。
    # 那一步不是"长稳表现"，是"东西被放掉了"。留在样本里会把末段水位拉低 45MB，
    # 得出"内存下降"这种荒唐结论 —— 实际观测里第一次就踩到了（t=1780 从 105.3 掉到 61.0）。
    #
    # 判据用"相对全程中位数掉 20% 以上"来识别停机，而不是写死一个秒数：
    # 采样器与应用各有自己的起点，两边秒数天然差十几秒，写死必然裁错。
    if rows:
        base_kb = statistics.median(kb for _, kb, _ in rows)
        threshold_kb = base_kb * 0.8
        for index, (_, kb, _) in enumerate(rows):
            if kb < threshold_kb:
                if not quiet:
                    print(f"（停机裁剪：丢弃 {len(rows) - index} 个播放结束后的采样点，"
                          f"起点 elapsed={rows[index][0]}s、PSS={kb}KB < {threshold_kb:.0f}KB）")
                rows = rows[:index]
                break
    if not rows:
        raise Failure(f"{path} 裁剪后没有样本了：全程都不在运行状态？")
    return rows


def check_memory(path: Path, max_growth_mb: float,
                 window: tuple[int, int] | None = None) -> tuple[float, float]:
    rows = read_mem(path, window)
    if len(rows) < WARMUP_SAMPLES * 2:
        raise Failure(f"内存样本只有 {len(rows)} 个，不足以做首末窗口比较")

    head = [kb for _, kb, _ in rows[:WARMUP_SAMPLES]]
    tail = [kb for _, kb, _ in rows[-WARMUP_SAMPLES:]]
    baseline_mb = statistics.median(head) / 1024.0
    final_mb = statistics.median(tail) / 1024.0
    growth_mb = final_mb - baseline_mb

    peak_mb = max(kb for _, kb, _ in rows) / 1024.0
    trough_mb = min(kb for _, kb, _ in rows) / 1024.0
    span_s = rows[-1][0] - rows[0][0]

    print(f"内存（{len(rows)} 个样本 / {span_s}s）：")
    print(f"  暖机后中位数 {baseline_mb:8.2f} MB（前 {WARMUP_SAMPLES} 点）")
    print(f"  末段中位数   {final_mb:8.2f} MB（后 {WARMUP_SAMPLES} 点）")
    print(f"  增长         {growth_mb:+8.2f} MB（判据 < {max_growth_mb:g} MB）")
    print(f"  全程区间     {trough_mb:.2f} – {peak_mb:.2f} MB（抖动 {peak_mb - trough_mb:.2f} MB）")

    if growth_mb >= max_growth_mb:
        raise Failure(f"内存增长 {growth_mb:.2f} MB ≥ {max_growth_mb:g} MB")
    return baseline_mb, growth_mb


def load_snapshots(pattern: str) -> list[dict]:
    paths = sorted(glob.glob(pattern))
    if not paths:
        raise Failure(f"没有匹配到快照：{pattern}")
    out = []
    for raw in paths:
        path = Path(raw)
        data = json.loads(path.read_text(encoding="utf-8"))
        if not data.get("schema", "").startswith("kivo.p12c.decode/"):
            raise Failure(f"{path.name}: schema 不是 decode")
        data["_name"] = path.name
        out.append(data)
    return out


def check_frames(snapshots: list[dict], final: dict) -> None:
    print(f"\n帧计数（{len(snapshots)} 份中途快照 + 1 份结束证据）：")

    previous_rendered = -1
    previous_dropped = -1
    total_dropped_at = []
    for data in snapshots + [final]:
        counters = data["counters"]
        rendered = int(counters["frames_rendered"])
        dropped = int(counters["frames_dropped_late"])
        discarded = int(counters.get("frames_discarded_no_surface", 0))
        total_dropped_at.append(dropped)

        if rendered < previous_rendered:
            raise Failure(f"{data['_name']}: frames_rendered 倒退了"
                          f"（{previous_rendered} → {rendered}）")
        if dropped < previous_dropped:
            raise Failure(f"{data['_name']}: frames_dropped_late 倒退了"
                          f"（{previous_dropped} → {dropped}）")
        previous_rendered = rendered
        previous_dropped = dropped

        received = int(counters["frames_received"])
        released = int(counters["frames_released"])
        # 中途快照**不是静止态**：从 `dequeueOutputBuffer` 拿到索引到 `release` 之间
        # 总有 1–3 帧在路上。所以中途只要求"缺口很小"，严格的
        # `released == received` 只在会话停下之后才成立（见下面的结束证据检查）。
        in_flight = received - released
        if in_flight < 0 or in_flight > IN_FLIGHT_SLACK:
            raise Failure(f"{data['_name']}: 在途缓冲 {in_flight} 超出 [0, {IN_FLIGHT_SLACK}]")
        if rendered + dropped + discarded + 1 > received:
            raise Failure(
                f"{data['_name']}: 帧数超出收帧数"
                f"（{rendered}+{dropped}+{discarded}+1 > {received}）"
            )

    first = snapshots[0]["counters"]
    last = final["counters"]
    print(f"  首个快照 rendered={first['frames_rendered']} "
          f"dropped={first['frames_dropped_late']}")
    print(f"  结束证据 rendered={last['frames_rendered']} "
          f"dropped={last['frames_dropped_late']}")

    # "无累积趋势"按字面执行：任何一个采样点出现非 0 就算失败。
    # 不是"最后总数很小就放过" —— 那会漏掉"前 25 分钟为 0、最后 5 分钟开始丢"。
    nonzero = [d for d in total_dropped_at if d != 0]
    if nonzero:
        raise Failure(f"出现丢帧：逐点序列 {total_dropped_at}")
    print("  丢帧：逐点全为 0 ✅（无累积趋势）")


def check_final(final: dict, expected_frames: int) -> None:
    print("\n结束状态：")
    counters = final["counters"]
    received = int(counters["frames_received"])
    released = int(counters["frames_released"])
    rendered = int(counters["frames_rendered"])
    dropped = int(counters["frames_dropped_late"])
    discarded = int(counters.get("frames_discarded_no_surface", 0))
    # 会话已停，这时才要求严格闭合。
    if released != received:
        raise Failure(f"结束证据 frames_released({released}) != frames_received({received})")
    if rendered + dropped + discarded + 1 != received:
        raise Failure(f"结束证据帧数不闭合（{rendered}+{dropped}+{discarded}+1 != {received}）")

    if not final["outcome"]["completed"]:
        raise Failure("completed=false：这一轮没播完")
    if final["outcome"]["error"]:
        raise Failure(f"error={final['outcome']['error']!r}")
    rendered = int(final["counters"]["frames_rendered"])
    print(f"  上屏 {rendered} 帧（片源 {expected_frames} 帧）")
    if rendered != expected_frames:
        raise Failure(f"上屏帧数 {rendered} != 片源帧数 {expected_frames}")

    pace = final["pace"]
    if pace["clock_mix"]["system_frames"] != pace["frames_decided"]:
        raise Failure("无音轨的片子却有帧走了音频时钟 —— 时钟口径错了")
    health = pace["clock_health"]
    if health["free_run_frames"] or health["jump_rebases"]:
        raise Failure(f"时钟不健康：{health}")

    print("  时钟：全程走 system 档，free_run / jump_rebases 均为 0 ✅")


def check_sync(sync: dict) -> None:
    """可选的 `kivo.p12c.sync/1` 检查。

    为什么单独一个：`decode/6` 里没有 `timeline`（那是合并报表的字段），
    而"播放速率是否实时"这条判据只在那里。两个 schema 各管一摊，不硬凑。
    """
    timeline = sync["timeline"]
    rate = float(timeline["playback_rate"])
    span = timeline["present_wall_span_ns"] / 1e9
    print(f"\n合并报表（kivo.p12c.sync/1）：")
    print(f"  播放速率 {rate:.6f}，呈现跨度 {span:.2f}s（片源 1800s）")
    if not 0.98 <= rate <= 1.02:
        raise Failure(f"播放速率 {rate:.4f} 偏离实时")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mem", required=True)
    parser.add_argument("--snapshots", required=True)
    parser.add_argument("--final", required=True)
    parser.add_argument("--max-growth-mb", type=float, default=5.0)
    parser.add_argument("--expected-frames", type=int, default=54000)
    parser.add_argument("--sync", default=None,
                        help="可选的 kivo.p12c.sync/1 文件，用于检查播放速率")
    args = parser.parse_args(argv)

    try:
        snapshots = load_snapshots(args.snapshots)
        final = json.loads(Path(args.final).read_text(encoding="utf-8"))

        # ── 时钟对齐：**两侧的绝对时间不能直接比** ──
        #
        # 设备侧 `generated_at_unix_ms` 来自设备系统时钟；主机侧 epoch 来自本机。
        # K40 这台机器的系统时钟比真实 UTC **快 8 小时**
        # （`adb shell date -u` 与主机 `date -u` 差整整 8h），所以两边直接相减
        # 会得到"播放窗口在 8 小时之后"这种结果，然后所有内存样本都被裁掉。
        #
        # 做法：用首个快照（设备时钟，对应"起播后 60s"）与首个内存样本（主机时钟，
        # 对应"采样开始后几秒"）估一个偏移量，把设备时间换算到主机时间轴。
        # 偏移量打出来 —— 它本身就是一个该被看见的事实，不该被藏进代码里。
        mem_rows = read_mem(Path(args.mem), quiet=True)
        first_snapshot_epoch = int(snapshots[0]["generated_at_unix_ms"])
        host_first_epoch = mem_rows[0][2]
        # 首个快照在 t=60s（采样间隔写死在播放器里）；首个内存样本在 t≈5s。
        offset_ms = first_snapshot_epoch - (host_first_epoch + 55_000)
        playback_start_host = first_snapshot_epoch - 60_000 - offset_ms
        playback_end_host = int(final["generated_at_unix_ms"]) - offset_ms
        print(f"设备时钟相对主机偏移 {offset_ms / 3_600_000:+.2f} 小时"
              f"（设备证据时间戳不能与主机时间直接相减）")
        print(f"播放窗口（换算到主机时间轴）："
              f"{playback_start_host} → {playback_end_host} "
              f"（{(playback_end_host - playback_start_host) / 1000:.0f}s）\n")

        check_memory(Path(args.mem), args.max_growth_mb,
                     (playback_start_host, playback_end_host))
        check_frames(snapshots, final)
        check_final(final, args.expected_frames)
        if args.sync:
            check_sync(json.loads(Path(args.sync).read_text(encoding="utf-8")))
    except Failure as failure:
        print(f"\nFAIL  {failure}")
        return 1

    print("\n全部通过：内存增长在判据内，丢帧无累积趋势，帧数闭合，播完整段。")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
