from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
PUBLIC_ROOT = ROOT / "include" / "kivo" / "video" / "subtitle_plane"
TEST_FILE = ROOT / "backend" / "tests" / "video" / "subtitle_plane" / "abi" / "p8_public_contract_tests.cpp"


def public_structs() -> set[str]:
    names: set[str] = set()
    for path in PUBLIC_ROOT.rglob("*.h"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        names.update(re.findall(r"typedef struct\s+([A-Za-z0-9_]+)", text))
    return names


def main() -> int:
    text = TEST_FILE.read_text(encoding="utf-8", errors="ignore")
    structs = public_structs()
    covered = set(re.findall(r"KIVO_P8_ASSERT_ABI_STRUCT\((Kivo[A-Za-z0-9_]+)\)", text))
    intentionally_small = {
        "KivoSubtitleFrameId",
        "KivoSubtitleSnapshotId",
        "KivoSubtitleTrackId",
        "KivoSubtitleSourceId",
        "KivoSubtitleCommandId",
        "KivoSubtitleEvidenceId",
        "KivoSubtitleTextBufferId",
        "KivoSubtitleFontBlobId",
        "KivoSubtitleStringId",
        "KivoSubtitleCorrelationId",
        "KivoSubtitleSnapshotLeaseId",
        "KivoSubtitleDiscoveryId",
        "KivoSubtitleSubscriptionToken",
        "KivoSubtitleSeekEpoch",
        "KivoP8PlaybackRevision",
        "KivoP8GraphSnapshotRevision",
        "KivoSubtitleSnapshotRevision",
        "KivoSubtitleTime",
        "KivoSubtitleDelayOffset",
    }
    missing = sorted(structs - covered - intentionally_small)
    report = {
        "tool": "p8_abi_layout_guard",
        "status": "PASS" if not missing else "FAIL",
        "public_struct_count": len(structs),
        "covered_struct_count": len(covered),
        "intentionally_small_wrappers": sorted(intentionally_small & structs),
        "missing": missing,
        "test_file": str(TEST_FILE.relative_to(ROOT)),
    }
    print(json.dumps(report, indent=2))
    return 0 if not missing else 1


if __name__ == "__main__":
    raise SystemExit(main())
