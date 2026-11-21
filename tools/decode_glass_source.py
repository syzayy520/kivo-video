#!/usr/bin/env python3
"""Decode a GitHub contents-API JSON captured by WebFetch into readable source.

Usage:
    python tools/decode_glass_source.py <webfetch.md> <output.kt>

The capture file is the tool output markdown saved under .qoder/tmp; the JSON
payload may be followed by a "[Local page source]" trailer, so we locate the
first {"name": ...} object and pull its base64 "content" field.
Research-only helper: source is Apache-2.0, used to read material constants,
never copied into product code.
"""
import base64
import json
import re
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    raw = Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
    match = re.search(r'\{"name":.*?"content":"((?:[A-Za-z0-9+/=\\n])*)"', raw, re.S)
    if not match:
        print("no base64 content field found", file=sys.stderr)
        return 1
    payload = match.group(1).replace("\\n", "")
    data = base64.b64decode(payload)
    out = Path(sys.argv[2])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)
    print(f"wrote {out} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
