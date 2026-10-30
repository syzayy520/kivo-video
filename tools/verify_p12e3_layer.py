#!/usr/bin/env python3
"""P12-E3 独立复算：从原始行重算图层色域，并与探针字段逐项比对。

用法：

    python tools/verify_p12e3_layer.py docs/evidence/p12e3/*.json

**不 import `probe_layer_dataspace`**（与 D5 的规矩一致：共用代码只能证明"它跟自己一致"）。
本脚本用自己的一套正则，从产物里的 `raw_lines` 重新解析，再与结构化字段比对；
两组之间还要比一次"必须不同"，否则"标记根本没起作用"会被读成通过。

判据（写死，别放宽）：

| 组 | 断言 |
| --- | --- |
| HDR | dataspace 名 == `BT2020_ITU_PQ`；metadata types != 0；dynamic_range == `HDR` |
| SDR | dataspace 名 != `BT2020_ITU_PQ`；metadata types == 0；dynamic_range == `SDR` |
| 对照 | 两组的 dataspace 数值必须不同 |
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

# 自己的一套正则：故意与探针写法不同（探针用 `dataspace=(\w+) \((\d+)\)`）。
OWN_DATASPACE = re.compile(r"dataspace\s*=\s*([A-Za-z0-9_]+)\s*\(\s*(\d+)\s*\)")
OWN_RANGE = re.compile(r"current\s+dynamic_range\s*:\s*([A-Za-z]+)", re.IGNORECASE)

HDR_DATASPACE = "BT2020_ITU_PQ"


def check_file(path: Path) -> tuple[dict, list[str]]:
    """返回 (产物摘要, 失败原因列表)。"""
    failures: list[str] = []
    payload = json.loads(path.read_text(encoding="utf-8"))
    raw = payload.get("raw_lines", {})

    name = payload.get("layer_dataspace", {}).get("name")
    value = payload.get("layer_dataspace", {}).get("value")
    meta = payload.get("layer_dataspace", {}).get("hdr_metadata_types")
    dyn = payload.get("display", {}).get("dynamic_range")

    # ① 从原始行独立重算 —— 字段与原文对不上就是探针有 bug，先于一切判据。
    raw_layer = raw.get("layer") or ""
    match = OWN_DATASPACE.search(raw_layer)
    if not match:
        failures.append(f"{path.name}: raw_lines.layer 里解析不出 dataspace：{raw_layer!r}")
    else:
        if match.group(1) != name:
            failures.append(f"{path.name}: 名字字段 {name} 与原始行 {match.group(1)} 不一致")
        if int(match.group(2)) != value:
            failures.append(f"{path.name}: 数值字段 {value} 与原始行 {match.group(2)} 不一致")

    raw_dyn = raw.get("dynamic_range") or ""
    dyn_match = OWN_RANGE.search(raw_dyn)
    if not dyn_match:
        failures.append(f"{path.name}: raw_lines.dynamic_range 里解析不出状态：{raw_dyn!r}")
    elif dyn_match.group(1).upper() != str(dyn).upper():
        failures.append(f"{path.name}: dynamic_range 字段 {dyn} 与原始行 {dyn_match.group(1)} 不一致")

    return {"file": path.name, "tag": payload.get("tag"), "name": name, "value": value,
            "meta": meta, "dynamic_range": dyn}, failures


def main(argv: list[str]) -> int:
    paths = [Path(a) for a in argv[1:]]
    if not paths:
        print("用法：verify_p12e3_layer.py <evidence.json> [...]", file=sys.stderr)
        return 2

    rows: list[dict] = []
    failures: list[str] = []
    for path in paths:
        row, problems = check_file(path)
        rows.append(row)
        failures.extend(problems)

    hdr_rows = [r for r in rows if str(r["tag"] or "").startswith("hdr")]
    sdr_rows = [r for r in rows if str(r["tag"] or "").startswith("sdr")]

    if not hdr_rows or not sdr_rows:
        failures.append(
            f"分组不全：HDR {len(hdr_rows)} 份、SDR {len(sdr_rows)} 份 —— "
            "没有对照组时任何结论都不成立"
        )

    for row in hdr_rows:
        if row["name"] != HDR_DATASPACE:
            failures.append(f"{row['file']}: HDR 轮 dataspace 是 {row['name']}，期望 {HDR_DATASPACE}")
        if not row["meta"]:
            failures.append(f"{row['file']}: HDR 轮 hdr metadata types = {row['meta']}，期望非 0")
        if str(row["dynamic_range"]).upper() != "HDR":
            failures.append(f"{row['file']}: HDR 轮 dynamic_range = {row['dynamic_range']}")

    for row in sdr_rows:
        if row["name"] == HDR_DATASPACE:
            failures.append(f"{row['file']}: SDR 轮 dataspace 竟然是 {HDR_DATASPACE}")
        if row["meta"] not in (0, None):
            failures.append(f"{row['file']}: SDR 轮 hdr metadata types = {row['meta']}，期望 0")
        if str(row["dynamic_range"]).upper() != "SDR":
            failures.append(f"{row['file']}: SDR 轮 dynamic_range = {row['dynamic_range']}")

    if hdr_rows and sdr_rows:
        hdr_values = {r["value"] for r in hdr_rows}
        sdr_values = {r["value"] for r in sdr_rows}
        if hdr_values & sdr_values:
            failures.append(
                f"对照组没有区分度：HDR 与 SDR 都出现 dataspace 数值 {sorted(hdr_values & sdr_values)}"
            )

    for row in rows:
        print(
            f"[{'PASS' if not any(row['file'] in f for f in failures) else 'FAIL'}] "
            f"{row['file']}: {row['name']}({row['value']}) meta={row['meta']} "
            f"range={row['dynamic_range']}"
        )

    print(f"\n--- {len(rows)} 份产物，{len(failures)} 项不符 ---")
    for failure in failures:
        print(f"  ! {failure}")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
