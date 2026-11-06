#!/usr/bin/env python3
"""往 MP4 的 HEVC 采样条目（`stsd` 里）插入一个 `dvcC` box（210 卡用）。

**为什么需要它**：手上没有真 DV 片源，而 210 卡的判据里有两个不需要真 DV 像素——
profile 8.x 本来就按 HDR10 透传（不需要 DV 元数据本体）、profile 5 在 `open()` 就被拒
（根本不解码）。所以「HEVC 流 + 手工注入 dvcC」足以驱动识别与拒绝路径
（`210-dv-a-detect-passthrough-design.md` §五 的合成兜底）。真 DV 样片到位后可作对照。

**它只改容器，不碰码流**。安全前提（不满足就拒绝执行）：`mdat` 必须在 `moov` 之前——
往 `moov` 深处插盒会撑大 moov，`mdat` 在后面则 `stco` 里的绝对偏移一个都不用改
（与 `inject_pssh.py` 同款前提，那条经验来自 P12-F3）。

**dvcC 载荷布局**（对照 KSPlayer swiftinterface:875 的字段；逐位布局以
FFmpeg `mov_read_dvcC` 的读法为准——它就是生态里被最广泛检验的实现）：

    [0]    dv_version_major (8)
    [1]    dv_version_minor (8)
    [2..3] dv_profile (7) | dv_level (6) | rpu_present (1) | el_present (1) | bl_present (1)
    [4]    bl_signal_compatibility_id (4) | reserved (4)
    [5..23] reserved (全 0)                       —— 载荷共 24 字节，盒共 32 字节

用法：
    python tools/inject_dvcc.py 输入.mp4 输出.mp4 --profile 8 --level 1
    python tools/inject_dvcc.py 输入.mp4 输出.mp4 --profile 5 --level 6
（rpu/el/bl/bl-sig 默认按 profile 推：8 → rpu=1 el=0 bl=1 sig=1；5 → rpu=1 el=0 bl=0 sig=0）
"""
from __future__ import annotations

import argparse
import pathlib
import struct
import sys

# 视觉采样条目的固定体长：6 保留 + 2 dri + 2 pre_defined + 2 保留 + 12 pre_defined
# + 2 宽 + 2 高 + 4+4 分辨率 + 4 保留 + 2 frame_count + 32 压缩器名 + 2 depth + 2 pre_defined。
VISUAL_SAMPLE_ENTRY_FIXED = 78
SAMPLE_ENTRY_HEADER = 8
HEVC_ENTRY_TYPES = (b"hvc1", b"hev1")
DV_ENTRY_TYPES = (b"dvh1", b"dvhe")
CONTAINERS = (b"moov", b"trak", b"mdia", b"minf", b"stbl")


def box_size(data: bytes, off: int) -> int:
    size = struct.unpack(">I", data[off : off + 4])[0]
    if size == 1:
        raise ValueError(f"64-bit box size not supported at {off}")
    if size == 0:
        size = len(data) - off
    return size


def top_level_boxes(data: bytes) -> list[tuple[int, int, bytes]]:
    out, off = [], 0
    while off + 8 <= len(data):
        size = box_size(data, off)
        if size < 8 or off + size > len(data):
            raise ValueError(f"corrupt top-level box at {off} (size={size})")
        out.append((off, size, data[off + 4 : off + 8]))
        off += size
    return out


def children(data: bytes, start: int, size: int) -> list[tuple[int, int, bytes]]:
    out, off, end = [], start + 8, start + size
    while off + 8 <= end:
        child = box_size(data, off)
        if child < 8 or off + child > end:
            break
        out.append((off, child, data[off + 4 : off + 8]))
        off += child
    return out


def find_video_sample_entry(
    data: bytes, start: int, size: int, chain: list[int]
) -> tuple[list[int], int, int, bytes] | None:
    """深度优先找第一个 HEVC 采样条目，返回 (祖先链偏移, 条目偏移, 条目大小, 类型)。

    祖先链含 moov…stsd 全部容器（不含采样条目自身）——它们的大小都要补插盒量。
    """
    for off, child_size, typ in children(data, start, size):
        if typ in HEVC_ENTRY_TYPES:
            # 条目自身前面那层就是 stsd；stsd 的头 8 字节之后还有 4+4（版本/flags + entry_count），
            # 但那是 stsd 的内部布局，不改条目相对位置，只改 stsd 总大小。
            return chain, off, child_size, typ
        if typ == b"stsd":
            inner = children(data, off, child_size)
            # stsd 的前 8 字节是头，随后 8 字节是 version/flags + entry_count，
            # children() 从 +8 起扫，会把那 8 字节误当 box —— 所以这里手动走采样条目。
            entry_off = off + 16
            if entry_off + 8 <= off + child_size:
                entry_size = box_size(data, entry_off)
                entry_type = data[entry_off + 4 : entry_off + 8]
                if entry_type in HEVC_ENTRY_TYPES:
                    return chain + [off], entry_off, entry_size, entry_type
                if entry_type in DV_ENTRY_TYPES:
                    raise ValueError("input already declares a DV sample entry (dvh1/dvhe)")
                # 第一条不是 HEVC（音轨等）——继续找别的 trak
        if typ in CONTAINERS:
            hit = find_video_sample_entry(data, off, child_size, chain + [off])
            if hit is not None:
                return hit
    return None


def make_dvcc(profile: int, level: int, rpu: int, el: int, bl: int, bl_sig: int) -> bytes:
    if not 0 <= profile < 128 or not 0 <= level < 64:
        raise ValueError("profile must be 0..127, level 0..63")
    word = (profile << 9) | (level << 3) | (rpu << 2) | (el << 1) | bl
    payload = (
        bytes([1, 0])  # dv_version_major = 1, minor = 0
        + struct.pack(">H", word)
        + bytes([(bl_sig & 0xF) << 4])
        + b"\x00" * 19
    )
    assert len(payload) == 24, len(payload)
    return struct.pack(">I", 8 + len(payload)) + b"dvcC" + payload


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="insert a dvcC box into an HEVC sample entry")
    ap.add_argument("source")
    ap.add_argument("target")
    ap.add_argument("--profile", type=int, required=True)
    ap.add_argument("--level", type=int, default=1)
    ap.add_argument("--rpu", type=int, default=None, help="默认按 profile：8→1，5→1")
    ap.add_argument("--el", type=int, default=0)
    ap.add_argument("--bl", type=int, default=None, help="默认按 profile：8→1，5→0")
    ap.add_argument("--bl-sig", type=int, default=None, help="默认按 profile：8→1，5→0")
    args = ap.parse_args(argv)

    rpu = args.rpu if args.rpu is not None else 1
    bl = args.bl if args.bl is not None else (0 if args.profile == 5 else 1)
    bl_sig = args.bl_sig if args.bl_sig is not None else (0 if args.profile == 5 else 1)

    path = pathlib.Path(args.source)
    if not path.is_file():
        print(f"missing input: {path}", file=sys.stderr)
        return 2
    data = path.read_bytes()

    tops = top_level_boxes(data)
    moov = next(((o, s) for o, s, t in tops if t == b"moov"), None)
    if moov is None:
        print("no moov box found", file=sys.stderr)
        return 2
    moov_off, moov_size = moov
    mdat = next(((o, s) for o, s, t in tops if t == b"mdat"), None)
    if mdat is None or mdat[0] > moov_off:
        print(
            "layout unsafe: mdat must come BEFORE moov "
            "(inserting would shift mdat and invalidate stco offsets); "
            "remux first: ffmpeg -i in.mp4 -c copy out.mp4",
            file=sys.stderr,
        )
        return 2
    if b"dvcC" in data[moov_off : moov_off + moov_size]:
        print("moov already contains dvcC; refusing to add a second one", file=sys.stderr)
        return 2

    hit = find_video_sample_entry(data, moov_off, moov_size, [moov_off])
    if hit is None:
        print("no HEVC sample entry (hvc1/hev1) found in moov", file=sys.stderr)
        return 2
    chain, entry_off, entry_size, entry_type = hit
    dvcc = make_dvcc(args.profile, args.level, rpu, args.el, bl, bl_sig)

    # 插入点：采样条目固定体之后、既有子盒（hvcC 等）之后 = 条目末尾。
    insert_at = entry_off + entry_size
    # 条目末尾也应是某个祖先盒的边界路径上的点——按构造它就是条目自身的终点。
    out = bytearray()
    out += data[:insert_at]
    out += dvcc
    out += data[insert_at:]

    # 补大小：采样条目 + 全部祖先（moov…stsd）。
    for ancestor_off in chain:
        old = struct.unpack(">I", out[ancestor_off : ancestor_off + 4])[0]
        struct.pack_into(">I", out, ancestor_off, old + len(dvcc))
    struct.pack_into(">I", out, entry_off, entry_size + len(dvcc))

    pathlib.Path(args.target).write_bytes(bytes(out))
    print(
        f"inserted dvcC ({len(dvcc)} bytes) into {entry_type.decode()} sample entry at {insert_at}; "
        f"profile={args.profile} level={args.level} rpu={rpu} el={args.el} bl={bl} bl_sig={bl_sig}; "
        f"patched sizes: sample entry + {chain} ancestors; mdat stays before moov so stco unchanged"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
