from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]

PUBLIC_ROOT = ROOT / "include" / "kivo" / "video" / "subtitle_plane"
ALLOWED_PUBLIC_FAMILIES = {
    "api",
    "cancel",
    "command",
    "config",
    "error",
    "evidence",
    "format",
    "foundation",
    "frame",
    "generation",
    "input",
    "metrics",
    "observer",
    "privacy",
    "request",
    "resource",
    "snapshot",
    "source",
    "state",
    "stream",
    "types",
}
FORBIDDEN_FAMILY_NAMES = {"common", "misc", "utils", "helpers", "shared"}
PUBLIC_HEADER_LINE_LIMIT = 220
PRODUCT_SOURCE_LINE_LIMIT = 320


def line_count(path: Path) -> int:
    return len(path.read_text(encoding="utf-8", errors="ignore").splitlines())


def main() -> int:
    violations: list[dict[str, object]] = []
    files: list[dict[str, object]] = []

    if PUBLIC_ROOT.exists():
        for child in PUBLIC_ROOT.iterdir():
            if child.is_file() and child.name != "kivo_subtitle_plane.h":
                violations.append({
                    "path": str(child.relative_to(ROOT)),
                    "reason": "public_root_flat_file",
                })
            if child.is_dir() and child.name not in ALLOWED_PUBLIC_FAMILIES:
                violations.append({
                    "path": str(child.relative_to(ROOT)),
                    "reason": "unknown_public_family",
                })
            if child.name in FORBIDDEN_FAMILY_NAMES:
                violations.append({
                    "path": str(child.relative_to(ROOT)),
                    "reason": "forbidden_dumping_ground_family",
                })

        for path in PUBLIC_ROOT.rglob("*"):
            if path.is_dir() and path.name in FORBIDDEN_FAMILY_NAMES:
                violations.append({
                    "path": str(path.relative_to(ROOT)),
                    "reason": "forbidden_dumping_ground_family",
                })
            if path.is_file():
                count = line_count(path)
                files.append({"path": str(path.relative_to(ROOT)), "lines": count})
                if path.suffix == ".h" and count > PUBLIC_HEADER_LINE_LIMIT:
                    violations.append({
                        "path": str(path.relative_to(ROOT)),
                        "reason": "public_header_line_limit",
                        "lines": count,
                    })

    for root in [
        ROOT / "backend" / "src" / "video" / "subtitle_plane",
        ROOT / "backend" / "include_private" / "video" / "subtitle_plane",
    ]:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if path.is_dir() and path.name in FORBIDDEN_FAMILY_NAMES:
                violations.append({
                    "path": str(path.relative_to(ROOT)),
                    "reason": "forbidden_dumping_ground_family",
                })
            if path.is_file():
                count = line_count(path)
                files.append({"path": str(path.relative_to(ROOT)), "lines": count})
                if path.suffix in {".cpp", ".hpp"} and count > PRODUCT_SOURCE_LINE_LIMIT:
                    violations.append({
                        "path": str(path.relative_to(ROOT)),
                        "reason": "product_source_line_limit",
                        "lines": count,
                    })

    report = {
        "tool": "p8_natural_tree_guard",
        "status": "PASS" if not violations else "FAIL",
        "public_root": str(PUBLIC_ROOT.relative_to(ROOT)),
        "file_count": len(files),
        "max_lines": max((item["lines"] for item in files), default=0),
        "files": sorted(files, key=lambda item: item["path"]),
        "violations": violations,
    }
    print(json.dumps(report, indent=2))
    return 0 if not violations else 1


if __name__ == "__main__":
    raise SystemExit(main())
