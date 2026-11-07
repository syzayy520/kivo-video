#!/usr/bin/env python3
"""231 私密区 · 证据校验器。

校验 docs/evidence/p231/p231-summary.json 自洽性 + 真实文件指纹：
  AC1 门：弹窗文本 dump 含系统提示 + 取消 reason + 成功路径记录
  AC2 隐藏：after 记录非空 + 截图指纹
  AC3 私密区：解锁状态 + 网格截图指纹
  AC4 往返：移出记录 + 恢复截图指纹 + store 终态为空
自检：6 个变异全部抓得到（--mutate）。
用法：python -X utf8 -I tools/verify_p231_private.py [--mutate]  退出码 0=全绿。
"""
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

EV = Path(r"C:/kivo video/docs/evidence/p231")
SUMMARY = EV / "p231-summary.json"
PROMPT_DUMP = EV / "p231-prompt-dump.txt"


def png_ok(path: Path) -> bool:
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
    dump = PROMPT_DUMP.read_text(encoding="utf-8", errors="replace")
    want("请触摸指纹传感器" in dump, "ac1: 弹窗系统提示缺失")
    want("取消" in dump, "ac1: 取消按钮缺失")
    parts = ev["ac1_gate"]["prompt_texts"].split(" / ")
    want(len(parts) >= 2 and parts[1] in dump, "ac1: 摘要弹窗文案与 dump 不符")
    want("已取消" in ev["ac1_gate"]["cancel_reason"], "ac1: 取消 reason 缺失")
    want("NONE_ENROLLED" in ev["ac1_gate"]["noenrolled_reason"],
         "ac1: 未录入负路径记录缺失")

    # AC2
    want(ev["ac2_hide"]["after"].strip() != "", "ac2: 隐藏观察记录缺失")
    want(png_ok(EV / "p231-hidden.png"), "ac2: 隐藏截图无效")
    want(sha256(EV / "p231-hidden.png") == ev["screenshot_sha256"]["p231-hidden.png"],
         "ac2: 截图指纹不符")

    # AC3
    want("1 个私密条目" in ev["ac3_zone"]["unlocked_status"], "ac3: 解锁状态缺失")
    want(png_ok(EV / "p231-grid.png"), "ac3: 私密网格截图无效")
    want(sha256(EV / "p231-grid.png") == ev["screenshot_sha256"]["p231-grid.png"],
         "ac3: 截图指纹不符")

    # AC4
    want("0 个私密条目" in ev["ac4_roundtrip"]["remove"], "ac4: 移出记录缺失")
    want(png_ok(EV / "p231-restored2.png"), "ac4: 恢复截图无效")
    want(sha256(EV / "p231-restored2.png") ==
         ev["screenshot_sha256"]["p231-restored2.png"], "ac4: 截图指纹不符")
    want("[]" in ev["store_final"], "ac4: store 终态非空")


MUTATIONS = [
    ("ac1 prompt", lambda ev: ev["ac1_gate"].__setitem__("cancel_reason", "none")),
    ("ac1 texts", lambda ev: ev["ac1_gate"].__setitem__("prompt_texts", "nothing")),
    ("ac2 hide", lambda ev: ev["ac2_hide"].__setitem__("after", "")),
    ("ac2 sha", lambda ev: ev["screenshot_sha256"].__setitem__("p231-hidden.png", "x" * 64)),
    ("ac3 status", lambda ev: ev["ac3_zone"].__setitem__("unlocked_status", "locked")),
    ("ac4 store", lambda ev: ev.__setitem__("store_final", '["1"]')),
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
    print("p231 verify: all green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
