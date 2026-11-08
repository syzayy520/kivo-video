"""R2 navigation regression on K40. Records real UI and activity-stack evidence."""
import argparse
import json
import re
import time
import subprocess
from contextlib import contextmanager

import detail_actions_device as device

OUT = device.ROOT / "docs/evidence/p240/detail-regression"
device.OUT = OUT


@contextmanager
def unavailable_requests():
    """Inject an unavailable SOCKS endpoint; restore the exact private config bytes."""
    original = device.adb("exec-out", "run-as", "com.kivo.cinema", "cat", "files/tmdb_proxy.txt")
    def write(data):
        subprocess.run(device.ADB + ["shell", "run-as", "com.kivo.cinema", "sh", "-c",
                       "'cat > files/tmdb_proxy.txt'"], input=data, check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    try:
        write(b"127.0.0.1:1")
        assert device.adb("exec-out", "run-as", "com.kivo.cinema", "cat",
                          "files/tmdb_proxy.txt") == b"127.0.0.1:1"
        device.record({"network_injection": "unavailable_app_proxy", "radios_unchanged": True})
        yield
    finally:
        write(original)
        assert device.adb("exec-out", "run-as", "com.kivo.cinema", "cat",
                          "files/tmdb_proxy.txt") == original
        device.record({"network_injection": "restored_exact_proxy_bytes"})


@contextmanager
def offline():
    """Cut radios and the app's USB proxy; restore both paths even on failure."""
    def setting(key):
        return device.adb("shell", "settings", "get", "global", key).decode().strip()
    original = {k: setting(k) for k in ("airplane_mode_on", "wifi_on", "mobile_data")}
    reverse = device.adb("reverse", "--list").decode().splitlines()
    mappings = [line.split()[-2:] for line in reverse if len(line.split()) == 3]
    proxy = device.adb("shell", "run-as", "com.kivo.cinema", "cat",
                       "files/tmdb_proxy.txt").decode().strip()
    host, _, port = proxy.rpartition(":")
    proxy_remote = f"tcp:{port}" if host in ("localhost", "127.0.0.1") and port.isdigit() else None
    assert original["airplane_mode_on"] == "0", original
    try:
        if proxy_remote and any(remote == proxy_remote for remote, _ in mappings):
            device.adb("reverse", "--remove", proxy_remote)
        device.adb("shell", "cmd", "connectivity", "airplane-mode", "enable")
        device.adb("shell", "svc", "wifi", "disable")
        time.sleep(3)
        probe = device.adb("shell", "sh", "-c",
            "ping -c 1 -W 2 1.1.1.1 >/dev/null 2>&1; echo $?").decode().strip()
        remaining = device.adb("reverse", "--list").decode().splitlines()
        assert not proxy_remote or not any(
            line.split()[-2] == proxy_remote for line in remaining if len(line.split()) == 3)
        device.record({"network": "offline", "original": original, "ping_exit": probe,
                       "usb_proxy_disabled": bool(proxy_remote), "reverse_before": mappings})
        assert probe != "0", "Connectivity probe still succeeds"
        yield
    finally:
        device.adb("shell", "cmd", "connectivity", "airplane-mode", "disable")
        device.adb("shell", "svc", "wifi", "enable" if original["wifi_on"] == "1" else "disable")
        device.adb("shell", "svc", "data", "enable" if original["mobile_data"] == "1" else "disable")
        for remote, local in mappings:
            device.adb("reverse", remote, local)
        device.record({"network": "restored", "settings": {k: setting(k) for k in original},
                       "reverse_restored": mappings})


def rows():
    return [r for r in device.nodes(device.xml()) if r["bounds"] != "[0,0][0,0]"]


def find(label):
    matches = [r for r in rows() if label in (r.get("text"), r.get("content-desc"))]
    text = [r for r in matches if r.get("text") == label]
    if len(text) == 1:
        matches = text
    assert len(matches) == 1, matches
    return matches[0]


def center(row):
    x1, y1, x2, y2 = map(int, re.findall(r"\d+", row["bounds"]))
    assert x2 > x1 and y2 > y1, row
    return str((x1 + x2) // 2), str((y1 + y2) // 2)


def wait_for(label, timeout=35):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        current = rows()
        if any(label in (r.get("text"), r.get("content-desc")) for r in current):
            return current
    raise AssertionError(f"UI did not show expected label: {label!r}")


def stack():
    raw = device.adb("shell", "dumpsys", "activity", "activities").decode("utf-8", "replace")
    # Hist rows are unique stack entries; other dumpsys sections repeat them.
    return [s.strip() for s in raw.splitlines()
            if re.search(r"\* Hist\s+#\d+:", s) and "com.kivo.cinema/" in s]


def capture(name):
    assert re.fullmatch(r"[a-z0-9-]+", name)
    folder = OUT / "android"
    folder.mkdir(parents=True, exist_ok=True)
    raw = device.xml()
    (folder / (name + ".xml")).write_bytes(raw)
    (folder / (name + ".png")).write_bytes(device.adb("exec-out", "screencap", "-p"))
    (folder / (name + "-stack.json")).write_text(json.dumps(stack(), indent=2), encoding="utf-8")
    device.record({"capture": name})
    return device.nodes(raw)


def tap(label, count=1):
    row = find(label)
    x, y = center(row)
    # One on-device command avoids round-trip gaps between repeated taps.
    command = "; ".join([f"input tap {x} {y}"] * count)
    device.adb("shell", command)
    device.record({"tap": label, "count": count, "node": row})


def compact(items):
    return [{"text": r.get("text"), "desc": r.get("content-desc"),
             "bounds": r["bounds"], "clickable": r["clickable"]}
            for r in items if r["bounds"] != "[0,0][0,0]"]


def main():
    p = argparse.ArgumentParser()
    p.add_argument("action", choices=["capture", "nodes", "tap", "swipe", "back", "stack"])
    p.add_argument("values", nargs="*")
    a = p.parse_args()
    if a.action == "capture":
        print(json.dumps(compact(capture(a.values[0])), ensure_ascii=True))
    elif a.action == "nodes":
        print(json.dumps(compact(rows()), ensure_ascii=True))
    elif a.action == "tap":
        tap(a.values[0], int(a.values[1]) if len(a.values) > 1 else 1)
    elif a.action == "swipe":
        assert len(a.values) == 5
        device.adb("shell", "input", "swipe", *a.values)
        device.record({"swipe": a.values})
    elif a.action == "back":
        device.adb("shell", "input", "keyevent", "4")
        device.record({"back": True})
    else:
        print(json.dumps(stack()))


if __name__ == "__main__":
    main()
