"""Recompute R2 saved-device assertions. Read-only; never connects to adb."""
import argparse
from collections import Counter
import json
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]


def verify(folder):
    captures = folder / "android"
    checks = []

    def check(name, condition):
        checks.append({"check": name, "pass": bool(condition)})

    def nodes(name):
        return [(n.get("text", ""), n.get("content-desc", ""),
                 n.get("bounds"), n.get("clickable"))
                for n in ET.fromstring((captures / f"{name}.xml").read_bytes()).iter("node")
                if (n.get("text") or n.get("content-desc"))
                and n.get("bounds") != "[0,0][0,0]"]

    def stack(name):
        return json.loads((captures / f"{name}-stack.json").read_text(encoding="utf-8"))

    def same(name, before, after):
        check(name, nodes(before) == nodes(after))

    def navigation(name, parent, child, returned):
        same(name + ": parent nodes restored", parent, returned)
        check(name + ": exactly one child", len(stack(child)) == len(stack(parent)) + 1)
        check(name + ": parent stack restored", stack(parent) == stack(returned))

    same("overview collapsed geometry", "release-overview-collapsed", "release-overview-recollapsed")
    check("overview expanded action", any(n[0] == "收起简介" for n in nodes("release-overview-expanded")))
    navigation("movie", "release-first-load-parent", "release-first-load-recovered",
               "release-first-load-parent-return")
    check("first load retry visible", any(n[0] == "重试加载" for n in nodes("release-first-load-failed")))
    check("first load recovered correct movie", any("地心记" in n[:2]
          for n in nodes("release-first-load-recovered")))
    navigation("TV", "release-tv-parent", "release-tv-child", "release-tv-parent-return")
    check("TV child title", any("干杯酒吧" in n[:2] for n in nodes("release-tv-child")))
    check("TV child episode metadata", any("集" in n[0] for n in nodes("release-tv-child")))
    same("horizontal movie return", "movie-horizontal-before", "movie-horizontal-return")
    original = Counter(nodes("release-injected-refresh-before"))
    failed = Counter(nodes("release-injected-refresh-failed"))
    check("refresh retained every original node", not (original - failed))
    check("refresh retry offered", any(n[0] == "重试" for n in failed))
    same("refresh retry geometry", "release-injected-refresh-before", "release-injected-refresh-retried")
    same("dismiss restored geometry", "release-dismiss-before", "release-dismiss-after")

    operations = [json.loads(line) for line in (folder / "operations.jsonl").read_text(
        encoding="utf-8").splitlines()]
    injections = [o["network_injection"] for o in operations if "network_injection" in o]
    check("three failure injections restored in order", injections == [
        "unavailable_app_proxy", "restored_exact_proxy_bytes"] * 3)
    for field in ("release_first_load_failure", "release_tv_similar_navigation"):
        check(field + ": four taps recorded", any(o.get(field) == "PASS"
              and o.get("rapid_taps") == 4 and o.get("added_activity_count") == 1
              for o in operations))
    return {"scope": "saved XML geometry, stack and operation assertions; not pixel/motion/network timing",
            "checks": checks, "pass": all(c["pass"] for c in checks)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path,
                        default=ROOT / "docs/evidence/p240/detail-regression")
    args = parser.parse_args()
    result = verify(args.evidence)
    print(json.dumps(result, ensure_ascii=True, indent=2))
    raise SystemExit(0 if result["pass"] else 1)


if __name__ == "__main__":
    main()
