#!/usr/bin/env python3
"""往 MP4 的 `moov` 里插入一个 `pssh` box（P12-F3 用）。

**为什么需要它**：本机没有 MP4Box / shaka-packager，而 ffmpeg 的
`-encryption_scheme cenc-aes-ctr` 只写 `encv/sinf/tenc/senc`，**从不写 `pssh`**。
于是我们手上没有任何一份「带 `pssh` 的 CENC」样本 —— 而"设备为什么不报加密"
这件事恰恰需要拿它来做对照实验（`50-p12f3-status.md` §六）。

**它只改容器，不碰码流**：插入点选在 `moov` 内部、`udta` 之前，而这份文件的
布局是 `ftyp / free / mdat / moov` —— `mdat` 在 `moov` 之前，所以 `stco` 里的
绝对偏移一个都不需要改。这是唯一能安全插入的位置，换别的布局会算错偏移。

用法：
    python tools/inject_pssh.py 输入.mp4 输出.mp4 [--system-id HEX]
"""
from __future__ import annotations

import argparse
import pathlib
import struct
import sys

# Widevine 的 system id —— 用它是因为设备侧最可能认识它；
# 认不认识其实不影响结论：这个实验只问"多一个 pssh 会不会改变 is_encrypted"。
DEFAULT_SYSTEM_ID = "edef8ba979d64acea3c827dcd51d21ed"

# pssh 的 data 部分：一版 Widevine 风格的最小 payload（KID 数 0）。
# 内容不重要 —— 见上文。长度为 4 字节的 KID 计数即可自洽。
PSSH_DATA = struct.pack(">I", 0)


def make_pssh(system_id_hex: str) -> bytes:
    system_id = bytes.fromhex(system_id_hex)
    if len(system_id) != 16:
        raise ValueError(f"system id must be 16 bytes, got {len(system_id)}")
    payload = b"\x00\x00\x00\x00" + system_id + struct.pack(">I", len(PSSH_DATA)) + PSSH_DATA
    return struct.pack(">I", 8 + len(payload)) + b"pssh" + payload


def find_moov(data: bytes) -> tuple[int, int]:
    """返回 (moov 起始偏移, moov 大小)。只认顶层 box。"""
    off = 0
    while off + 8 <= len(data):
        size = struct.unpack(">I", data[off : off + 4])[0]
        typ = data[off + 4 : off + 8]
        if size == 1:
            size = struct.unpack(">Q", data[off + 8 : off + 16])[0]
        elif size == 0:
            size = len(data) - off
        if size < 8:
            raise ValueError(f"corrupt top-level box at {off} (size={size})")
        if typ == b"moov":
            return off, size
        off += size
    raise ValueError("no moov box found")


def moov_children(data: bytes, start: int, size: int) -> list[tuple[int, int, bytes]]:
    out: list[tuple[int, int, bytes]] = []
    off = start + 8
    end = start + size
    while off + 8 <= end:
        child = struct.unpack(">I", data[off : off + 4])[0]
        typ = data[off + 4 : off + 8]
        if child < 8 or off + child > end:
            break
        out.append((off, child, typ))
        off += child
    return out


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="insert a pssh box into moov")
    ap.add_argument("source")
    ap.add_argument("target")
    ap.add_argument("--system-id", default=DEFAULT_SYSTEM_ID)
    args = ap.parse_args(argv)

    path = pathlib.Path(args.source)
    if not path.is_file():
        print(f"missing input: {path}", file=sys.stderr)
        return 2
    data = path.read_bytes()
    if b"pssh" in data:
        print("input already contains pssh; refusing to add a second one", file=sys.stderr)
        return 2

    moov_off, moov_size = find_moov(data)
    children = moov_children(data, moov_off, moov_size)
    # 插在 udta 之前；没有 udta 就插在 moov 末尾。
    insert_at = next((off for off, _, typ in children if typ == b"udta"), moov_off + moov_size)

    pssh = make_pssh(args.system_id)
    out = bytearray()
    out += data[:moov_off]
    out += struct.pack(">I", moov_size + len(pssh)) + b"moov" + data[moov_off + 8 : insert_at]
    out += pssh
    out += data[insert_at:moov_off + moov_size]
    out += data[moov_off + moov_size :]

    pathlib.Path(args.target).write_bytes(bytes(out))
    print(
        f"inserted pssh ({len(pssh)} bytes) into moov at {insert_at}; "
        f"moov {moov_size} -> {moov_size + len(pssh)}; "
        f"mdat stays before moov so stco offsets are unchanged"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
