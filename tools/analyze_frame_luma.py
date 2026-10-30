#!/usr/bin/env python3
"""P12-E4 帧亮度统计：把截图变成可比数字（中位数 / 分位 / 亮暗截断比例）。

用法：

    python tools/analyze_frame_luma.py --ffmpeg third_party/ffmpeg-sdk/bin/ffmpeg.exe \
        --out docs/evidence/p12e4/p12e4-luma.json \
        hdr10=artifacts/p12e4/a-hdr-marked.png sdrtag=artifacts/p12e4/b-sdr-marked.png

为什么不用 numpy/PIL：**本机没有**（实测 `No module named 'PIL'`）。
仓库里有 FFmpeg SDK，`ffmpeg -f rawvideo -pix_fmt rgb24 -` 就能把 PNG 解成裸像素，
再用纯 Python 统计 —— 少一个依赖，别人复算时也不必装环境。

口径（写死，别改）：

- 亮度按 **Rec.709 系数**在 **sRGB 编码值**上算：`Y = 0.2126R + 0.7152G + 0.0722B`。
  这是"截屏后看到的编码值"，**不是线性光** —— 做 tone mapping 的对比足够，
  但不要拿它去算绝对尼特。
- 截断两档固定：亮截断 `Y >= 250`、暗截断 `Y <= 5`，按像素占比报。
- 只比较**同一分辨率**的两张图；尺寸不同直接报错，不缩放（缩放会改变分布）。
"""
from __future__ import annotations

import argparse
import json
import statistics
import subprocess
import sys
import time
from pathlib import Path

SCHEMA = "kivo.p12e4.frame-luma/1"


class Failure(RuntimeError):
    pass


def probe_size(ffmpeg: str, image: Path) -> tuple[int, int]:
    """用 ffprobe 同目录的 ffprobe 取尺寸；没有就退回 ffmpeg -i 的 stderr 解析。"""
    ffprobe = Path(ffmpeg).with_name("ffprobe.exe")
    if not ffprobe.exists():
        alt = Path(ffmpeg).with_name("ffprobe")
        ffprobe = alt if alt.exists() else ffprobe
    if ffprobe.exists():
        out = subprocess.run(
            [str(ffprobe), "-v", "error", "-select_streams", "v:0",
             "-show_entries", "stream=width,height", "-of", "csv=p=0:s=x", str(image)],
            capture_output=True, text=True, timeout=60,
        )
        text = out.stdout.strip().splitlines()
        if text and "x" in text[0]:
            width, height = text[0].split("x")[:2]
            return int(width), int(height)
    raise Failure(f"取不到 {image} 的尺寸（ffprobe 不可用）")


def read_rgb(ffmpeg: str, image: Path, width: int, height: int) -> bytes:
    out = subprocess.run(
        [ffmpeg, "-v", "error", "-i", str(image), "-f", "rawvideo", "-pix_fmt", "rgb24", "-"],
        capture_output=True, timeout=120,
    )
    expected = width * height * 3
    if len(out.stdout) != expected:
        raise Failure(f"{image}: 解出 {len(out.stdout)} 字节，期望 {expected}")
    return out.stdout


def analyze(data: bytes) -> dict:
    total = len(data) // 3
    # 主循环用 memoryview + 切片，纯 Python 也能在 2 秒内跑完 1080x2400。
    values: list[int] = []
    bright = 0
    dark = 0
    mv = memoryview(data)
    for index in range(0, len(data), 3):
        red = mv[index]
        green = mv[index + 1]
        blue = mv[index + 2]
        y = int(0.2126 * red + 0.7152 * green + 0.0722 * blue)
        values.append(y)
        if y >= 250:
            bright += 1
        if y <= 5:
            dark += 1

    values.sort()
    percentile = lambda p: values[min(len(values) - 1, int(len(values) * p))]  # noqa: E731
    return {
        "pixels": total,
        "luma_mean": round(statistics.fmean(values), 2),
        "luma_p05": percentile(0.05),
        "luma_p50": percentile(0.50),
        "luma_p95": percentile(0.95),
        "luma_max": values[-1],
        "bright_clip_ratio": round(bright / total, 5),
        "dark_clip_ratio": round(dark / total, 5),
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="截图亮度统计与对比")
    parser.add_argument("frames", nargs="+", help="label=path.png ...")
    parser.add_argument("--ffmpeg", default="third_party/ffmpeg-sdk/bin/ffmpeg.exe")
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv[1:])

    rows: list[dict] = []
    sizes: set[tuple[int, int]] = set()
    for item in args.frames:
        if "=" not in item:
            raise Failure(f"参数要写成 label=path：{item}")
        label, path_text = item.split("=", 1)
        path = Path(path_text)
        if not path.exists():
            raise Failure(f"图不存在：{path}")
        width, height = probe_size(args.ffmpeg, path)
        sizes.add((width, height))
        stats = analyze(read_rgb(args.ffmpeg, path, width, height))
        rows.append({"label": label, "file": str(path).replace("\\", "/"),
                     "width": width, "height": height, **stats})

    if len(sizes) != 1:
        raise Failure(f"所有图必须同尺寸（尺寸集合 {sorted(sizes)}）；不缩放是为了保住分布")

    payload = {
        "schema": SCHEMA,
        # 证据必须带实测时间戳（F4 层 1）；没有它分不清真机产物与手写片段。
        "generated_at_unix_ms": int(time.time() * 1000),
        "method": "Rec.709 luma on sRGB-encoded RGB values from the screenshot; NOT linear light",
        "thresholds": {"bright_clip": 250, "dark_clip": 5},
        "frames": rows,
    }
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    for row in rows:
        print(
            f"[{row['label']}] {row['width']}x{row['height']} "
            f"mean={row['luma_mean']} p50={row['luma_p50']} "
            f"p05={row['luma_p05']} p95={row['luma_p95']} max={row['luma_max']} "
            f"bright_clip={row['bright_clip_ratio']:.2%} dark_clip={row['dark_clip_ratio']:.2%}"
        )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except Failure as exc:
        print(f"失败：{exc}", file=sys.stderr)
        sys.exit(2)
