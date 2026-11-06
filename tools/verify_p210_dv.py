#!/usr/bin/env python3
"""210 · DV-A 证据校验器（含变异自检）。

判据抄自 `210-dv-a-detect-passthrough-design.md` §六：
- AC1/AC2（8.x 透传轮）：kind=dolby_vision、verdict=passthrough_hdr10、dv_profile=8、
  bl_mime=video/hevc、configure_format_rebuilt=true、dataspace.applied=true、
  理由成对非空、零丢帧、帧数闭合；
- AC3（profile 5 拒绝轮）：kivo.p12f.negative/1、return_code=13、stage=open、
  reason 点名 dv_profile=5；
- AC4（回归轮）：既有片源 kind 不变（hdr10/sdr），dv.verdict=not_dolby_vision，
  透传动作全空，dataspace 行为与改动前一致。

文件按名字分类：`*dv81*` 透传轮、`*dv5*` 拒绝轮、`*regress-hdr10*` / `*regress-sdr*`
回归轮。**verdict 与 reason 成对出现**是本卡贯穿判据 —— reason 缺席即红。

`--self-test` 在临时目录造三份合成证据，先验正例全绿，再逐条变异
（抓不到任何一条即 exit 2）—— P16 门禁纪律：判据自己也要有人验。
"""
from __future__ import annotations

import json
import pathlib
import sys
import tempfile

SCHEMA_DECODE_PREFIX = "kivo.p12c.decode/"
SCHEMA_NEGATIVE = "kivo.p12f.negative/1"

DV_KEYS = ("verdict", "dv_profile", "dv_level", "bl_compatibility_id",
           "bl_mime", "configure_format_rebuilt", "reason")


class Report:
    def __init__(self) -> None:
        self.failures: list[str] = []
        self.checks = 0

    def check(self, ok: bool, what: str) -> bool:
        self.checks += 1
        if not ok:
            self.failures.append(what)
        return ok


def check_dv_block(dv: dict, rep: Report, label: str) -> None:
    for key in DV_KEYS:
        rep.check(key in dv, f"{label}: color.dv 缺键 {key}")
    rep.check(bool(dv.get("reason")), f"{label}: color.dv.reason 非空（成对铁律）")


def check_passthrough(d: dict, rep: Report, label: str) -> None:
    color = d.get("color", {})
    hdr = color.get("hdr", {})
    dv = color.get("dv", {})
    rep.check(hdr.get("kind") == "dolby_vision", f"{label}: hdr.kind=dolby_vision")
    rep.check(dv.get("verdict") == "passthrough_hdr10", f"{label}: dv.verdict")
    rep.check(dv.get("dv_profile") == 8, f"{label}: dv_profile=8")
    rep.check(dv.get("bl_mime") == "video/hevc", f"{label}: bl_mime=video/hevc")
    rep.check(dv.get("configure_format_rebuilt") is True,
              f"{label}: configure_format_rebuilt=true")
    dataspace = hdr.get("dataspace", {})
    rep.check(dataspace.get("applied") is True, f"{label}: dataspace.applied=true")
    rep.check(bool(hdr.get("kind_reason")), f"{label}: kind_reason 非空")
    counters = d.get("counters", {})
    received = counters.get("frames_received")
    rendered = counters.get("frames_rendered")
    dropped = counters.get("frames_dropped_late")
    disc = counters.get("frames_discarded_no_surface") or 0
    rep.check(dropped == 0, f"{label}: 丢帧=0")
    if received is not None and rendered is not None:
        # end_reason 在 outcome 里（P14-R 加的那格），不在顶层。
        eos_bonus = 1 if d.get("outcome", {}).get("end_reason") == "completed" else 0
        rep.check(received == rendered + dropped + disc + eos_bonus,
                  f"{label}: 帧数闭合 {received} vs {rendered}+{dropped}+{disc}+{eos_bonus}")
    rep.check(d.get("outcome", {}).get("completed") is True or d.get("outcome") is None,
              f"{label}: outcome.completed=true")


def check_regress(kind: str, applied: bool, d: dict, rep: Report, label: str) -> None:
    color = d.get("color", {})
    hdr = color.get("hdr", {})
    dv = color.get("dv", {})
    rep.check(hdr.get("kind") == kind, f"{label}: hdr.kind={kind}（回归：不得改变）")
    rep.check(dv.get("verdict") == "not_dolby_vision", f"{label}: dv.verdict=not_dolby_vision")
    rep.check(dv.get("dv_profile") is None, f"{label}: 非 DV 片源 dv_profile=null")
    rep.check(dv.get("bl_mime") is None, f"{label}: 非 DV 片源 bl_mime=null")
    rep.check(dv.get("configure_format_rebuilt") is False,
              f"{label}: 非 DV 片源不重建格式")
    ds = hdr.get("dataspace", {})
    rep.check(ds.get("applied") is applied, f"{label}: dataspace.applied={applied}（回归）")


def check_negative(d: dict, rep: Report, label: str) -> None:
    rep.check(d.get("schema") == SCHEMA_NEGATIVE, f"{label}: schema={SCHEMA_NEGATIVE}")
    rep.check(d.get("stage") == "open", f"{label}: stage=open（configure 之前拒绝）")
    rep.check(d.get("return_code") == 13, f"{label}: return_code=13（DV 专码）")
    reason = d.get("reason") or ""
    rep.check("dv_profile=5" in reason, f"{label}: reason 点名 dv_profile=5")


def check_file(path: pathlib.Path, rep: Report) -> str:
    d = json.loads(path.read_text(encoding="utf-8"))
    schema = d.get("schema", "")
    label = path.name
    if schema.startswith("kivo.p12e.hdr-probe"):
        # 设计期的 track_format_dump 实测（更正块 §十 的证据），不是播放取证，
        # 不按播放判据查 —— 单列分类，计入覆盖。
        return "probe"
    if schema == SCHEMA_NEGATIVE or "dv5" in path.name:
        check_negative(d, rep, label)
        return "negative"
    if not schema.startswith(SCHEMA_DECODE_PREFIX):
        rep.check(False, f"{label}: schema 前缀不认识（{schema}）")
        return "unknown"
    color = d.get("color", {})
    dv = color.get("dv", {})
    check_dv_block(dv, rep, label)
    name = path.name
    if "dv81" in name:
        check_passthrough(d, rep, label)
        return "passthrough"
    if "regress-hdr10" in name:
        check_regress("hdr10", True, d, rep, label)
        return "regress"
    if "regress-sdr" in name:
        check_regress("sdr", False, d, rep, label)
        return "regress"
    rep.check(False, f"{label}: 文件名不含 dv81/dv5/regress，不知道该按哪套判据")
    return "unknown"


def synth_evidence(kind: str) -> dict:
    """最小但字段齐全的合成证据（自检用）。"""
    if kind == "negative":
        return {"schema": SCHEMA_NEGATIVE, "case": "dv-profile5", "stage": "open",
                "return_code": 13, "media": "dv5.mp4",
                "reason": "open(dv5.mp4): dv_profile=5 (IPT-coded BL ...)"}
    base = {
        "schema": "kivo.p12c.decode/19",
        "end_reason": "completed",
        "outcome": {"completed": True, "end_reason": "completed", "error": ""},
        "counters": {"frames_received": 301, "frames_rendered": 300,
                     "frames_dropped_late": 0, "frames_discarded_no_surface": 0},
        "color": {
            "hdr": {"kind": "dolby_vision", "kind_reason": "dv facts",
                    "static_info_bytes": None,
                    "dataspace": {"applied": True, "value": 163971072, "reason": None}},
            "dv": {"verdict": "passthrough_hdr10", "dv_profile": 8, "dv_level": 1,
                   "bl_compatibility_id": 1, "bl_mime": "video/hevc",
                   "configure_format_rebuilt": True, "reason": "dv_profile=8 ..."},
            "verdict": "kIndeterminate", "reason": "...",
        },
    }
    if kind != "dv81":
        base["color"]["hdr"]["kind"] = "hdr10" if kind == "regress-hdr10" else "sdr"
        base["color"]["dv"] = {"verdict": "not_dolby_vision", "dv_profile": None,
                               "dv_level": None, "bl_compatibility_id": None,
                               "bl_mime": None, "configure_format_rebuilt": False,
                               "reason": "mime is video/hevc; not a Dolby Vision track"}
        if kind == "regress-sdr":
            base["color"]["hdr"]["dataspace"] = {"applied": False, "value": 0,
                                                 "reason": "source is SDR; ..."}
    return base


def self_test() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        tmpdir = pathlib.Path(tmp)
        cases = ("dv81", "regress-hdr10", "regress-sdr", "negative")
        for kind in cases:
            good = tmpdir / f"p210-{kind}-r1.json"
            good.write_text(json.dumps(synth_evidence(kind)), encoding="utf-8")
            rep = Report()
            check_file(good, rep)
            if rep.failures:
                print(f"自检：正例 {kind} 判红 —— 判据写错了：{rep.failures}")
                return 2
        mutations = (
            ("dv81", lambda d: d["color"]["hdr"].__setitem__("kind", "hdr10")),
            ("dv81", lambda d: d["color"]["dv"].__setitem__("verdict", "unsupported")),
            ("dv81", lambda d: d["color"]["dv"].__setitem__("bl_mime", "video/avc")),
            ("dv81", lambda d: d["color"]["dv"].__setitem__("reason", "")),
            ("dv81", lambda d: d["color"]["hdr"]["dataspace"].__setitem__("applied", False)),
            ("dv81", lambda d: d["counters"].__setitem__("frames_dropped_late", 3)),
            ("dv81", lambda d: d["color"]["dv"].pop("bl_mime")),
            ("negative", lambda d: d.__setitem__("return_code", 12)),
            ("negative", lambda d: d.__setitem__("reason", "open failed: no decoder")),
            ("regress-hdr10", lambda d: d["color"]["dv"].__setitem__("verdict",
                                                                     "passthrough_hdr10")),
        )
        for kind, mutate in mutations:
            mutated = json.loads(json.dumps(synth_evidence(kind)))
            mutate(mutated)
            bad = tmpdir / f"p210-{kind}-mutated.json"
            bad.write_text(json.dumps(mutated), encoding="utf-8")
            rep = Report()
            check_file(bad, rep)
            if not rep.failures:
                print(f"自检：变异未被抓到（{kind}）—— 判据有洞，exit 2")
                return 2
        print(f"自检：4 正例全绿 + {len(mutations)} 条变异全部被抓到")
    return 0


def main(argv: list[str]) -> int:
    args = [a for a in argv if a != "--self-test"]
    if "--self-test" in argv:
        return self_test()
    if not args:
        roots = [pathlib.Path("docs/evidence/p210")]
    else:
        roots = [pathlib.Path(a) for a in args]
    files = sorted(p for root in roots
                   for p in (root.rglob("*.json") if root.is_dir() else [root]))
    if not files:
        print("没有找到证据文件（docs/evidence/p210/*.json）")
        return 1
    total_checks, total_failures = 0, 0
    kinds: dict[str, int] = {}
    for path in files:
        rep = Report()
        kind = "parse-error"
        try:
            kind = check_file(path, rep)
        except (json.JSONDecodeError, KeyError) as exc:
            rep.check(False, f"{path.name}: 解析失败 {exc}")
        kinds[kind] = kinds.get(kind, 0) + 1
        total_checks += rep.checks
        total_failures += len(rep.failures)
        status = "PASS" if not rep.failures else "FAIL"
        print(f"  {status} {path.name}（{kind}，{rep.checks} 项）")
        for failure in rep.failures:
            print(f"       - {failure}")
    covered = sum(kinds.values())
    print(f"覆盖：{covered} 份 / {total_checks} 项；分类：{kinds}")
    if total_failures:
        print(f"FAIL：{total_failures} 项")
        return 1
    print("PASS：210 DV-A 证据全部通过")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
