#!/usr/bin/env python3
"""P12-E3 色域标记探针：从 `dumpsys SurfaceFlinger` 读"这一层被标成了什么色域"。

用法（应用必须先播起来，层存在才有数据）：

    python tools/probe_layer_dataspace.py --tag hdr10-r1 \
        --out docs/evidence/p12e3/p12e3-hdr10-r1-layer.json

四条硬规矩（前三条是 D5 踩出来的，第 4 条是 E2/E3 自己的）：

1. **层名必须带 `#id` 与 `(BLAST)`。** 少一个都匹配不到，而且**不报错** ——
   上一轮就是把它当成了"平台不支持"。本脚本取不到就退出，并打印候选层名。
2. **只取一份 dump。** 图层 dataspace 与显示级 dynamic_range 必须出自同一份 dump，
   跨 dump 比较会把"滞后一轮"读成"不一致"。
3. **`HDR Panel:true` 不是判据。** SDR 播放时它同样是 true —— 那是面板能力，不是状态。
   本脚本照记，但校验器不用它。
4. **层名来自 `--list` 现取，不写死。** `#id` 每次播放都会变。
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

SCHEMA = "kivo.p12e3.layer-dataspace/1"

DATASPACE_RE = re.compile(r"dataspace=([A-Za-z0-9_]+) \((\d+)\)")
META_RE = re.compile(r"hdr metadata types=(\d+)")
DYNAMIC_RANGE_RE = re.compile(r"current dynamic_range:\s*(\w+)")
COLOR_MODE_RE = re.compile(r"Current Color Mode:\s*(\w+)")
HDR_PANEL_RE = re.compile(r"HDR Panel:(\w+)")


class Failure(RuntimeError):
    pass


def adb_path() -> str:
    return os.environ.get("ADB") or "adb"


def run(adb: str, args: list[str], serial: str | None, timeout: float = 60.0) -> str:
    command = [adb]
    if serial:
        command += ["-s", serial]
    command += ["shell", *args]
    proc = subprocess.run(command, capture_output=True, text=False, timeout=timeout)
    return proc.stdout.decode("utf-8", errors="replace").replace("\r", "")


def find_layer(adb: str, serial: str | None, package: str) -> str:
    """现取 BLAST SurfaceView 层名（含 `#id`）。取不到直接失败，不回落。"""
    listing = run(adb, ["dumpsys", "SurfaceFlinger", "--list"], serial)
    pattern = re.compile(rf"^SurfaceView\[{re.escape(package)}/.*\]\(BLAST\)#\d+$")
    for line in listing.splitlines():
        name = line.strip()
        if pattern.match(name):
            return name
    candidates = [ln.strip() for ln in listing.splitlines() if package in ln]
    raise Failure(
        f"没找到 {package} 的 BLAST SurfaceView 层。\n"
        f"`--list` 里与包名相关的行：\n  " + "\n  ".join(candidates[:12])
    )


def layer_block(dump: str, layer_name: str) -> list[str]:
    """截出该层的 dump 块（从 `* Layer` 到下一个 `* Layer`）。"""
    lines = dump.splitlines()
    for index, line in enumerate(lines):
        if line.startswith("* Layer") and f"({layer_name})" in line:
            block = [line]
            for follow in lines[index + 1 :]:
                if follow.startswith("* Layer"):
                    break
                block.append(follow)
            return block
    raise Failure(f"dump 里没有层 {layer_name} 的块（层可能已销毁）")


def pick(pattern: re.Pattern[str], lines: list[str]) -> re.Match[str] | None:
    for line in lines:
        match = pattern.search(line)
        if match:
            return match
    return None


def group(pattern: re.Pattern[str], lines: list[str], index: int = 1) -> str | None:
    """取第一个匹配的第 index 组。没有就 None —— 与 `pick()` 分开，
    避免在调用处写 `(... or [None, None])[1]` 那种既难读又容易错的东西。"""
    match = pick(pattern, lines)
    return match.group(index) if match else None


def main() -> int:
    parser = argparse.ArgumentParser(description="读 SF 图层 dataspace 与显示 dynamic range")
    parser.add_argument("--tag", required=True, help="这一轮的标签，如 hdr10-r1")
    parser.add_argument("--package", default="com.kivo.cinema")
    parser.add_argument("--serial", default=None)
    parser.add_argument("--out", required=True)
    parser.add_argument("--raw-out", default=None, help="原始 dump 落盘路径（可选）")
    args = parser.parse_args()

    adb = adb_path()
    layer_name = find_layer(adb, args.serial, args.package)
    dump = run(adb, ["dumpsys", "SurfaceFlinger"], args.serial)

    block = layer_block(dump, layer_name)
    dataspace = pick(DATASPACE_RE, block)
    metadata = pick(META_RE, block)
    if dataspace is None:
        raise Failure(
            f"层 {layer_name} 的块里没有 dataspace 字段。\n块的前几行：\n  "
            + "\n  ".join(block[:6])
        )

    all_lines = dump.splitlines()
    payload = {
        "schema": SCHEMA,
        "tag": args.tag,
        "package": args.package,
        "layer": layer_name,
        "layer_dataspace": {
            "name": dataspace.group(1),
            "value": int(dataspace.group(2)),
            # 3 = SMPTE2086 | CTA861_3（两位一起）。**不解释成"有 HDR 静态元数据"** ——
            # 那是 SF 自己按缓冲里带的东西标的位；要断言"画质对"是 E4 的事。
            "hdr_metadata_types": int(metadata.group(1)) if metadata else None,
        },
        "display": {
            "dynamic_range": group(DYNAMIC_RANGE_RE, all_lines),
            "color_mode": group(COLOR_MODE_RE, all_lines),
            # 面板能力，**不是状态**：SDR 播放时同样是 true。照记，别当判据。
            "hdr_panel": group(HDR_PANEL_RE, all_lines),
        },
        # 原始行：读数与字段对不上时靠它复算。只留相关的几行，不落整份 dump。
        "raw_lines": {
            "layer": next((ln.strip() for ln in block if "dataspace=" in ln), None),
            "dynamic_range": next(
                (ln.strip() for ln in all_lines if "current dynamic_range:" in ln), None
            ),
            "color_mode": next(
                (ln.strip() for ln in all_lines if "Current Color Mode:" in ln), None
            ),
            "hdr_panel": next((ln.strip() for ln in all_lines if "HDR Panel:" in ln), None),
        },
    }

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    if args.raw_out:
        Path(args.raw_out).parent.mkdir(parents=True, exist_ok=True)
        Path(args.raw_out).write_text(dump, encoding="utf-8")

    print(
        f"[{args.tag}] layer={layer_name} "
        f"dataspace={payload['layer_dataspace']['name']}"
        f"({payload['layer_dataspace']['value']}) "
        f"meta={payload['layer_dataspace']['hdr_metadata_types']} "
        f"dynamic_range={payload['display']['dynamic_range']} "
        f"color_mode={payload['display']['color_mode']}"
    )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Failure as exc:
        print(f"失败：{exc}", file=sys.stderr)
        sys.exit(2)
