from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
MANIFEST = ROOT / "docs" / "design" / "p8" / "KIVO-VIDEO-P8-SUBTITLE-PLANE-DESIGN-LOCK-001-V1.5.5.rule_manifest.yaml"
MATRIX = ROOT / "docs" / "roadmap" / "p8_exit_criteria_s1_s30_matrix.md"


def main() -> int:
    manifest_text = MANIFEST.read_text(encoding="utf-8", errors="ignore")
    matrix_text = MATRIX.read_text(encoding="utf-8", errors="ignore")
    missing: list[str] = []
    for index in range(1, 31):
        marker = f"S{index}"
        if marker not in manifest_text:
            missing.append(f"manifest:{marker}")
        if marker not in matrix_text:
            missing.append(f"matrix:{marker}")
    for marker in [
        "C-001", "C-002", "C-003", "C-004", "C-005",
        "G-001", "G-015", "A-001", "A-005", "I-001", "I-003",
    ]:
        if marker not in manifest_text:
            missing.append(f"manifest:{marker}")
    report = {
        "tool": "p8_manifest_check",
        "status": "PASS" if not missing else "FAIL",
        "manifest": str(MANIFEST.relative_to(ROOT)),
        "matrix": str(MATRIX.relative_to(ROOT)),
        "criteria_checked": 30,
        "missing": missing,
    }
    print(json.dumps(report, indent=2))
    return 0 if not missing else 1


if __name__ == "__main__":
    raise SystemExit(main())
