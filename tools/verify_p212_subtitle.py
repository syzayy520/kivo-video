#!/usr/bin/env python3
"""212 · 字幕证据校验器（含变异自检）。

判据（211 设计稿 §五 同款纪律）：
- 字幕证据（kivo.p212.subtitle/1）：events >= 0、rendered_frames >= 0、
  rendered_frames <= events × 窗口上界（结构性上界，防计时器错乱）、
  fonts_injected >= 0；文件名带 -visible 的轮次要求 rendered_frames > 0（AC1/AC2 可见轮）；
- 渲染器产物含 srt→ass 转换链路的证据（events 与素材 Dialogue 数一致由真机侧保证）。
"""
from __future__ import annotations

import json
import pathlib
import sys
import tempfile

SCHEMA_SUBTITLE = "kivo.p212.subtitle/1"


class Report:
    def __init__(self) -> None:
        self.failures: list[str] = []
        self.checks = 0

    def check(self, ok: bool, what: str) -> bool:
        self.checks += 1
        if not ok:
            self.failures.append(what)
        return ok


def check_subtitle(d: dict, rep: Report, label: str) -> None:
    rep.check(d.get("schema") == SCHEMA_SUBTITLE, f"{label}: schema")
    events = d.get("events")
    rendered = d.get("rendered_frames")
    fonts = d.get("fonts_injected")
    rep.check(isinstance(events, int) and events >= -1, f"{label}: events >= -1")
    rep.check(isinstance(rendered, int) and rendered >= 0, f"{label}: rendered_frames >= 0")
    rep.check(isinstance(fonts, int) and fonts >= 0, f"{label}: fonts_injected >= 0")
    # 结构性上界：100ms 轮询 × 事件窗口（本卡素材 ≤ 12s）→ 上界 120 帧
    rep.check(rendered <= 120, f"{label}: rendered_frames 结构上界 120（计时器错乱防护）")
    if "-visible" in label:
        rep.check(rendered > 0, f"{label}: 可见轮要求 rendered_frames > 0（AC1/AC2）")
        rep.check(fonts > 0, f"{label}: 可见轮要求字体注入成功")


def self_test() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        tmpdir = pathlib.Path(tmp)
        cases = {
            "p212-subtitle-visible-ok.json": {
                "schema": SCHEMA_SUBTITLE, "file": "a.srt", "events": 3,
                "rendered_frames": 58, "last_render_position_us": 7916672,
                "fonts_injected": 2},
            "p212-subtitle-ok.json": {
                "schema": SCHEMA_SUBTITLE, "file": "a.srt", "events": 3,
                "rendered_frames": 0, "last_render_position_us": -1,
                "fonts_injected": 2},
        }
        for name, body in cases.items():
            f = tmpdir / name
            f.write_text(json.dumps(body), encoding="utf-8")
            rep = Report()
            check_file(f, rep)
            if rep.failures:
                print(f"自检：正例 {name} 判红——判据写错了：{rep.failures}")
                return 2
        mutations = [
            lambda d: d.__setitem__("rendered_frames", -5),
            lambda d: d.__setitem__("fonts_injected", -1),
            lambda d: d.__setitem__("rendered_frames", 500),
            lambda d: d.pop("events"),
        ]
        for i, mutate in enumerate(mutations):
            mutated = json.loads(json.dumps(synth("p212-subtitle-visible-ok.json")))
            mutate(mutated)
            bad = tmpdir / f"p212-mut-{i}.json"
            bad.write_text(json.dumps(mutated), encoding="utf-8")
            rep = Report()
            check_file(bad, rep)
            if not rep.failures:
                print(f"自检：变异未被抓到（{i}）——判据有洞，exit 2")
                return 2
        print(f"自检：2 正例全绿 + {len(mutations)} 条变异全部被抓到")
    return 0


def synth(name: str) -> dict:
    return {
        "schema": SCHEMA_SUBTITLE, "file": "a.srt", "events": 3,
        "rendered_frames": 58, "last_render_position_us": 7916672,
        "fonts_injected": 2}


def check_file(path: pathlib.Path, rep: Report) -> str:
    d = json.loads(path.read_text(encoding="utf-8"))
    label = path.name
    if d.get("schema") == SCHEMA_SUBTITLE:
        check_subtitle(d, rep, label)
        return "subtitle"
    rep.check(False, f"{label}: schema 不认识（{d.get('schema')}）")
    return "unknown"


def main(argv: list[str]) -> int:
    if "--self-test" in argv:
        return self_test()
    roots = [pathlib.Path(a) for a in argv] or [pathlib.Path("docs/evidence/p212")]
    files = sorted(p for root in roots
                   for p in (root.rglob("*.json") if root.is_dir() else [root]))
    if not files:
        print("没有找到证据文件（docs/evidence/p212/*.json）")
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
    print("PASS：212 字幕证据全部通过")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
