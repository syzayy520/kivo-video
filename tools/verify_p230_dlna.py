#!/usr/bin/env python3
"""230 DLNA 投屏 · 证据校验器。

校验 docs/evidence/p230/p230-summary.json 自洽性 + 真实文件指纹：
  AC1 发现：日志含 "scan done: 2 devices" 且含两台渲染器名
  AC2 投本地：PNG 指纹 + 链路含 LAN 代理描述
  AC3 URL 直传：PNG 指纹 + 链路含 emby 直连描述 + 手机状态文案
  AC4 控制：Stop 日志证据 + 本校验器 5 变异自检（--mutate）
用法：python -X utf8 -I tools/verify_p230_dlna.py [--mutate]  退出码 0=全绿。
"""
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

EV = Path(r"C:/kivo video/docs/evidence/p230")
SUMMARY = EV / "p230-summary.json"
CAST_LOG = EV / "p230-status-cast.log"


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

    log = CAST_LOG.read_text(encoding="utf-8", errors="replace")

    # AC1
    want("scan done: 2 devices" in log, "ac1: 日志无 2 devices 扫描结果")
    want(ev["ac1_discovery"]["found"] in log, "ac1: 摘要 found 与日志不符")
    want("2 devices" in ev["ac1_discovery"]["found"], "ac1: 摘要 found 未记录 2 台")
    want("小爱音箱-8245" in log, "ac1: 真渲染器(小爱)未出现在发现结果")
    want("kivo-test-renderer" in log, "ac1: 测试渲染器未出现在发现结果")
    want(len(ev["renderers"]) == 2, "ac1: 渲染器清单数量不符")

    # AC2
    want("0.0.0.0" in ev["ac2_cast_local"]["chain"], "ac2: LAN 代理链路描述缺失")
    want(png_ok(EV / ev["ac2_cast_local"]["evidence_png"]), "ac2: ffplay 截图无效")
    want(sha256(EV / ev["ac2_cast_local"]["evidence_png"]) ==
         ev["screenshot_sha256"][ev["ac2_cast_local"]["evidence_png"]],
         "ac2: 截图指纹不符")

    # AC3
    want("emby" in ev["ac3_cast_url"]["chain"].lower(), "ac3: URL 直传链路描述缺失")
    want(png_ok(EV / ev["ac3_cast_url"]["evidence_png"]), "ac3: Emby ffplay 截图无效")
    want(sha256(EV / ev["ac3_cast_url"]["evidence_png"]) ==
         ev["screenshot_sha256"][ev["ac3_cast_url"]["evidence_png"]],
         "ac3: 截图指纹不符")
    want(ev["ac3_cast_url"]["phone_status"].startswith("投屏中"),
         "ac3: 手机投屏状态文案缺失")

    # AC4
    want("SOAP Stop" in ev["ac4_control"]["stop"], "ac4: Stop 证据缺失")
    want("两次" in ev["ac4_control"]["stop"], "ac4: Stop 未达两次")


MUTATIONS = [
    ("ac1 devices", lambda ev: ev["ac1_discovery"].__setitem__("found", "0 devices")),
    ("ac1 renderer", lambda ev: ev["renderers"].pop()),
    ("ac2 chain", lambda ev: ev["ac2_cast_local"].__setitem__("chain", "loopback only")),
    ("ac2 sha", lambda ev: ev["screenshot_sha256"].__setitem__("p230-pc-ffplay.png", "x" * 64)),
    ("ac4 stop", lambda ev: ev["ac4_control"].__setitem__("stop", "none")),
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
    print("p230 verify: all green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
