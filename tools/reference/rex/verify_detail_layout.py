"""Read-only checks of saved K40 layout evidence, independent of capture helpers."""
import json
import re
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
FOLDER = ROOT / "docs/evidence/p240/detail-layout"


def verify():
    checks = []

    def check(name, value):
        checks.append({"check": name, "pass": bool(value)})

    def nodes(name):
        return [n.attrib for n in ET.fromstring(
            (FOLDER / "android" / (name + ".xml")).read_bytes()).iter("node")
            if n.get("bounds") != "[0,0][0,0]"
            and (n.get("text") or n.get("content-desc"))]

    def bounds(n):
        return tuple(map(int, re.findall(r"\d+", n["bounds"])))

    def text(name, label):
        return next(n for n in nodes(name) if n.get("text") == label)

    def content(name):
        # The decorative edge-to-edge backdrop is now exposed above the list viewport.
        return [(n.get("text", ""), n.get("content-desc", ""), n["bounds"], n["clickable"])
                for n in nodes(name) if bounds(n) != (0, 0, 1080, 80)]

    check("default first-screen content coordinates unchanged",
          content("before-default-top") == content("after-default-top"))
    check("restoring font scale restores default first screen",
          content("after-restored-top") == content("after-default-top"))
    insets = (FOLDER / "system-insets.txt").read_text(encoding="utf-8")
    check("device reports 80px visible status bar", bool(re.search(
        r"ITYPE_STATUS_BAR frame=\[0,0\]\[1080,80\] visible=true", insets)))
    before = [bounds(n)[1] for n in nodes("before-default-scroll") if n.get("text")]
    after = [bounds(n)[1] for n in nodes("after-default-scroll") if n.get("text")]
    check("old deep-scroll text invaded status bar", any(y < 80 for y in before))
    check("new deep-scroll text starts at or below status bar", min(after) == 80)
    for label in ("分级", "内容提示", "美国 R · 英国 15", "不当用语、惊吓"):
        old = bounds(text("before-default-scroll", label))
        new = bounds(text("after-default-scroll", label))
        check("default guide text dimensions preserved: " + label,
              (old[2]-old[0], old[3]-old[1]) == (new[2]-new[0], new[3]-new[1]))
    for label in ("美国 R · 英国 15", "不当用语、惊吓"):
        old = bounds(text("before-large-genres-full", label))
        new = bounds(text("after-large-guide-genres", label))
        check("1.5x guide value gains full line height: " + label,
              old[3]-old[1] == 48 and new[3]-new[1] == 73)
    genres = [bounds(text("after-stress-genres-wrap", label))
              for label in ("恐怖", "科幻", "冒险")]
    check("3x stress moves third genre onto another row",
          genres[0][1] == genres[1][1] and genres[2][1] > genres[0][3])
    check("3x stress genre labels remain within viewport",
          all(0 <= x1 < x2 <= 1080 and 80 <= y1 < y2 <= 2276
              for x1, y1, x2, y2 in genres))
    operations = [json.loads(line) for line in
                  (FOLDER / "operations.jsonl").read_text(encoding="utf-8").splitlines()]
    for scale in ("1.5", "3.0"):
        start = next(i for i, o in enumerate(operations) if o.get("font_scale_test") == scale)
        restored = next(o for o in operations[start+1:] if "font_scale_restored" in o)
        check("font scale restored after " + scale,
              restored["font_scale_restored"] == operations[start]["font_scale_original"] == "1.0")
    return {"scope": "saved UI geometry; 3x stress proves genre wrapping only; no full-page accessibility or pixel claim",
            "checks": checks, "pass": all(c["pass"] for c in checks)}


if __name__ == "__main__":
    result = verify()
    print(json.dumps(result, ensure_ascii=True, indent=2))
    raise SystemExit(0 if result["pass"] else 1)
