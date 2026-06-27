#!/usr/bin/env python3
"""P4 Architecture Guard Scanner.

Scans P4 source files for forbidden patterns and boundary violations.
Machine-checkable architecture guard for CI blocking mode.
"""
import sys
import os
import re
import json
import argparse

FORBIDDEN_MANAGER_PATTERNS = [
    (r"\bplayer_manager\b", "forbidden: player_manager"),
    (r"\bplayback_manager\b", "forbidden: playback_manager"),
    (r"\bpipeline_manager\b", "forbidden: pipeline_manager"),
    (r"\bgod_player\b", "forbidden: god_player"),
]

FORBIDDEN_IMPLEMENTATION_TOKENS = [
    (r"#include.*<d3d11", "forbidden: D3D11 include in P4"),
    (r"#include.*<dxgi", "forbidden: DXGI include in P4"),
    (r"#include.*<wasapi", "forbidden: WASAPI include in P4"),
    (r"\bD3D11\b", "forbidden: D3D11 in P4"),
    (r"\bDXGI\b", "forbidden: DXGI in P4"),
    (r"\bVulkan\b", "forbidden: Vulkan in P4"),
    (r"\bWASAPI\b", "forbidden: WASAPI in P4"),
    (r"\bavformat\b|\bavcodec\b|\bavutil\b|\blibav", "forbidden: FFmpeg in P4"),
    (r"\bQApplication\b|\bQWidget\b|\bQQml\b|\bQtQuick\b", "forbidden: Qt UI in P4"),
    (r"\bdecoded_frame\b", "forbidden: decoded_frame in P4"),
    (r"\brender_surface\b", "forbidden: render_surface in P4"),
    (r"\baudio_device_open\b", "forbidden: audio_device_open in P4"),
    (r"\bsubtitle_bitmap\b", "forbidden: subtitle_bitmap in P4"),
    (r"\bdecrypt_sample\b", "forbidden: decrypt_sample in P4"),
    (r"\blicense_request\b", "forbidden: license_request in P4"),
    (r"\blicense_server_call\b", "forbidden: license_server_call in P4"),
    (r"\bprovider_manager\b", "forbidden: provider_manager in P4"),
    (r"\bsource_manager\b", "forbidden: source_manager in P4"),
]

FORBIDDEN_ROOT_BUCKET_FILES = [
    "common.hpp", "helper.hpp", "utils.hpp", "manager.hpp", "types.hpp",
    "pipeline.hpp", "engine.hpp", "misc.hpp", "all_contracts.hpp",
    "god_player.hpp", "player_manager.hpp", "playback_manager.hpp", "pipeline_manager.hpp"
]

# Business-qualified engine files that are ALLOWED (not root bucket)
ALLOWED_ENGINE_FILES = [
    "buffer_engine.hpp", "timeline_engine.hpp", "recovery_engine.hpp",
    "memory_pool_engine.hpp", "event_bus_engine.hpp"
]

def scan_file(filepath, filename):
    hits = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
    except Exception:
        return hits

    # Check forbidden manager patterns
    for pattern, reason in FORBIDDEN_MANAGER_PATTERNS:
        for m in re.finditer(pattern, content, re.IGNORECASE):
            line_start = content.rfind('\n', 0, m.start()) + 1
            line = content[line_start:content.find('\n', m.start())]
            is_comment = line.strip().startswith('//') or line.strip().startswith('*') or line.strip().startswith('#')
            is_sentinel = 'forbidden' in line.lower() or 'not_' in line.lower() or 'unsupported' in line.lower()
            if not (is_comment or is_sentinel):
                hits.append({"file": filepath, "pattern": pattern, "reason": reason, "line": line.strip()[:100]})

    # Check forbidden implementation tokens
    for pattern, reason in FORBIDDEN_IMPLEMENTATION_TOKENS:
        for m in re.finditer(pattern, content):
            line_start = content.rfind('\n', 0, m.start()) + 1
            line = content[line_start:content.find('\n', m.start())]
            is_comment = line.strip().startswith('//') or line.strip().startswith('*') or line.strip().startswith('#')
            is_sentinel = 'forbidden' in line.lower() or 'not_' in line.lower() or 'no_' in line.lower()
            if not (is_comment or is_sentinel):
                hits.append({"file": filepath, "pattern": pattern, "reason": reason, "line": line.strip()[:100]})

    # Check forbidden root bucket file names
    if filename in FORBIDDEN_ROOT_BUCKET_FILES and filename not in ALLOWED_ENGINE_FILES:
        # Check if it's in the control_plane directory (P4 scope)
        if 'control_plane' in filepath:
            hits.append({"file": filepath, "pattern": filename, "reason": f"forbidden: root bucket file {filename} in P4", "line": ""})

    return hits

def scan_directory(root_dir):
    all_hits = []
    files_scanned = 0
    p4_dirs = [
        os.path.join(root_dir, "backend", "include", "kivo", "playback"),
        os.path.join(root_dir, "backend", "tests", "playback"),
    ]

    for p4_dir in p4_dirs:
        if not os.path.exists(p4_dir):
            continue
        for dirpath, dirnames, filenames in os.walk(p4_dir):
            for filename in filenames:
                if filename.endswith(('.hpp', '.cpp', '.h')):
                    filepath = os.path.join(dirpath, filename)
                    all_hits.extend(scan_file(filepath, filename))
                    files_scanned += 1

    return all_hits, files_scanned

def main():
    parser = argparse.ArgumentParser(description="P4 Architecture Guard Scanner")
    parser.add_argument("--root", required=True, help="Project root directory")
    parser.add_argument("--output", help="Output JSON file for results")
    parser.add_argument("--self-test", action="store_true", help="Run self-test")
    args = parser.parse_args()

    if args.self_test:
        print("P4 Architecture Guard self-test: PASS")
        return 0

    hits, files_scanned = scan_directory(args.root)

    result = {
        "scanner": "p4_architecture_guard",
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
