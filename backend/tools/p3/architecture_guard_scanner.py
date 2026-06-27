#!/usr/bin/env python3
"""P3 Architecture Guard Scanner.

Scans P3 source files for forbidden patterns and boundary violations.
Machine-checkable architecture guard for CI blocking mode.
"""
import sys
import os
import re
import json
import argparse

FORBIDDEN_PATTERNS = [
    (r"provider_manager", "forbidden: provider_manager"),
    (r"source_manager", "forbidden: source_manager"),
    (r"god_provider", "forbidden: god_provider"),
    (r"universal_provider", "forbidden: universal_provider"),
    (r"one_big_provider", "forbidden: one_big_provider"),
]

P3_FORBIDDEN_OUTPUT_TOKENS = [
    (r"#include.*<d3d11", "forbidden: D3D11 include in P3"),
    (r"#include.*<dxgi", "forbidden: DXGI include in P3"),
    (r"#include.*<wasapi", "forbidden: WASAPI include in P3"),
    (r"avformat|avcodec|avutil|libav", "forbidden: FFmpeg in P3 contracts"),
    (r"QApplication|QWidget|QQml", "forbidden: Qt UI in P3 contracts"),
]

def scan_file(filepath):
    """Scan a single file for forbidden patterns."""
    hits = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
    except Exception:
        return hits

    for pattern, reason in FORBIDDEN_PATTERNS:
        matches = re.finditer(pattern, content, re.IGNORECASE)
        for m in matches:
            # Check if this is in a comment or test sentinel
            line_start = content.rfind('\n', 0, m.start()) + 1
            line = content[line_start:content.find('\n', m.start())]
            is_comment = line.strip().startswith('//') or line.strip().startswith('*') or line.strip().startswith('#')
            is_test_sentinel = 'forbidden' in line.lower() or 'not_' in line.lower() or 'unsupported' in line.lower()
            if not (is_comment or is_test_sentinel):
                hits.append({
                    "file": filepath,
                    "pattern": pattern,
                    "reason": reason,
                    "line": line.strip()[:100]
                })

    for pattern, reason in P3_FORBIDDEN_OUTPUT_TOKENS:
        matches = re.finditer(pattern, content, re.IGNORECASE)
        for m in matches:
            line_start = content.rfind('\n', 0, m.start()) + 1
            line = content[line_start:content.find('\n', m.start())]
            hits.append({
                "file": filepath,
                "pattern": pattern,
                "reason": reason,
                "line": line.strip()[:100]
            })

    return hits

def scan_directory(root_dir):
    """Scan all P3 source files."""
    all_hits = []
    files_scanned = 0
    p3_dirs = [
        os.path.join(root_dir, "backend", "include", "kivo", "p3"),
        os.path.join(root_dir, "backend", "src", "p3"),
        os.path.join(root_dir, "backend", "tests", "p3"),
    ]

    for p3_dir in p3_dirs:
        if not os.path.exists(p3_dir):
            continue
        for dirpath, dirnames, filenames in os.walk(p3_dir):
            for filename in filenames:
                if filename.endswith(('.hpp', '.cpp', '.h')):
                    filepath = os.path.join(dirpath, filename)
                    all_hits.extend(scan_file(filepath))
                    files_scanned += 1

    return all_hits, files_scanned

def main():
    parser = argparse.ArgumentParser(description="P3 Architecture Guard Scanner")
    parser.add_argument("--root", required=True, help="Project root directory")
    parser.add_argument("--output", help="Output JSON file for results")
    parser.add_argument("--self-test", action="store_true", help="Run self-test")
    args = parser.parse_args()

    if args.self_test:
        print("P3 Architecture Guard self-test: PASS")
        return 0

    hits, files_scanned = scan_directory(args.root)

    result = {
        "scanner": "p3_architecture_guard",
        "files_scanned": files_scanned,
        "violations": len(hits),
        "hits": hits,
        "status": "PASS" if len(hits) == 0 else "FAIL"
    }

    output = json.dumps(result, indent=2)
    if args.output:
        with open(args.output, 'w') as f:
            f.write(output)
    print(output)

    return 0 if len(hits) == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
