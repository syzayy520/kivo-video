#!/usr/bin/env python3
"""211 · 软解兜底证据校验器（含变异自检）。

判据抄自 `211-soft-decode-fallback-design.md` §五：
- 软解证据（`kivo.p211.decode/1`）：backend=ffmpeg、计数闭合
  （rendered + render_failures == frames_decoded）、routing 块理由非空、
  end_reason 合法、sink 名非空；文件名带 `-visible` 的轮次**额外**要求
  rendered == frames_decoded（AC1/AC2 的可见播放轮，锁屏轮不冒充）；
- 硬解对照（decode schema）：`decoder.backend == "mediacodec"`（AC3 的字段判据）；
- 探针证据（`kivo.p211.sd-probe/1`）：ok=true 且 frames_decoded ≥ 1。

`--self-test`：合成正例 + 逐条变异（抓不到任何一条即 exit 2）——P16 门禁纪律。
"""
from __future__ import annotations

import json
import pathlib
import sys
import tempfile

SCHEMA_DECODE_PREFIX = "kivo.p12c.decode/"
SCHEMA_SOFTDEC = "kivo.p211.decode/1"
SCHEMA_PROBE = "kivo.p211.sd-probe/1"

COUNTER_KEYS = ("packets_read", "video_packets", "keyframes", "frames_decoded",
                "frames_rendered", "render_failures", "read_errors")
END_REASONS_OK = ("completed", "interrupted")


class Report:
    def __init__(self) -> None:
        self.failures: list[str] = []
        self.checks = 0

    def check(self, ok: bool, what: str) -> bool:
        self.checks += 1
        if not ok:
            self.failures.append(what)
        return ok


def check_softdec(d: dict, rep: Report, label: str) -> None:
    rep.check(d.get("schema") == SCHEMA_SOFTDEC, f"{label}: schema={SCHEMA_SOFTDEC}")
    decoder = d.get("decoder", {})
    rep.check(decoder.get("backend") == "ffmpeg", f"{label}: decoder.backend=ffmpeg")
    counters = d.get("counters", {})
    for key in COUNTER_KEYS:
        rep.check(key in counters, f"{label}: counters 缺键 {key}")
    decoded = counters.get("frames_decoded")
    rendered = counters.get("frames_rendered")
    failures = counters.get("render_failures")
    if None not in (decoded, rendered, failures):
        rep.check(rendered + failures == decoded,
                  f"{label}: 计数闭合 rendered({rendered})+fail({failures})==decoded({decoded})")
    end_reason = d.get("end_reason") or ""
    rep.check(end_reason in END_REASONS_OK or end_reason.startswith("error:"),
              f"{label}: end_reason 合法（{end_reason}）")
    routing = d.get("routing", {})
    rep.check(bool(routing.get("reason")), f"{label}: routing.reason 非空（AC5）")
    rep.check(routing.get("backend_decision") in ("ffmpeg", "mediacodec"),
              f"{label}: routing.backend_decision 合法")
    rep.check(bool(d.get("render", {}).get("sink")), f"{label}: sink 名非空")
    if "-visible" in label:
        # AC1/AC2 的可见播放轮：锁屏轮的失败计数在结构上合法，
        # 但它不许冒充可见播放——文件名说了算。
        rep.check(rendered == decoded and failures == 0,
                  f"{label}: 可见轮要求 rendered==decoded 且零渲染失败")


def check_mediacodec(d: dict, rep: Report, label: str) -> None:
    rep.check(d.get("schema", "").startswith(SCHEMA_DECODE_PREFIX),
              f"{label}: decode schema 前缀")
    decoder = d.get("decoder", {})
    rep.check(decoder.get("backend") == "mediacodec",
              f"{label}: decoder.backend=mediacodec（AC3 字段判据）")


def check_probe(d: dict, rep: Report, label: str) -> None:
    rep.check(d.get("schema") == SCHEMA_PROBE, f"{label}: schema={SCHEMA_PROBE}")
    rep.check(d.get("ok") is True, f"{label}: ok=true")
    rep.check((d.get("frames_decoded") or 0) >= 1, f"{label}: frames_decoded ≥ 1")


def check_file(path: pathlib.Path, rep: Report) -> str:
    d = json.loads(path.read_text(encoding="utf-8"))
    schema = d.get("schema", "")
    label = path.name
    if schema == SCHEMA_SOFTDEC:
        check_softdec(d, rep, label)
        return "softdec"
    if schema == SCHEMA_PROBE:
        check_probe(d, rep, label)
        return "probe"
    if schema.startswith(SCHEMA_DECODE_PREFIX):
        check_mediacodec(d, rep, label)
        return "mediacodec"
    rep.check(False, f"{label}: schema 不认识（{schema}）")
    return "unknown"


def synth(kind: str) -> dict:
    if kind == "softdec":
        return {
            "schema": SCHEMA_SOFTDEC,
            "decoder": {"backend": "ffmpeg", "codec": "wmv2"},
            "counters": {"packets_read": 152, "video_packets": 151, "keyframes": 1,
                         "frames_decoded": 150, "frames_rendered": 150,
                         "render_failures": 0, "read_errors": 0},
            "routing": {"backend_decision": "ffmpeg", "reason": "no video track"},
            "render": {"sink": "anative_window_cpu"},
            "end_reason": "completed",
        }
    if kind == "softdec-visible":
        base = synth("softdec")
        base["counters"]["frames_rendered"] = 150
        return base
    if kind == "mediacodec":
        return {"schema": "kivo.p12c.decode/20",
                "decoder": {"backend": "mediacodec", "codec_name": "OMX.qcom.hevc"}}
    probe = {
        "schema": SCHEMA_PROBE, "ok": True,
        "counters": {"frames_decoded": 30},
        "frames_decoded": 30,
    }
    return probe


def self_test() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        tmpdir = pathlib.Path(tmp)
        for kind in ("softdec", "softdec-visible", "mediacodec", "probe"):
            good = tmpdir / f"p211-{kind}-r1.json"
            good.write_text(json.dumps(synth(kind)), encoding="utf-8")
            rep = Report()
            check_file(good, rep)
            if rep.failures:
                print(f"自检：正例 {kind} 判红——判据写错了：{rep.failures}")
                return 2
        mutations = (
            ("softdec", lambda d: d["decoder"].__setitem__("backend", "mediacodec")),
            ("softdec", lambda d: d["counters"].__setitem__("frames_rendered", 149)),
            ("softdec", lambda d: d["routing"].__setitem__("reason", "")),
            ("softdec", lambda d: d.__setitem__("end_reason", " mysteriously stopped")),
            ("softdec-visible",
             lambda d: d["counters"].__setitem__("render_failures", 5)),
            ("mediacodec", lambda d: d["decoder"].pop("backend")),
            ("probe", lambda d: d.__setitem__("ok", False)),
        )
        for kind, mutate in mutations:
            mutated = json.loads(json.dumps(synth(kind)))
            mutate(mutated)
            bad = tmpdir / f"p211-{kind}-mut.json"
            bad.write_text(json.dumps(mutated), encoding="utf-8")
            rep = Report()
            check_file(bad, rep)
            if not rep.failures:
                print(f"自检：变异未被抓到（{kind}）——判据有洞，exit 2")
                return 2
        print(f"自检：4 正例全绿 + {len(mutations)} 条变异全部被抓到")
    return 0


def main(argv: list[str]) -> int:
    if "--self-test" in argv:
        return self_test()
    roots = [pathlib.Path(a) for a in argv] or [pathlib.Path("docs/evidence/p211")]
    files = sorted(p for root in roots
                   for p in (root.rglob("*.json") if root.is_dir() else [root]))
    if not files:
        print("没有找到证据文件（docs/evidence/p211/*.json）")
        return 1
    total_failures = 0
    kinds: dict[str, int] = {}
    for path in files:
        rep = Report()
        kind = "parse-error"
        try:
            kind = check_file(path, rep)
        except (json.JSONDecodeError, KeyError) as exc:
            rep.check(False, f"{path.name}: 解析失败 {exc}")
        kinds[kind] = kinds.get(kind, 0) + 1
        total_failures += len(rep.failures)
        status = "PASS" if not rep.failures else "FAIL"
        print(f"  {status} {path.name}（{kind}，{rep.checks} 项）")
        for failure in rep.failures:
            print(f"       - {failure}")
    print(f"覆盖：{sum(kinds.values())} 份；分类：{kinds}")
    if total_failures:
        print(f"FAIL：{total_failures} 项")
        return 1
    print("PASS：211 软解兜底证据全部通过")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
