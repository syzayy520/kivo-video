#!/usr/bin/env python3
"""P12-F3 负向路径校验：四条用例的证据是否都"把话说到位"。

用法：

    python tools/verify_p12f3_errors.py docs/evidence/p12f3

判据（写死，别放宽）：

1. 每份证据的 `reason` **非空** —— 空串一律判失败（那正是 G9 要防的"静默"）；
2. `reason` 要指出**是哪种问题**，不能只说 "failed"：按用例查关键词；
3. 四份证据必须都在，缺一份就判失败 —— 不能因为"跑漏一条"而显示通过。

**加密那条在 P12-F3B 之后的判法变了**：不再是"看 sample flag 有没有 ENCRYPTED"
（K40 上那位对真 CENC 恒为 0），而是看**打开是否被拒**：
`return_code` 必须是 12、`stage` 必须是 `open`、`reason` 必须点名 crypto/encrypted。
判定依据见 `51-p12f3b-design.md`。
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

# 用例 → 期望。`file` 是证据文件名，`field` 是 reason 所在字段。
CASES = {
    "bad-file": {
        "file": "p12f3-bad-file-failure.json",
        "field": ("reason",),
        "keywords": ("truncated", "corrupt", "0 tracks"),
        "stage": "open",
    },
    "no-video-track": {
        "file": "p12f3-no-video-track-failure.json",
        "field": ("reason",),
        "keywords": ("no video track",),
        "stage": "open",
    },
    "no-audio-track": {
        "file": "p12f3-no-audio-track-sync.json",
        "field": ("av_drift", "unavailable_reason"),
        "keywords": ("no audio track",),
        "stage": None,
    },
    "encrypted": {
        "file": "p12f3-encrypted-failure.json",
        "field": ("reason",),
        # reason 必须点名"加密"，而不是笼统的"打不开"。
        "keywords": ("crypto-mode", "encrypted", "not supported"),
        "stage": "open",
        # P12-F3B 专用：加密拒绝有独立返回码，能把它和普通打开失败分开。
        "return_code": 12,
    },
}


def dig(payload: dict, path: tuple[str, ...]) -> str:
    node = payload
    for key in path:
        if not isinstance(node, dict) or key not in node:
            return ""
        node = node[key]
    return node if isinstance(node, str) else ""


def main(argv: list[str]) -> int:
    root = Path(argv[1] if len(argv) > 1 else "docs/evidence/p12f3")
    failures = 0
    rows: list[tuple[str, str, str]] = []

    for case, spec in CASES.items():
        path = root / spec["file"]
        if not path.exists():
            rows.append((case, "MISSING", f"证据文件不存在：{spec['file']}"))
            failures += 1
            continue

        payload = json.loads(path.read_text(encoding="utf-8"))
        reason = dig(payload, spec["field"])

        # P12-F3B：加密拒绝必须带自己的返回码。
        # 只查 reason 字符串是不够的 —— 那样"打不开"加一句"可能是加密"也能过。
        expected_code = spec.get("return_code")
        if expected_code is not None and payload.get("return_code") != expected_code:
            rows.append(
                (case, "FAIL",
                 f"return_code={payload.get('return_code')} 期望 {expected_code}"
                 "（12 才是'加密轨被拒'，5 是普通打开失败）")
            )
            failures += 1
            continue

        if not reason:
            rows.append((case, "FAIL", f"{'.'.join(spec['field'])} 为空"))
            failures += 1
            continue
        if spec.get("stage") and payload.get("stage") != spec["stage"]:
            rows.append((case, "FAIL", f"stage={payload.get('stage')} 期望 {spec['stage']}"))
            failures += 1
            continue
        if not any(keyword in reason for keyword in spec["keywords"]):
            rows.append((case, "FAIL", f"reason 没点到问题类型：{reason[:80]}"))
            failures += 1
            continue
        rows.append((case, "PASS", reason[:90]))

    for case, status, note in rows:
        print(f"[{status}] {case}: {note}")
    print(f"\n--- {len(rows)} 条用例，{failures} 条未通过 ---")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
