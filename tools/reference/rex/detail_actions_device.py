"""R2 device evidence helper. Use visible XML labels/bounds, never clears application data."""
import argparse
import json
import re
import subprocess
import time
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "docs/evidence/p240/detail-actions"
ADB = ["C:/Android/sdk/platform-tools/adb.exe", "-s", "b6e950ee"]


def adb(*args):
    return subprocess.check_output(ADB + list(args), stderr=subprocess.PIPE)


def xml():
    adb("shell", "uiautomator", "dump", "/sdcard/kivo-r2-window.xml")
    return adb("exec-out", "cat", "/sdcard/kivo-r2-window.xml")


def nodes(raw):
    return [n.attrib for n in ET.fromstring(raw).iter("node")
            if n.get("text") or n.get("content-desc")]


def record(entry):
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / "operations.jsonl").open("a", encoding="utf-8") as f:
        f.write(json.dumps({"at_epoch_ms": round(time.time() * 1000), **entry},
                           ensure_ascii=False) + "\n")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("action", choices=["capture", "nodes", "tap", "swipe", "key", "marks"])
    p.add_argument("values", nargs="*")
    a = p.parse_args()
    if a.action in ("capture", "nodes"):
        raw = xml()
        if a.action == "capture":
            name = a.values[0]
            assert re.fullmatch(r"[a-z0-9-]+", name)
            folder = OUT / "android"
            folder.mkdir(parents=True, exist_ok=True)
            (folder / (name + ".png")).write_bytes(adb("exec-out", "screencap", "-p"))
            (folder / (name + ".xml")).write_bytes(raw)
            activities = adb("shell", "dumpsys", "activity", "activities").decode("utf-8", "replace")
            relevant = [s.strip() for s in activities.splitlines()
                        if "ActivityRecord{" in s and "com.kivo.cinema/" in s]
            (folder / (name + "-stack.json")).write_text(json.dumps(relevant, indent=2), encoding="utf-8")
            record({"capture": name})
        print(json.dumps(nodes(raw), ensure_ascii=True, indent=2))
    elif a.action == "tap":
        label = a.values[0]
        rows = nodes(xml())
        matches = [n for n in rows if label in (n.get("text"), n.get("content-desc"))]
        matches = [n for n in matches if n["bounds"] != "[0,0][0,0]"]
        text_matches = [n for n in matches if n.get("text") == label]
        if len(text_matches) == 1:
            matches = text_matches
        assert len(matches) == 1, matches
        row = matches[0]
        x1, y1, x2, y2 = map(int, re.findall(r"\d+", row["bounds"]))
        assert x2 > x1 and y2 > y1, row
        adb("shell", "input", "tap", str((x1+x2)//2), str((y1+y2)//2))
        record({"tap": label, "node": row})
    elif a.action == "swipe":
        assert len(a.values) == 5
        adb("shell", "input", "swipe", *a.values)
        record({"swipe": a.values})
    elif a.action == "key":
        adb("shell", "input", "keyevent", a.values[0])
        record({"key": a.values[0]})
    elif a.action == "marks":
        raw = adb("exec-out", "run-as", "com.kivo.cinema", "cat", "files/media/marks/titles.json")
        value = json.loads(raw)
        record({"persisted_marks": value})
        print(json.dumps(value, ensure_ascii=True))


if __name__ == "__main__":
    main()
