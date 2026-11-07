#!/usr/bin/env python3
"""224 Emby 对接 · 证据校验器。

校验 docs/evidence/p224/p224-summary.json 的自洽性 + 真实文件指纹：
  AC1 登录：两台真服务器（空密码 / 有密码）登录均记录 items 数 ≥ 10
  AC2 海报墙：截图 PNG 有效 + 指纹一致 + 文案记录非空
  AC3 流播：硬解日志行存在（url-play harddec ok）+ 分辨率记录 + 节拍比 ∈ [0.90, 1.10]
  AC4 本校验器自检：5 个变异全部抓得到（--mutate）
用法：python -X utf8 -I tools/verify_p224_emby.py [--mutate]  退出码 0=全绿。
"""
import hashlib
import json
import re
import struct
import sys
import zlib
from pathlib import Path

EV = Path(r"C:/kivo video/docs/evidence/p224")
SUMMARY = EV / "p224-summary.json"
STATUS_LOG = EV / "p224-status.log"


def png_ok(path: Path) -> bool:
    """PNG 魔数 + IHDR CRC + IEND 尾完整。"""
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

    # AC1：双服务器登录
    for s in ev["servers"]:
        want(s["login"] == "ok", f"ac1: {s['host']} 登录未标记 ok")
        want(s["items"] >= 10, f"ac1: {s['host']} 条目数不足（{s['items']}）")
        want(s["auth_mode"] in ("empty-password", "password"),
             f"ac1: {s['host']} auth_mode 非法")
    want(len(ev["servers"]) >= 2, "ac1: 双服务器路径未覆盖")

    # AC2：海报墙
    for name, label in (("p224-login1.png", "server1 海报墙"),
                        ("p224-login2.png", "server2 海报墙")):
        want(png_ok(EV / name), f"ac2: {label} 截图无效")
        want(sha256(EV / name) == ev["screenshot_sha256"][name],
             f"ac2: {label} 指纹不符")
    want(ev["poster_wall"]["observed"].strip() != "", "ac2: 海报墙观察记录缺失")

    # AC3：直连流播 + 节拍（日志含历史轮次——取最后一次 url-play 块）
    log = STATUS_LOG.read_text(encoding="utf-8", errors="replace")
    want(re.search(r"url-play harddec ok \d+x\d+", log) is not None,
         "ac3: 硬解成功日志行缺失")
    begins = re.findall(r"url-play begin url=(\S+)", log)
    want(bool(begins) and ev["playback"]["stream_host"] in begins[-1],
         "ac3: 起播 URL 主机与记录不符")
    want(ev["playback"]["width"] >= 1920, "ac3: 分辨率记录异常")
    want(0.90 <= ev["playback"]["rate"] <= 1.10,
         f"ac3: 节拍比越界（{ev['playback']['rate']:.3f}）")
    for n in ("p224-play2.png", "p224-play3.png"):
        want(png_ok(EV / n), f"ac3: {n} 截图无效")
        want(sha256(EV / n) == ev["screenshot_sha256"][n], f"ac3: {n} 指纹不符")

    # 节拍对自洽
    pace = ev["playback"]["pace_pair"]
    d_wall = pace["b_wall_ms"] - pace["a_wall_ms"]
    d_media = pace["b_pos_s"] - pace["a_pos_s"]
    want(d_wall > 0 and d_media > 0, "ac3: 节拍对非正")
    want(abs(d_media * 1000.0 / d_wall - ev["playback"]["rate"]) < 0.005,
         "ac3: rate 与节拍对不自洽")


MUTATIONS = [
    ("ac1 login", lambda ev: ev["servers"][0].__setitem__("login", "failed")),
    ("ac1 items", lambda ev: ev["servers"][1].__setitem__("items", 3)),
    ("ac2 sha", lambda ev: ev["screenshot_sha256"].__setitem__("p224-login1.png", "x" * 64)),
    ("ac3 rate", lambda ev: ev["playback"].__setitem__("rate", 1.60)),
    ("ac3 log", lambda ev: ev["playback"].__setitem__("stream_host", "nowhere.example.com")),
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
    print("p224 verify: all green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
