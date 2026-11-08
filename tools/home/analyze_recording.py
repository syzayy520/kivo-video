"""Sample an original recording without a computer-vision dependency.

Usage: python tools/home/analyze_recording.py VIDEO OUTPUT --start 0 --duration 32
Outputs timestamped contact sheets and per-frame hero differences, at 5 Hz.
These differences locate candidate events; they do not infer gestures or playback.
"""
import argparse
import csv
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("video", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--start", type=float, default=0)
    parser.add_argument("--duration", type=float, default=32)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    info = json.loads(subprocess.check_output([
        "ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
        "stream=width,height:stream_tags=rotate:stream_side_data=rotation", "-of", "json", str(args.video)]))
    stream = info["streams"][0]
    width, height = stream["width"], stream["height"]
    rotation = int(stream.get("tags", {}).get("rotate", 0))
    for side in stream.get("side_data_list", []):
        rotation = int(side.get("rotation", rotation))
    if abs(rotation) % 180 == 90:
        width, height = height, width
    sample_height = round(height * 390 / width)
    probe = subprocess.check_output([
        "ffmpeg", "-v", "error", "-ss", str(args.start), "-i", str(args.video),
        "-t", str(args.duration), "-vf", f"fps=5,scale=390:{sample_height}", "-f", "rawvideo",
        "-pix_fmt", "rgb24", "-threads", "1", "pipe:1"])
    frames = np.frombuffer(probe, np.uint8).reshape(-1, sample_height, 390, 3)
    with (args.output / "frame-differences.csv").open("w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["time_s", "upper_hero_mean_absolute_difference", "lower_hero_mean_absolute_difference"])
        for i in range(1, len(frames)):
            delta = np.abs(frames[i].astype(float) - frames[i - 1].astype(float))
            writer.writerow([round(args.start + i / 5, 2), round(delta[70:350, 30:360].mean(), 3),
                             round(delta[400:580, 30:360].mean(), 3)])
    # One image per second, 8 per sheet. Full-resolution samples at 0.2s remain
    # recoverable from the source; avoid committing hundreds of redundant frames.
    selected = list(range(0, len(frames), 5))
    for batch in range(0, len(selected), 8):
        indices = selected[batch:batch + 8]
        sheet = Image.new("RGB", (390 * 4, (sample_height + 30) * 2), "#dddddd")
        draw = ImageDraw.Draw(sheet)
        for j, index in enumerate(indices):
            x, y = j % 4 * 390, j // 4 * (sample_height + 30)
            sheet.paste(Image.fromarray(frames[index]), (x, y + 30))
            draw.text((x + 8, y + 7), f"{args.start + index / 5:.1f} s", fill="black")
        sheet.save(args.output / f"contact-{batch // 8}.jpg", quality=90)
    print(f"Sampled {len(frames)} frames: {args.output}")


if __name__ == "__main__":
    main()
