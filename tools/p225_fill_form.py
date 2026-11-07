#!/usr/bin/env python3
"""225 真机驱动：SmbActivity 表单动态定位填写 + 测试连通性 + 断言浏览态。

坐标不写死：每步 uiautomator dump 取标签行当前 bounds，点其右侧值区。
用法：python -X utf8 -I tools/p225_fill_form.py [--tap-connect]
退出码 0=浏览态就绪；1=断言失败。
"""
import re
import subprocess
import sys
import time

ADB = r"C:/Android/sdk/platform-tools/adb.exe"
SERIAL = "b6e950ee"

FIELDS = [
    # (标签, 期望值, 清空长度, 是否密码)
    ("主机", "192.168.5.142", 30, False),
    ("共享", "kivo", 10, False),
    ("用户", "kivosmb", 10, False),
    ("密码", "kivo2026!", 10, True),
]


def sh(*args, timeout=30):
    cmd = [ADB, "-s", SERIAL, "shell"] + list(args)
    return subprocess.run(cmd, capture_output=True, timeout=timeout).stdout.decode(
        "utf-8", "replace")


def dump():
    """uiautomator dump 到设备文件再 cat（/dev/tty 在 shell 通道下拿不到 xml）。"""
    sh("uiautomator", "dump", "/sdcard/p225_dump.xml", timeout=45)
    out = sh("cat", "/sdcard/p225_dump.xml", timeout=15)
    xml_start = out.find("<?xml")
    if xml_start < 0:
        return []
    xml = out[xml_start:]
    rows = []
    for m in re.finditer(
            r'text="([^"]*)"[^>]*?bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"', xml):
        t = m.group(1)
        x1, y1, x2, y2 = (int(m.group(i)) for i in range(2, 6))
        rows.append((t, (x1 + x2) // 2, (y1 + y2) // 2))
    return rows


def tap(x, y):
    sh("input", "tap", str(x), str(y))


def text(s):
    sh("input", "text", s)


def keys(*codes):
    sh("input", "keyevent", *[str(c) for c in codes])


MOVE_END, DEL = 123, 67


def main():
    tap_connect = "--tap-connect" in sys.argv
    for label, value, clear_len, _is_pw in FIELDS:
        rows = dump()
        hit = next((r for r in rows if r[0] == label), None)
        if hit is None:
            print(f"FAIL: label {label} not found; visible="
                  f"{[r[0] for r in rows if r[0]][:8]}")
            return 1
        _, x, y = hit
        tap(x + 380, y)   # 值区（标签右侧；整行 TextField 占宽）
        time.sleep(1.2)
        if clear_len:
            keys(MOVE_END, *[DEL] * clear_len)
            time.sleep(0.4)
        text(value)
        time.sleep(0.6)
        print(f"filled {label}")

    # 校验（密码框是 PasswordVisualTransformation：dump 里是圆点串，接受等长圆点）
    rows = dump()
    ok = True
    for label, value, _cl, is_pw in FIELDS:
        if is_pw:
            got = next((r[0] for r in rows
                        if r[0].startswith("•") and len(r[0]) == len(value)), None)
            print(f"check {label}: obscured {'OK' if got else 'NOT FOUND'}")
            ok = ok and got is not None
        else:
            got = next((r[0] for r in rows if r[0].startswith(value[:6])), None)
            print(f"check {label}: want={value!r} got={got!r}")
            ok = ok and got is not None and got.startswith(value[:6])
    if not ok:
        return 1

    if tap_connect:
        rows = dump()
        btn = next((r for r in rows if r[0] == "测试连通性"), None)
        if btn is None:
            print("FAIL: 测试连通性 button not found")
            return 1
        tap(*btn[1:])
        time.sleep(6)
        rows = dump()
        texts = [r[0] for r in rows if r[0]]
        print("after connect:", texts[:14])
        # 浏览态断言：出现"上一级"或"可连接"或目录列表
        joined = " ".join(texts)
        if "上一级" in joined or "可连接" in joined:
            print("BROWSE OK")
            return 0
        print("NOT BROWSE YET")
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
