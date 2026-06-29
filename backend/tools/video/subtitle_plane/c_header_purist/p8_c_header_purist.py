from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
PUBLIC_ROOT = ROOT / "include" / "kivo" / "video" / "subtitle_plane"
FORBIDDEN = [
    "std::",
    "noexcept",
    "class ",
    "QString",
    "QObject",
    "AVFrame",
    "AVPacket",
    "windows.h",
    "d3d11.h",
    "dxgi.h",
    "ComPtr",
    "KivoInputBytesView",
    "volatile uint32_t*",
]


def main() -> int:
    violations: list[dict[str, str]] = []
    files: list[str] = []
    for path in sorted(PUBLIC_ROOT.rglob("*.h")):
        files.append(str(path.relative_to(ROOT)))
        text = path.read_text(encoding="utf-8", errors="ignore")
        for token in FORBIDDEN:
            if token in text:
                violations.append({"path": str(path.relative_to(ROOT)), "token": token})
    report = {
        "tool": "p8_c_header_purist",
        "status": "PASS" if not violations else "FAIL",
        "files_scanned": files,
        "forbidden_tokens": FORBIDDEN,
        "violations": violations,
    }
    print(json.dumps(report, indent=2))
    return 0 if not violations else 1


if __name__ == "__main__":
    raise SystemExit(main())
