#!/usr/bin/env python3
"""kivo video · UI 治理门禁（行数 / 裸色 / Preview）。

三项指标的审计口径见 `docs/audit/ui-governance-audit.md` §0，
修复批次见 `docs/audit/ui-parity-master-report.md` §7.2 / §10。

用法（在仓库根直接运行，仅依赖 Python 3 标准库）：

    python tools/ui_gate.py                 # 人类可读报告 + 退出码
    python tools/ui_gate.py --json          # 机器可读报告（含退出码字段）
    python tools/ui_gate.py --baseline      # 把当前三项指标写入 docs/audit/ui-gate-baseline.json

退出码：任何**回归**返回 1，否则 0。
回归定义为：裸色数高于 baseline、出现 baseline 之外的新超限文件、缺 @Preview 的 Page 数增加。
"""

import argparse
import datetime
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KT_ROOT = ROOT / "android" / "app" / "src" / "main" / "java" / "com" / "kivo" / "cinema"
BASELINE_PATH = ROOT / "docs" / "audit" / "ui-gate-baseline.json"

LINE_LIMIT = 300
REPORT_TOP_BARE_COLOR_FILES = 20

# 白名单：允许超过 300 行，但只能在 baseline 中登记为「待拆」并始终以 warning 报告。
# batch 是拆解归属的修复批次（master-report §10）；None 行数表示此文件当前未超限。
LINE_WHITELIST = {
    "ui/PlayerActivity.kt": {
        "batch": "B5（治理）",
        "note": "901 行，UI 主文件最大；B4 播放器先对齐帧，B5 拆到家族树节点",
    },
    "ui/detail/EmbyResource.kt": {
        "batch": "B5（治理）",
        "note": "454 行，Emby 详情资源块与页面杂糅，B5 拆分",
    },
    "ui/design/DiscoverPage.kt": {
        "batch": "B3（发现）/ B5（治理）",
        "note": "426 行；B3 补 6 个模块时先行内聚，B5 拆分",
    },
    "ui/compose/PlayerActionRouter.kt": {
        "batch": "B4（播放器）/ B5（治理）",
        "note": "406 行；B4 改手势与面板后由 B5 拆路由表",
    },
    "ui/compose/PlayerComposeActivity.kt": {
        "batch": "B4（播放器）/ B5（治理）",
        "note": "382 行；B4 播放器交互改动的宿主，B5 拆分",
    },
    "ui/MultiPlayerActivity.kt": {
        "batch": "B5（治理）",
        "note": "多实例播放宿主，当前已 ≤300 行；留白名单防回归时误报",
    },
}

BARE_HEX_RE = re.compile(r"Color\(0x[0-9A-Fa-f]+")
BARE_NAMED_RE = re.compile(r"Color\.(?:White|Black)\b")
PREVIEW_RE = re.compile(r"@Preview\b")


def kt_files():
    """按相对路径排序返回全部 Kotlin 源文件。"""
    return sorted(
        (p for p in KT_ROOT.rglob("*.kt") if p.is_file()),
        key=lambda p: p.relative_to(KT_ROOT).as_posix(),
    )


def rel(path):
    return path.relative_to(KT_ROOT).as_posix()


def in_tokens_dir(path):
    """tokens/ 目录（含任意层级同名目录）内的文件视为令牌定义，不计裸色。"""
    return "tokens" in path.relative_to(KT_ROOT).parts[:-1]


def count_lines(path):
    # 与 `wc -l` 一致：只数换行符，末行无换行时不计。
    return path.read_bytes().count(b"\n")


def scan():
    files = kt_files()
    over_limit = {}
    bare_by_file = {}
    pages = []
    pages_missing_preview = []

    for path in files:
        relpath = rel(path)
        lines = count_lines(path)
        if lines > LINE_LIMIT:
            over_limit[relpath] = lines

        text = path.read_text(encoding="utf-8", errors="replace")

        if not in_tokens_dir(path):
            hits = len(BARE_HEX_RE.findall(text)) + len(BARE_NAMED_RE.findall(text))
            if hits:
                bare_by_file[relpath] = hits

        if path.name.endswith("Page.kt"):
            pages.append(relpath)
            if not PREVIEW_RE.search(text):
                pages_missing_preview.append(relpath)

    whitelisted = {p: v for p, v in over_limit.items() if p in LINE_WHITELIST}
    non_whitelisted = {p: v for p, v in over_limit.items() if p not in LINE_WHITELIST}

    return {
        "kt_file_count": len(files),
        "files_over_300": {
            "limit": LINE_LIMIT,
            "count": len(over_limit),
            "files": dict(sorted(over_limit.items(), key=lambda kv: (-kv[1], kv[0]))),
            "whitelisted_pending_split": {
                p: {
                    "lines": lines,
                    "batch": LINE_WHITELIST[p]["batch"],
                    "note": LINE_WHITELIST[p]["note"],
                }
                for p, lines in sorted(whitelisted.items(), key=lambda kv: (-kv[1], kv[0]))
            },
            "non_whitelisted": dict(sorted(non_whitelisted.items(), key=lambda kv: (-kv[1], kv[0]))),
        },
        "bare_color_total": sum(bare_by_file.values()),
        "bare_color_by_file": dict(sorted(bare_by_file.items(), key=lambda kv: (-kv[1], kv[0]))),
        "page_count": len(pages),
        "page_preview_missing": len(pages_missing_preview),
        "page_preview_missing_files": pages_missing_preview,
    }


def load_baseline():
    if not BASELINE_PATH.exists():
        return None
    try:
        return json.loads(BASELINE_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"[WARN] baseline 无法解析（{exc}），本次不做回归比较：{BASELINE_PATH}", file=sys.stderr)
        return None


def baseline_doc(m):
    return {
        "generated_at": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
        "files_over_300": m["files_over_300"],
        "bare_color_total": m["bare_color_total"],
        "page_preview_missing": m["page_preview_missing"],
        "page_preview_missing_files": m["page_preview_missing_files"],
        "thresholds": {
            "line_limit": LINE_LIMIT,
            "bare_color_total": "不得高于本文件记录的 bare_color_total",
            "page_preview": "每个 *Page.kt 至少 1 个 @Preview；missing 数不得增加",
            "whitelisted_pending_split": LINE_WHITELIST,
        },
        "source": "python tools/ui_gate.py --baseline",
        "scope": "android/app/src/main/java/com/kivo/cinema/**/*.kt（tokens/ 目录不计裸色）",
    }


def evaluate(m, baseline):
    """返回 (violations, regressions, warnings, notes)。

    violations = 存量违规（报告层面报错，但 baseline 已记录，不阻断退出码）；
    regressions = 相对 baseline 的回归，非空即退出码 1。
    """
    violations, regressions, warnings, notes = [], [], [], []
    files_over = m["files_over_300"]
    base_over = set(baseline.get("files_over_300", {}).get("files", {})) if baseline else None

    for relpath, lines in files_over["non_whitelisted"].items():
        msg = f"{relpath} {lines} 行 > {LINE_LIMIT}"
        if base_over is not None and relpath not in base_over:
            regressions.append(f"行数回归（新增超限）：{msg}")
        else:
            violations.append(f"行数超限（存量，待 B5 拆分）：{msg}")
    for relpath, info in files_over["whitelisted_pending_split"].items():
        warnings.append(f"待拆（白名单 {info['batch']}）：{relpath} {info['lines']} 行 > {LINE_LIMIT}")

    for relpath in m["page_preview_missing_files"]:
        warnings.append(f"缺 @Preview：{relpath}（B2/B3/B4 起补）")

    if baseline is None:
        notes.append("baseline 不存在，只报存量、不做回归比较；运行 `python tools/ui_gate.py --baseline` 固化基线")
        return violations, regressions, warnings, notes

    base_bare = int(baseline.get("bare_color_total", 0))
    cur_bare = m["bare_color_total"]
    if cur_bare > base_bare:
        regressions.append(f"裸色回归：{cur_bare} > baseline {base_bare}（+{cur_bare - base_bare}）")
    elif cur_bare == base_bare:
        warnings.append(f"裸色与 baseline 持平：{cur_bare}（未下降）")
    else:
        notes.append(f"裸色下降：{cur_bare} < baseline {base_bare}（−{base_bare - cur_bare}），可用 --baseline 刷新")

    base_missing = int(baseline.get("page_preview_missing", 0))
    cur_missing = m["page_preview_missing"]
    if cur_missing > base_missing:
        regressions.append(f"Preview 回归：缺 @Preview 的 {cur_missing} 个 Page > baseline {base_missing}")
    elif cur_missing == base_missing:
        warnings.append(f"缺 @Preview 的 Page 数与 baseline 持平：{cur_missing}")
    else:
        notes.append(f"缺 @Preview 的 Page 减少：{cur_missing} < baseline {base_missing}，可用 --baseline 刷新")

    if base_over is not None and not any("行数回归" in item for item in regressions):
        notes.append(f"行数无新增超限文件（当前超限 {files_over['count']} 个，均在 baseline 内）")

    return violations, regressions, warnings, notes


def print_report(m, violations, regressions, warnings, notes, baseline_path=None):
    print(f"kivo video · UI Gate（{datetime.datetime.now().astimezone().isoformat(timespec='seconds')}）")
    print(f"扫描范围：{KT_ROOT}")
    print(f"Kotlin 文件 {m['kt_file_count']} 个 · Page 文件 {m['page_count']} 个")
    print()

    print(f"── 1. 行数（上限 {LINE_LIMIT}）")
    fo = m["files_over_300"]
    print(f"   超限 {fo['count']} 个（白名单待拆 {len(fo['whitelisted_pending_split'])} 个，待拆分 {len(fo['non_whitelisted'])} 个）")
    for relpath, lines in fo["non_whitelisted"].items():
        print(f"   [FAIL] {relpath:<52} {lines} 行")
    for relpath, info in fo["whitelisted_pending_split"].items():
        print(f"   [WARN] {relpath:<52} {info['lines']} 行 · 待拆 {info['batch']}")

    print()
    print("── 2. 裸色（tokens/ 之外 Color(0x… 与 Color.White/Black）")
    print(f"   合计 {m['bare_color_total']} 处，按文件 top {REPORT_TOP_BARE_COLOR_FILES}：")
    top = list(m["bare_color_by_file"].items())[:REPORT_TOP_BARE_COLOR_FILES]
    for relpath, hits in top:
        print(f"   {relpath:<60} {hits}")
    if not top:
        print("   （无）")

    print()
    print("── 3. Preview（每个 *Page.kt 至少 1 个 @Preview）")
    print(f"   缺 @Preview 的 Page：{m['page_preview_missing']} / {m['page_count']}")
    for relpath in m["page_preview_missing_files"]:
        print(f"   [WARN] {relpath}")

    print()
    print("── 4. 回归结论（退出码只由回归决定）")
    if violations:
        print(f"   [FAIL] 存量违规 {len(violations)} 项（已在 baseline 中，不算回归；修复目标见 B5）")
        for item in violations:
            print(f"          · {item}")
    for item in regressions:
        print(f"   [FAIL] {item}")
    for item in warnings:
        print(f"   [WARN] {item}")
    for item in notes:
        print(f"   [INFO] {item}")
    if baseline_path:
        print(f"   基线文件：{baseline_path}")
    print()
    if regressions:
        print(f"RESULT: FAIL（{len(regressions)} 项回归）")
    else:
        print("RESULT: PASS（无回归；存量违规与 warning 为待治理项，不阻断）")


def main(argv=None):
    parser = argparse.ArgumentParser(description="kivo video UI 门禁：行数 / 裸色 / Preview")
    parser.add_argument("--baseline", action="store_true", help="把当前指标写入 docs/audit/ui-gate-baseline.json")
    parser.add_argument("--json", action="store_true", help="输出 JSON 摘要")
    args = parser.parse_args(argv)

    if not KT_ROOT.is_dir():
        print(f"找不到 Kotlin 源目录：{KT_ROOT}", file=sys.stderr)
        return 2

    metrics = scan()
    baseline = load_baseline()

    if args.baseline:
        BASELINE_PATH.parent.mkdir(parents=True, exist_ok=True)
        doc = baseline_doc(metrics)
        BASELINE_PATH.write_text(json.dumps(doc, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        # 刚写入的基线即本次比较基准：--baseline 自身不产生回归。
        baseline = doc
        if not args.json:
            print(f"[OK] baseline 已写入 {BASELINE_PATH}\n")

    violations, regressions, warnings, notes = evaluate(metrics, baseline)

    if args.json:
        payload = {
            "generated_at": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
            "metrics": {
                "kt_file_count": metrics["kt_file_count"],
                "files_over_300_count": metrics["files_over_300"]["count"],
                "files_over_300_non_whitelisted": list(metrics["files_over_300"]["non_whitelisted"]),
                "files_over_300_whitelisted_pending_split": {
                    p: i["batch"] for p, i in metrics["files_over_300"]["whitelisted_pending_split"].items()
                },
                "bare_color_total": metrics["bare_color_total"],
                "bare_color_by_file": metrics["bare_color_by_file"],
                "page_count": metrics["page_count"],
                "page_preview_missing": metrics["page_preview_missing"],
                "page_preview_missing_files": metrics["page_preview_missing_files"],
            },
            "baseline": None
            if baseline is None
            else {
                "path": str(BASELINE_PATH.relative_to(ROOT)).replace("\\", "/"),
                "generated_at": baseline.get("generated_at"),
                "bare_color_total": baseline.get("bare_color_total"),
                "page_preview_missing": baseline.get("page_preview_missing"),
            },
            "violations": violations,
            "regressions": regressions,
            "warnings": warnings,
            "notes": notes,
            "exit_code": 1 if regressions else 0,
        }
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return payload["exit_code"]

    print_report(
        metrics,
        violations,
        regressions,
        warnings,
        notes,
        baseline_path=str(BASELINE_PATH.relative_to(ROOT)).replace("\\", "/"),
    )
    return 1 if regressions else 0


if __name__ == "__main__":
    sys.exit(main())