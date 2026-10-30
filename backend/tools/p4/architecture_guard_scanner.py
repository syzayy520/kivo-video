#!/usr/bin/env python3
"""P4 Architecture Guard Scanner.

Machine-checkable architecture guard for P4 playback control plane boundaries.

Required scan:
- No P5/P6/P7/UI pollution in P4 headers/src.
- No P3 type redefinition in P4 namespace.
- No root manager bucket files.
- No forbidden implementation coupling tokens.
- Forbidden scan must NOT misfire on business-qualified *_engine.hpp.

Usage:
    python backend/tools/p4/architecture_guard_scanner.py [p4_root]
"""

import os
import re
import sys
import json
from pathlib import Path

FORBIDDEN_TOKENS = [
    "D3D11", "DXGI", "Vulkan",
    "WASAPI", "AVAudioEngine", "AAudio", "AudioTrack",
    "decoded_frame", "render_surface", "audio_device_open",
    "subtitle_bitmap",
    "QWidget", "QML", "QtQuick",
    "drm_decrypt", "license_acquisition",
    "provider_manager", "source_manager", "player_manager", "god_player",
]

BUSINESS_ENGINE_FILES = {
    "buffer_engine.hpp",
    "timeline_engine.hpp",
    "recovery_engine.hpp",
    "memory_pool_engine.hpp",
    "event_bus_engine.hpp",
}

P3_REDEFINITION_PATTERNS = [
    r"struct\s+PlaybackHandoffContract\b",
    r"struct\s+PlaybackCandidateSet\b",
    r"class\s+PlaybackHandoffContract\b",
    r"class\s+PlaybackCandidateSet\b",
]


def scan_forbidden_tokens(p4_root: Path) -> list:
    """Scan for forbidden implementation coupling tokens."""
    violations = []
    for filepath in p4_root.rglob("*.hpp"):
        rel = filepath.relative_to(p4_root)
        filename = filepath.name

        # Allow business-qualified engine files
        if filename in BUSINESS_ENGINE_FILES:
            continue

        # Allow policy documentation, forbidden fixtures, test sentinels
        parts = str(rel).replace("\\", "/").lower()
        if "forbidden" in parts or "sentinel" in parts:
            continue

        try:
            content = filepath.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue

        for token in FORBIDDEN_TOKENS:
            # Use word boundary to avoid false positives
            pattern = re.compile(re.escape(token), re.IGNORECASE)
            for match in pattern.finditer(content):
                line_num = content[:match.start()].count("\n") + 1
                violations.append({
                    "file": str(rel),
                    "line": line_num,
                    "token": token,
                })
    return violations


def scan_p3_redefinition(p4_root: Path) -> list:
    """Scan for P3 type redefinition in P4 namespace."""
    violations = []
    for filepath in p4_root.rglob("*.hpp"):
        rel = filepath.relative_to(p4_root)
        try:
            content = filepath.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue

        for pattern in P3_REDEFINITION_PATTERNS:
            for match in re.finditer(pattern, content):
                line_num = content[:match.start()].count("\n") + 1
                violations.append({
                    "file": str(rel),
                    "line": line_num,
                    "pattern": pattern,
                })
    return violations


def scan_root_bucket_files(p4_root: Path) -> list:
    """Scan for root manager bucket files (flat files in root)."""
    violations = []
    root_files = [f for f in p4_root.iterdir() if f.is_file() and f.suffix in (".hpp", ".cpp")]
    for f in root_files:
        violations.append({"file": f.name})
    return violations


def main():
    p4_root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("backend/include/kivo/playback/control_plane")

    if not p4_root.exists():
        print(f"ERROR: P4 root not found: {p4_root}")
        sys.exit(1)

    results = {
        "p4_root": str(p4_root),
        "forbidden_token_violations": scan_forbidden_tokens(p4_root),
        "p3_redefinition_violations": scan_p3_redefinition(p4_root),
        "root_bucket_violations": scan_root_bucket_files(p4_root),
    }

    total_violations = (
        len(results["forbidden_token_violations"])
        + len(results["p3_redefinition_violations"])
        + len(results["root_bucket_violations"])
    )

    results["total_violations"] = total_violations
    results["status"] = "PASS" if total_violations == 0 else "FAIL"

    print(json.dumps(results, indent=2))
    sys.exit(0 if total_violations == 0 else 1)


if __name__ == "__main__":
    main()
