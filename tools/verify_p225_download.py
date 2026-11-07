#!/usr/bin/env python3
"""225 下载管理 · 证据校验器。

校验 docs/evidence/p225/p225-summary.json 的自洽性 + 真实文件指纹：
  AC1 下载完整性：ffprobe JSON 存在且 duration=60s / size 与 evidence 一致
  AC2 通知：进度/完成截图 PNG 有效且 sha256 与 manifest 一致
  AC3 媒体库：截图 PNG 有效 + library_count 递增 + downloaded_visible 非空
  AC4 续传：resume_first_observed > interrupt 字节数（断点接续，不归零）
自检：6 个变异全部抓得到（--mutate）。
用法：python -X utf8 -I tools/verify_p225_download.py [--mutate]  退出码 0=全绿。
"""
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

EV = Path(r"C:/kivo video/docs/evidence/p225")
SUMMARY = EV / "p225-summary.json"
FFPROBE = EV / "p225-ac1-ffprobe.json"


def png_ok(path: Path) -> bool:
    """PNG 魔数 + IHDR CRC + IEND 尾完整（变异截断/改字节能打红）。"""
    try:
        data = path.read_bytes()
        if not data.startswith(b"\x89PNG\r\n\x1a\n"):
            return False
        if b"IEND" not in data[-16:]:
            return False
        if len(data) < 33 or data[12:16] != b"IHDR":
            return False
        ihdr_len = struct.unpack(">I", data[8:12])[0]
        crc = struct.unpack(">I", data[29:33])[0]
        return ihdr_len == 13 and zlib.crc32(data[12:29]) == crc
    except OSError:
        return False


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def check(ev, failures):
    def want(cond, label):
        if not cond:
            failures.append(label)

    # AC1
    ac1 = ev["ac1_download"]
    probe = json.loads(FFPROBE.read_text(encoding="utf-8"))
    want(sha256(FFPROBE) == ev.get("ffprobe_sha256"), "ac1: ffprobe JSON 指纹不符")
    fmt = probe["format"]
    want(abs(float(fmt["duration"]) - 60.0) < 0.5, "ac1: ffprobe duration != 60s")
    want(int(fmt["size"]) == ac1["bytes_on_device"], "ac1: size 与 ffprobe 不一致")
    want(probe["streams"][0]["codec_name"] == "h264", "ac1: 视频轨缺失")
    want(len(probe["streams"]) >= 2, "ac1: 音频轨缺失")
    want(len(ac1["sha256"]) == 64
         and all(c in "0123456789abcdef" for c in ac1["sha256"]),
         "ac1: sha256 非法")

    # AC2
    for key, name in (("progress_shot", "p225-progress.png"),
                      ("completion_shot", "p225-notification.png")):
        shot = ev["ac2_notification"][key]
        want(shot["png"] == name and shot["observed"].strip() != "",
             f"ac2: {key} 文案缺失")
        want(png_ok(EV / name), f"ac2: {name} PNG 无效")
        want(sha256(EV / name) == ev["screenshot_sha256"][name],
             f"ac2: {name} 指纹不符")
    want("isForeground=true" in ev["ac2_notification"]["fgs_record"],
         "ac2: FGS 记录缺失")

    # AC3
    ac3 = ev["ac3_library"]
    want(ac3["library_count_after"] == ac3["library_count_before"] + 1,
         "ac3: 媒体库计数未 +1")
    want(ac3["downloaded_visible"].strip() != "", "ac3: 入库可见性缺失")
    want(png_ok(EV / ac3["png"]), "ac3: 媒体库截图无效")
    want(sha256(EV / ac3["png"]) == ev["screenshot_sha256"][ac3["png"]],
         "ac3: 截图指纹不符")

    # AC4
    ac4 = ev["ac4_resume"]
    want(ac4["resume_first_observed"] > ac4["interrupt"]["bytes_at_force_stop"],
         "ac4: 续传起点未越过断点（疑似重下）")
    want(ac4["interrupt"]["bytes_at_force_stop"] > 0, "ac4: 断点为 0")
    want(ac4["file"].strip() != "", "ac4: 续传对象缺失")
    want(ac4["final_bytes"] > ac4["resume_first_observed"], "ac4: 终值未越过续传起点")
    samples = ac4.get("samples", [])
    want(len(samples) >= 5, "ac4: 采样点不足")
    want(all(b >= a for a, b in zip(samples, samples[1:])), "ac4: 尺寸采样回落（疑似重下）")
    want(samples[-1] == ac4["final_bytes"], "ac4: 终值与最后采样不一致")
    want(sha256(EV / "p225-resume-done.png") == ev["screenshot_sha256"]["p225-resume-done.png"],
         "ac4: 完成截图指纹不符")


MUTATIONS = [
    ("ac1 size", lambda ev: ev["ac1_download"].__setitem__("bytes_on_device", 12345)),
    ("ac1 sha", lambda ev: ev["ac1_download"].__setitem__("sha256", "x" * 64)),
    ("ac2 observed", lambda ev: ev["ac2_notification"]["progress_shot"].__setitem__("observed", "")),
    ("ac2 png", lambda ev: ev["screenshot_sha256"].__setitem__("p225-progress.png", "y" * 64)),
    ("ac3 count", lambda ev: ev["ac3_library"].__setitem__("library_count_after", 157)),
    ("ac4 resume", lambda ev: ev["ac4_resume"].__setitem__("resume_first_observed", 1024)),
    ("ac4 monotonic", lambda ev: ev["ac4_resume"]["samples"].__setitem__(3, 1024)),
]


def main() -> int:
    ev = json.loads(SUMMARY.read_text(encoding="utf-8"))
    if "--mutate" in sys.argv:
        caught = 0
        for label, mutate in MUTATIONS:
            broken = json.loads(SUMMARY.read_text(encoding="utf-8"))
            mutate(broken)
            failures: list[str] = []
            check(broken, failures)
            if failures:
                caught += 1
                print(f"mutation caught: {label} -> {failures[0]}")
            else:
                print(f"MISSED: {label}")
        print(f"{caught}/{len(MUTATIONS)} mutations caught")
        return 0 if caught == len(MUTATIONS) else 1

    failures: list[str] = []
    check(ev, failures)
    if failures:
        for f in failures:
            print(f"FAIL: {f}")
        return 1
    print("p225 verify: all green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
