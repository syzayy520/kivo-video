from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
MANIFEST = ROOT / "backend" / "tools" / "video" / "playback_graph" / "contract_manifest.yaml"
OUT = ROOT / "artifacts" / "p7" / "tooling" / "p7_header_diff_report.json"


def manifest_public_headers() -> list[str]:
    headers: list[str] = []
    in_public = False
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        if line.strip() == "public_headers:":
            in_public = True
            continue
        if in_public and line and not line.startswith("  - "):
            break
        if in_public and line.startswith("  - "):
            headers.append(line.split("- ", 1)[1].strip())
    return headers


def main() -> int:
    expected = sorted(manifest_public_headers())
    actual = sorted(
        str(path.relative_to(ROOT)).replace("\\", "/")
        for path in (ROOT / "include" / "kivo" / "video" / "playback_graph").glob("*.hpp")
    )
    report = {
        "expected_count": len(expected),
        "actual_count": len(actual),
        "missing": sorted(set(expected) - set(actual)),
        "extra": sorted(set(actual) - set(expected)),
        "private_header_count": len(list((ROOT / "backend" / "include_private" / "video" / "playback_graph").rglob("*.hpp"))),
        "fake_test_header_count": len(list((ROOT / "backend" / "tests" / "video" / "playback_graph").rglob("*.hpp"))),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    return 0 if not report["missing"] and not report["extra"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
