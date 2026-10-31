#!/usr/bin/env python3
"""P14-H：守卫知道"过了多久"之后的行为校验。

本卡只改一个判据（`advance > threshold` → `advance > threshold + elapsed`），
所以判据只盯一件事：**钳制是否只发生在"位置真的跑在墙钟前面"的时候**。

修好之后，钳制的充分必要条件就是「残差越阈值」
（`residual = 候选步长 − 观测间隔`，而观测间隔正是喂给守卫的 `elapsed`）。
于是 P14-G 里那条被判红的"原始判据"（`beyond_threshold >= jumps_clamped`）
现在**成立**了 —— 它当年红，不是判据错，是实现错。

H1 注入不进守卫  这条是**发现**，不是回归：`ClockOffsetSource` 包在
               `AaudioClockSource` **外面**（`jni_session_handle.cxx:341`），
               台阶加在守卫**下游**，守卫看到的 `candidate` 里根本没有它。
               所以注入轮的 `beyond_threshold` 必须是 0。
               第一版判据写的是"注入必须触发钳制"，被这条真机结果判红 ——
               红得对：**那是判据建错了地方，不是守卫修坏了。**
               "真跳变仍被钳制"由桌面单测 C25 / C27 保证（225.5 ms / 0.5 ms 间隔
               与 1 s / 100 ms 间隔都必须钳制，且只放行 threshold + elapsed）。
H2 无注入不钳制   `offset_ns == 0` → `jumps_clamped == 0`。
               P14-G 的 121 分钟证明这段时间里没有真跳变（最大残差 44 ms），
               任何一次钳制都只能是"我们没看"造成的假案。
H3 钳制与残差对齐 `beyond_threshold >= jumps_clamped`（对全部轮次）。
H4 间隔仍要记账  `max_mono_step_ns >= 阈值` 时 `long_gaps >= 1`：不钳制 ≠ 不记录。

用法：
    python tools/verify_p14h_guard.py docs/evidence/p14h/*-sync.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path


class Failure(Exception):
    pass


def verify(path: Path) -> str:
    document = json.loads(path.read_text(encoding="utf-8"))
    clock = document["audio"]["clock"]
    tail = clock["tail"]
    injection = document["clock_injection"]

    offset_ns = int(injection["offset_ns"])
    clamps = int(clock["jumps_clamped"])
    beyond = int(tail["beyond_threshold"])
    long_gaps = int(tail["long_gaps"])
    max_step = int(tail["max_mono_step_ns"])
    threshold = int(tail["clamp_threshold_ns"])

    # ── H1：注入的台阶**不该**出现在守卫的视野里（它是下游的事）──
    if offset_ns > 0:
        if beyond > 0:
            raise Failure(
                f"H1 注入 {offset_ns / 1e6:.0f} ms 的轮次出现了 {beyond} 个越阈值残差 —— "
                "如果注入真的进到了守卫，说明拓扑变了，本卡的结论要重算"
            )
        if clamps > 0:
            raise Failure(
                f"H1 注入轮钳制了 {clamps} 次 —— 台阶在守卫下游，这一次只能来自"
                "真实长间隔，而那已经不该再钳（见 H2）"
            )

    # ── H2：没有注入就不该有钳制 ──
    if offset_ns == 0 and clamps > 0:
        raise Failure(
            f"H2 没有注入任何台阶，却钳制了 {clamps} 次 —— "
            f"最长间隔 {max_step / 1e6:.0f} ms 已被 elapsed 解释，不该再钳"
        )

    # ── H3：钳制与残差对齐（这就是 P14-G 里被判红的那条）──
    if beyond < clamps:
        raise Failure(
            f"H3 钳制 {clamps} 次，但越阈值残差只有 {beyond} 个 —— "
            "两者不等说明 elapsed 与 tail 的间隔不是同一个来源"
        )

    # ── H4：不钳制不代表不记录 ──
    if max_step >= threshold and long_gaps < 1:
        raise Failure(
            f"H4 最长间隔 {max_step / 1e6:.0f} ms 已越阈值，long_gaps 却是 0"
        )

    duration_s = float(document["source"]["duration_us"]) / 1e6
    return (
        f"{path.name}: OK  inject={offset_ns / 1e6:.0f}ms clamps={clamps} "
        f"beyond={beyond} gaps={long_gaps}/{max_step / 1e6:.0f}ms "
        f"dur={duration_s:.0f}s dropped={document['frames_dropped']} "
        f"p99={document['pace']['lateness_ns']['p99'] / 1e6:.2f} ms"
    )


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    exit_code = 0
    for raw in argv[1:]:
        path = Path(raw)
        try:
            print(verify(path))
        except Failure as failure:
            print(f"{path.name}: FAIL  {failure}")
            exit_code = 1
        except (KeyError, ValueError, TypeError) as error:
            print(f"{path.name}: FAIL  结构不对：{error!r}")
            exit_code = 1
    return exit_code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
