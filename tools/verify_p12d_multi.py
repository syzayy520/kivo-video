#!/usr/bin/env python3
"""P12-D2 多实例并发解码的独立校验。

卡片判据：**同时 3 路 1080p 不互相拖垮，各自 fps ≥ 单路 60%。**

这个脚本只读证据（`kivo.p12c.decode/6`），不 import 任何项目代码，
也不接受手填的 fps —— 基线从单路证据里取中位数，比例在这里算。

两种检查，分开跑：

    # ① 吞吐比：3 路各自 fps / 单路基线中位数 ≥ 0.6
    python tools/verify_p12d_multi.py ratio \
        --baseline "docs/evidence/p12d/p12d2-baseline-r*.json" \
        --multi    "docs/evidence/p12d/p12d2-multi-r*.json"

    # ② 真实节拍：3 路同时按 30fps 跑，谁都不许丢帧
    python tools/verify_p12d_multi.py paced \
        --files "docs/evidence/p12d/p12d2-paced-*.json" --target-fps 30

为什么要分两种：①是卡片字面判据（能跑多快），②才是用户真正感知的
（同时播会不会卡）。只测①会得到"3 路都很快"而忽略"是不是每一帧都上屏了"。
"""

from __future__ import annotations

import argparse
import glob
import json
import statistics
import sys
from pathlib import Path

MIN_RATIO = 0.60
"""卡片写死的门槛。不许为了让结果好看而下调。"""

MAX_INTRA_SPREAD = 0.10
"""同轮内最快与最慢的差 / 中位数。超过 10% 说明有某一路被饿着了。"""


class Failure(Exception):
    pass


def load(pattern: str) -> list[dict]:
    paths = sorted(glob.glob(pattern))
    if not paths:
        raise Failure(f"没有匹配到文件：{pattern}")
    out = []
    for raw in paths:
        path = Path(raw)
        data = json.loads(path.read_text(encoding="utf-8"))
        schema = data.get("schema")
        if schema != "kivo.p12c.decode/6":
            raise Failure(f"{path.name}: schema 是 {schema!r}，期望 'kivo.p12c.decode/6'")
        data["_name"] = path.name
        out.append(data)
    return out


def fps_of(data: dict) -> float:
    return float(data["timing"]["decode_fps"])


def check_common(data: dict) -> None:
    name = data["_name"]
    if not data["outcome"]["completed"]:
        raise Failure(f"{name}: completed=false（这一路没跑完，数字不能比）")
    if data["outcome"]["error"]:
        raise Failure(f"{name}: error={data['outcome']['error']!r}")
    counters = data["counters"]
    received = int(counters["frames_received"])
    rendered = int(counters["frames_rendered"])
    dropped = int(counters["frames_dropped_late"])
    discarded = int(counters.get("frames_discarded_no_surface", 0))
    if int(counters["frames_released"]) != received:
        raise Failure(f"{name}: frames_released != frames_received（有输出槽没归还）")
    if rendered + dropped + discarded + 1 != received:
        raise Failure(f"{name}: 帧数不闭合（{rendered}+{dropped}+{discarded}+1 != {received}）")
    if fps_of(data) <= 0.0:
        raise Failure(f"{name}: decode_fps 为 0")


def run_group_key(name: str) -> str:
    """`p12d2-multi-r2-1.json` → `p12d2-multi-r2`。

    按"轮"分组，而不是把所有多路文件混在一起比 —— 不同轮之间设备状态不同，
    混着比会把轮间波动算成"某一路被拖垮"。
    """
    stem = name[:-5] if name.endswith(".json") else name
    head, _, tail = stem.rpartition("-")
    return head if tail.isdigit() else stem


def check_ratio(baseline_pattern: str, multi_pattern: str) -> int:
    baseline_files = load(baseline_pattern)
    for data in baseline_files:
        check_common(data)
    baseline_fps = [fps_of(d) for d in baseline_files]
    median_baseline = statistics.median(baseline_fps)
    if median_baseline <= 0:
        raise Failure("基线 fps 为 0，没法算比例")

    print(f"基线（单路 {len(baseline_files)} 轮）：")
    for data, fps in zip(baseline_files, baseline_fps):
        print(f"  {data['_name']:<28} {fps:8.2f} fps")
    print(f"  中位数 {median_baseline:.2f} fps（比例的分母）"
          f"，极差 {max(baseline_fps) - min(baseline_fps):.2f} fps")
    print()

    groups: dict[str, list[dict]] = {}
    for data in load(multi_pattern):
        check_common(data)
        groups.setdefault(run_group_key(data["_name"]), []).append(data)

    exit_code = 0
    for group in sorted(groups):
        members = sorted(groups[group], key=lambda d: d["_name"])
        fps_values = [fps_of(d) for d in members]
        median_group = statistics.median(fps_values)
        spread = (max(fps_values) - min(fps_values)) / median_group if median_group else 1.0
        total = sum(fps_values)

        print(f"{group}（{len(members)} 路）：")
        worst_ratio = 1.0
        for data, fps in zip(members, fps_values):
            ratio = fps / median_baseline
            worst_ratio = min(worst_ratio, ratio)
            flag = "OK " if ratio >= MIN_RATIO else "FAIL"
            print(f"  {flag} {data['_name']:<28} {fps:8.2f} fps  = 基线的 {ratio:6.1%}")
        print(f"  合计 {total:8.2f} fps（单路的 {total / median_baseline:.2f}×）"
              f"，轮内极差 {spread:.1%}，最差比例 {worst_ratio:.1%}")

        if worst_ratio < MIN_RATIO:
            print(f"  FAIL 最差比例 {worst_ratio:.1%} < {MIN_RATIO:.0%}（卡片门槛）")
            exit_code = 1
        if spread > MAX_INTRA_SPREAD:
            print(f"  FAIL 轮内极差 {spread:.1%} > {MAX_INTRA_SPREAD:.0%}：有某一路被饿着了")
            exit_code = 1
        print()
    return exit_code


def check_paced(files_pattern: str, target_fps: float) -> int:
    files = load(files_pattern)
    groups: dict[str, list[dict]] = {}
    for data in files:
        check_common(data)
        if not data["pace"]["enabled"]:
            raise Failure(f"{data['_name']}: 这一轮没开节拍，不能当真实节拍证据")
        groups.setdefault(run_group_key(data["_name"]), []).append(data)

    exit_code = 0
    for group in sorted(groups):
        members = sorted(groups[group], key=lambda d: d["_name"])
        print(f"{group}（{len(members)} 路，目标 {target_fps:g} fps）：")
        for data in members:
            fps = fps_of(data)
            dropped = int(data["counters"]["frames_dropped_late"])
            frames = int(data["counters"]["frames_rendered"])
            # 真实节拍下"跑得比目标快"没有意义（会被节拍器压住），
            # 有意义的是：一帧不丢，且平均帧率跟得上目标。
            slow = fps < target_fps * 0.98
            flag = "OK " if (dropped == 0 and not slow) else "FAIL"
            print(f"  {flag} {data['_name']:<28} {fps:8.2f} fps  dropped={dropped:<3} frames={frames}")
            if dropped != 0:
                print("       FAIL 有丢帧：同时播 3 路会卡")
                exit_code = 1
            if slow:
                print(f"       FAIL 平均帧率 {fps:.2f} 跟不上目标 {target_fps:g}")
                exit_code = 1
        print()
    return exit_code


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    ratio = sub.add_parser("ratio", help="吞吐比检查（卡片字面判据）")
    ratio.add_argument("--baseline", required=True)
    ratio.add_argument("--multi", required=True)

    paced = sub.add_parser("paced", help="真实节拍检查（用户感知判据）")
    paced.add_argument("--files", required=True)
    paced.add_argument("--target-fps", type=float, default=30.0)

    args = parser.parse_args(argv)

    try:
        if args.command == "ratio":
            code = check_ratio(args.baseline, args.multi)
        else:
            code = check_paced(args.files, args.target_fps)
    except Failure as failure:
        print(f"FAIL  {failure}")
        return 1
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
