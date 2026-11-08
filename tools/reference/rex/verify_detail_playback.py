"""Read-only verification of saved K40 playback-entry evidence."""
import json
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
FOLDER = ROOT / "docs/evidence/p240/detail-playback"


def verify():
    checks = []

    def check(name, value):
        checks.append({"check": name, "pass": bool(value)})

    def nodes(name):
        return [(n.get("text", ""), n.get("content-desc", ""),
                 n.get("bounds"), n.get("clickable"))
                for n in ET.fromstring((FOLDER / "android" / (name + ".xml")).read_bytes()).iter("node")
                if n.get("bounds") != "[0,0][0,0]"
                and (n.get("text") or n.get("content-desc"))]

    def stack(name):
        return json.loads((FOLDER / "android" / (name + "-stack.json")).read_text(encoding="utf-8"))

    check("old button opened intermediate detail", any("/.DetailActivity}" in s or
          "/.ui.detail.DetailActivity}" in s for s in stack("legacy-intermediate")))
    base = stack("release-before")
    playing = stack("release-player")
    check("new button added one player", len(playing) == len(base) + 1 and
          sum("PlayerComposeActivity}" in s for s in playing) == 1)
    check("new button retained parent stack", playing[1:] == base)
    check("no old intermediate", not any("/.DetailActivity}" in s or
          "/.ui.detail.DetailActivity}" in s for s in playing))
    check("test resource identified", any("playing 生化危机.爆发夜.2026.1080p.mp4" == n[0]
          for n in nodes("release-player")))
    for name in ("release-return", "release-return-second"):
        check(name + " restored nodes", nodes(name) == nodes("release-before"))
        check(name + " restored stack", stack(name) == base)
    for suffix in ("before", "after"):
        check("no-resource reason " + suffix, any(n[0] == "未加载到相关资源"
              for n in nodes("release-no-resource-" + suffix)))
    check("no-resource unchanged nodes", nodes("release-no-resource-before") == nodes("release-no-resource-after"))
    check("no-resource unchanged stack", stack("release-no-resource-before") == stack("release-no-resource-after"))
    check("trailer idle nodes unchanged", nodes("trailer-ready") == nodes("trailer-idle"))
    check("trailer idle stack unchanged", stack("trailer-ready") == stack("trailer-idle"))
    check("external YouTube URL visible", any(n[0] == "www.youtube.com" for n in nodes("trailer-external")))
    check("external return restored nodes", nodes("trailer-single-before") == nodes("trailer-final-return"))
    check("external return restored stack", stack("trailer-single-before") == stack("trailer-final-return"))
    operations = [json.loads(s) for s in (FOLDER / "operations.jsonl").read_text(encoding="utf-8").splitlines()]
    check("four play taps recorded", any(o.get("tap") == "▶  播放" and o.get("count") == 4 for o in operations))
    return {"scope": "saved UI nodes and stacks; no performance, network-request or video-success claim",
            "checks": checks, "pass": all(c["pass"] for c in checks)}


if __name__ == "__main__":
    result = verify()
    print(json.dumps(result, ensure_ascii=True, indent=2))
    raise SystemExit(0 if result["pass"] else 1)
