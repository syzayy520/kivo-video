from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
OUT = ROOT / "artifacts" / "p7" / "tooling" / "p7_arch_guard_report.json"
PUBLIC = ROOT / "include" / "kivo" / "video" / "playback_graph"
PRIVATE = ROOT / "backend" / "include_private" / "video" / "playback_graph"
PUBLIC_FORBIDDEN = [
    "Qt",
    "QString",
    "QObject",
    "avcodec",
    "avformat",
    "d3d11",
    "dxgi",
    "audioclient",
    "mmdeviceapi",
    "backend/include_private",
    "std::future",
    "condition_variable",
    "UnsupportedPlaybackRate",
]


def scan(root: Path, tokens: list[str]) -> list[dict[str, object]]:
    hits: list[dict[str, object]] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in {".hpp", ".cpp"}:
            continue
        text = path.read_text(encoding="utf-8")
        for line_no, line in enumerate(text.splitlines(), start=1):
            for token in tokens:
                if token in line:
                    hits.append({
                        "path": str(path.relative_to(ROOT)).replace("\\", "/"),
                        "line": line_no,
                        "token": token,
                    })
    return hits


def main() -> int:
    public_hits = scan(PUBLIC, PUBLIC_FORBIDDEN)
    private_bad_names = [
        str(path.relative_to(ROOT)).replace("\\", "/")
        for path in PRIVATE.rglob("*")
        if path.is_dir() and path.name in {"common", "misc", "utils", "helpers", "manager", "all_in_one"}
    ]
    report = {
        "public_header_purity": not public_hits,
        "public_hits": public_hits,
        "private_bad_family_names": private_bad_names,
        "public_exception_count": 0,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    return 0 if report["public_header_purity"] and not private_bad_names else 1


if __name__ == "__main__":
    raise SystemExit(main())
