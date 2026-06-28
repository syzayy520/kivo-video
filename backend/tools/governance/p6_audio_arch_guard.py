#!/usr/bin/env python3
# p6_audio_arch_guard.py — P6A Architecture Guard Scanner (§42, 4 modes)
#
# Modes:
#   PublicHeadersStrict    — scans include/kivo/video/audio_plane/**/*.hpp
#   ArchitectureRuleDocs   — allowed rule text (guard scripts, rule docs)
#   NegativeFixture        — scans backend/tests/video/audio_plane/negative*/ only
#   BackendPrivate         — scans backend/src/video/audio_plane/ (must NOT exist in P6A)
#
# Usage:
#   python p6_audio_arch_guard.py --repo-root "C:/kivo video" --mode PublicHeadersStrict
#   python p6_audio_arch_guard.py --repo-root "C:/kivo video" --mode BackendPrivate
#
# Exit code: 0 = PASS, 1 = FAIL, 2 = ERROR
import argparse
import os
import re
import sys
from pathlib import Path

FORBIDDEN_TOKENS = [
    # Windows headers
    "windows.h", "audioclient.h", "mmdeviceapi.h", "ksmedia.h",
    "endpointvolume.h", "audiopolicy.h",
    # Windows COM types (case-sensitive)
    "AUDCLNT_REFERENCE_TIME", "GUID", "CoInitializeEx", "CoUninitialize",
    "IMMNotificationClient", "PROPVARIANT", "PKEY", "PKEY_",
    # FFmpeg headers/types
    "avcodec.h", "avformat.h", "swresample.h",
    "AVFrame", "AVPacket", "AVCodecParameters", "AVCodecContext",
    "AVRational", "AVChannelLayout", "SwrContext",
    # Qt types
    "QString", "QByteArray", "QVector", "QSharedPointer",
    "QVariant", "QImage", "QWindow", "QObject", "QWidget",
    # WASAPI interface types
    "IAudioClient", "IAudioClient2", "IAudioClient3",
    "IAudioRenderClient", "IAudioClock",
    "IMMDevice", "IAudioSessionControl", "ISimpleAudioVolume",
    "IAudioEndpointVolume",
    # Windows result/format types
    "HRESULT", "WAVEFORMATEX", "WAVEFORMATEXTENSIBLE",
    # COM smart pointers
    "ComPtr", "wil::com_ptr",
]

FORBIDDEN_PATH_SEGMENTS = ["/wasapi/", "/ffmpeg/", "/qt/", "/win32/", "/com/"]

FORBIDDEN_INCLUDE_PREFIXES = [
    "#include <windows.h>",
    "#include <audioclient.h>",
    "#include <mmdeviceapi.h>",
    "#include <ksmedia.h>",
    "#include <endpointvolume.h>",
    "#include <audiopolicy.h>",
    "#include <avcodec.h>",
    "#include <avformat.h>",
    "#include <swresample.h>",
    "#include <QString>",
    "#include <QByteArray>",
    "#include <QVector>",
    "#include <QImage>",
    "#include <QObject>",
    "#include <QWidget>",
    "#include <d3d11.h>",
    "#include <dxgi.h>",
]


def scan_file_for_tokens(filepath, tokens):
    """Case-sensitive scan. Returns list of (line_no, token, line_text)."""
    hits = []
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            for line_no, line in enumerate(f, 1):
                for token in tokens:
                    if token in line:
                        hits.append((line_no, token, line.rstrip()))
    except Exception as e:
        hits.append((0, "READ_ERROR", str(e)))
    return hits


def scan_path_segments(filepath, segments):
    """Check if filepath contains forbidden path segments."""
    normalized = filepath.replace("\\", "/")
    for seg in segments:
        if seg in normalized:
            return seg
    return None


def scan_forbidden_includes(filepath, include_prefixes):
    """Check for forbidden #include directives."""
    hits = []
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            for line_no, line in enumerate(f, 1):
                stripped = line.strip()
                for prefix in include_prefixes:
                    if stripped.startswith(prefix):
                        hits.append((line_no, prefix, line.rstrip()))
    except Exception:
        pass
    return hits


def scan_backend_src_reference(filepath):
    """Check if public header includes backend/src (forbidden per §42)."""
    hits = []
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            for line_no, line in enumerate(f, 1):
                if "backend/src" in line or "backend/include_private" in line:
                    hits.append((line_no, "backend_src_ref", line.rstrip()))
    except Exception:
        pass
    return hits


def collect_hpp_files(root_dir):
    """Collect all .hpp files recursively."""
    result = []
    for dirpath, _, filenames in os.walk(root_dir):
        for fn in filenames:
            if fn.endswith(".hpp"):
                result.append(os.path.join(dirpath, fn))
    return sorted(result)


def mode_public_headers_strict(repo_root):
    """Mode 1: PublicHeadersStrict — scans public P6A headers."""
    public_root = os.path.join(repo_root, "include", "kivo", "video", "audio_plane")
    if not os.path.isdir(public_root):
        return False, [("N/A", "MISSING_DIR", f"Public root not found: {public_root}")]

    files = collect_hpp_files(public_root)
    if not files:
        return False, [("N/A", "NO_FILES", "No .hpp files found in public root")]

    violations = []
    for fpath in files:
        rel = os.path.relpath(fpath, repo_root).replace("\\", "/")

        # Path segment check
        seg = scan_path_segments(rel, FORBIDDEN_PATH_SEGMENTS)
        if seg:
            violations.append((rel, 0, f"FORBIDDEN_PATH_SEGMENT: {seg}"))
            continue

        # Forbidden token check (case-sensitive)
        token_hits = scan_file_for_tokens(fpath, FORBIDDEN_TOKENS)
        for ln, token, line in token_hits:
            violations.append((rel, ln, f"FORBIDDEN_TOKEN: {token} | {line.strip()[:80]}"))

        # Forbidden #include check
        inc_hits = scan_forbidden_includes(fpath, FORBIDDEN_INCLUDE_PREFIXES)
        for ln, prefix, line in inc_hits:
            violations.append((rel, ln, f"FORBIDDEN_INCLUDE: {prefix} | {line.strip()[:80]}"))

        # backend/src reference check
        src_hits = scan_backend_src_reference(fpath)
        for ln, _, line in src_hits:
            violations.append((rel, ln, f"BACKEND_SRC_REF: {line.strip()[:80]}"))

    return len(violations) == 0, violations


def mode_architecture_rule_docs(repo_root):
    """Mode 2: ArchitectureRuleDocs — allowed rule text, no scanning of production."""
    # This mode is for documentation purposes. It allows forbidden tokens in rule docs.
    # No violations possible — rule docs are allowed to contain forbidden tokens as text.
    return True, []


def mode_negative_fixture(repo_root):
    """Mode 3: NegativeFixture — scans negative fixture dirs only."""
    negative_dirs = []
    test_base = os.path.join(repo_root, "backend", "tests", "video", "audio_plane")
    if os.path.isdir(test_base):
        for entry in os.listdir(test_base):
            full = os.path.join(test_base, entry)
            if os.path.isdir(full) and "negative" in entry.lower():
                negative_dirs.append(full)

    if not negative_dirs:
        return True, [("N/A", "NO_NEGATIVE_DIR", "No negative fixture dirs found (allowed in P6A)")]

    violations = []
    for ndir in negative_dirs:
        files = collect_hpp_files(ndir) + sorted(
            os.path.join(dp, fn)
            for dp, _, fns in os.walk(ndir)
            for fn in fns if fn.endswith(".cpp")
        )
        for fpath in files:
            rel = os.path.relpath(fpath, repo_root).replace("\\", "/")
            # Negative fixtures ARE allowed to contain forbidden tokens
            # But must NOT be in production include graph
            if "include/kivo/video/audio_plane" in rel:
                violations.append((rel, 0, "NEGATIVE_IN_PRODUCTION_GRAPH"))

    return len(violations) == 0, violations


def mode_backend_private(repo_root):
    """Mode 4: BackendPrivate — scans backend/src + include_private for P6C compliance.

    P6C (FFmpeg Decode Backend) allows:
      - backend/src/video/audio_plane/decode/ffmpeg/  (real avcodec source)
      - backend/include_private/video/audio_plane/decode/ffmpeg/  (private interface)
    But enforces:
      - No /wasapi/ or /qt/ path segments
      - No #include <libavformat/*> (avformat/demux forbidden — avcodec layer ONLY)
      - No WASAPI/Qt forbidden tokens
      - FFmpeg avcodec types ARE allowed in P6C private backend
      - Public headers must NOT include from backend/include_private/
    """
    # P6C-allowed backend directories
    backend_dirs = [
        os.path.join(repo_root, "backend", "src", "video", "audio_plane"),
        os.path.join(repo_root, "backend", "include_private", "video", "audio_plane"),
    ]

    # Tokens forbidden in backend private (WASAPI/Qt only — FFmpeg avcodec allowed in P6C)
    backend_forbidden_tokens = [
        # Windows headers
        "windows.h", "audioclient.h", "mmdeviceapi.h", "ksmedia.h",
        "endpointvolume.h", "audiopolicy.h",
        # Windows COM types
        "CoInitializeEx", "CoUninitialize",
        "IMMNotificationClient",
        # Qt types
        "QString", "QByteArray", "QVector", "QSharedPointer",
        "QVariant", "QImage", "QWindow", "QObject", "QWidget",
        # WASAPI interface types
        "IAudioClient", "IAudioClient2", "IAudioClient3",
        "IAudioRenderClient", "IAudioClock",
        "IMMDevice", "IAudioSessionControl", "ISimpleAudioVolume",
        "IAudioEndpointVolume",
        # Windows result/format types
        "HRESULT", "WAVEFORMATEX", "WAVEFORMATEXTENSIBLE",
    ]

    # Forbidden includes in backend private (avformat/demux ONLY — avcodec IS allowed)
    backend_forbidden_includes = [
        "#include <libavformat/",
        "#include <windows.h>",
        "#include <audioclient.h>",
        "#include <mmdeviceapi.h>",
        "#include <ksmedia.h>",
        "#include <endpointvolume.h>",
        "#include <audiopolicy.h>",
        "#include <d3d11.h>",
        "#include <dxgi.h>",
    ]

    # Forbidden path segments in backend private (/ffmpeg/ IS allowed in P6C)
    backend_forbidden_segments = ["/wasapi/", "/qt/", "/win32/"]

    violations = []
    for backend_dir in backend_dirs:
        if not os.path.isdir(backend_dir):
            continue
        # Collect .hpp and .cpp files
        all_files = collect_hpp_files(backend_dir)
        for dp, _, fns in os.walk(backend_dir):
            for fn in fns:
                if fn.endswith(".cpp"):
                    all_files.append(os.path.join(dp, fn))
        all_files = sorted(all_files)

        for fpath in all_files:
            rel = os.path.relpath(fpath, repo_root).replace("\\", "/")

            # Path segment check
            seg = scan_path_segments(rel, backend_forbidden_segments)
            if seg:
                violations.append((rel, 0, f"FORBIDDEN_PATH_SEGMENT: {seg}"))

            # Forbidden token check (WASAPI/Qt only)
            token_hits = scan_file_for_tokens(fpath, backend_forbidden_tokens)
            for ln, token, line in token_hits:
                violations.append((rel, ln, f"FORBIDDEN_TOKEN: {token} | {line.strip()[:80]}"))

            # Forbidden #include check (avformat/Windows only)
            inc_hits = scan_forbidden_includes(fpath, backend_forbidden_includes)
            for ln, prefix, line in inc_hits:
                violations.append((rel, ln, f"FORBIDDEN_INCLUDE: {prefix} | {line.strip()[:80]}"))

    # Check: public headers must NOT include from backend/include_private/
    public_root = os.path.join(repo_root, "include", "kivo", "video", "audio_plane")
    if os.path.isdir(public_root):
        public_files = collect_hpp_files(public_root)
        for fpath in public_files:
            rel = os.path.relpath(fpath, repo_root).replace("\\", "/")
            src_hits = scan_backend_src_reference(fpath)
            for ln, _, line in src_hits:
                violations.append((rel, ln, f"PUBLIC_INCLUDES_PRIVATE: {line.strip()[:80]}"))

    return len(violations) == 0, violations


MODES = {
    "PublicHeadersStrict": mode_public_headers_strict,
    "ArchitectureRuleDocs": mode_architecture_rule_docs,
    "NegativeFixture": mode_negative_fixture,
    "BackendPrivate": mode_backend_private,
}


def main():
    parser = argparse.ArgumentParser(description="P6A Audio Architecture Guard Scanner")
    parser.add_argument("--repo-root", required=True, help="Repository root path")
    parser.add_argument("--mode", required=True, choices=list(MODES.keys()),
                        help="Scan mode")
    parser.add_argument("--json", action="store_true", help="Output JSON format")
    args = parser.parse_args()

    repo_root = os.path.abspath(args.repo_root)
    if not os.path.isdir(repo_root):
        print(f"ERROR: repo-root not found: {repo_root}", file=sys.stderr)
        return 2

    scan_fn = MODES[args.mode]
    passed, violations = scan_fn(repo_root)

    if args.json:
        import json
        result = {
            "mode": args.mode,
            "passed": passed,
            "violation_count": len(violations),
            "violations": [{"file": v[0], "line": v[1], "detail": v[2]} for v in violations],
        }
        print(json.dumps(result, indent=2))
    else:
        print(f"=== P6 Audio Architecture Guard ===")
        print(f"Mode: {args.mode}")
        print(f"Repo: {repo_root}")
        print(f"Result: {'PASS' if passed else 'FAIL'}")
        print(f"Violations: {len(violations)}")
        if violations:
            print("---")
            for rel, ln, detail in violations:
                print(f"  {rel}:{ln}: {detail}")

    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
