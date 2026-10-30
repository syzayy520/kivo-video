#!/usr/bin/env python3
"""P12-D5 上屏时刻探针：从 `dumpsys SurfaceFlinger --latency` 读每帧的(请求, 实际)上屏时刻。

用法：
    python tools/probe_present_timing.py --present surface_at_time --tag idle-r1 \
        --out docs/evidence/p12d5/p12d5-idle-r1.json

三条硬规矩（都是踩出来的，见 33-p12d5-design.md）：

1. **层名必须带 `#id`。** 少了它 `--latency` 只返回刷新率那一行，不报错、不提示 ——
   C7 就是栽在这里，把"能测"当成了"不可测"。本脚本从 `--list` 现取，取不到直接退出。
2. **只在应用活着的时候 dump。** 层一销毁统计就没了。
3. **`cpu_paced` 下不算 Δ。** 那时第一列是媒体 pts，减单调时钟得到的是无意义的数；
   脚本会显式拒绝，并把原因写进产物。
"""
from __future__ import annotations

import argparse
import json
import re
import statistics
import subprocess
import sys
import time
from pathlib import Path

SCHEMA = "kivo.p12d5.present-timing/1"

# 第一列超过这个量级就认为它是单调时钟（秒级 epoch 才有的量级），
# 否则是媒体 pts（几十秒量级）。K40 上二者差 4 个数量级，不会误判。
MONOTONIC_THRESHOLD_NS = 10**14

# 起播瞬态窗口：**起播后前 0.5 秒**（30 fps 下 15 帧）不计入稳态。
#
# 这个数改过一次：最初写 3，后来有一轮观察到前 **9 帧**全部晚 ~25 ms
# （Surface 刚建、解码器刚出第一帧、首批缓冲在排队），3 帧盖不住。
# 改成 15 是在看过那一轮之后做的 —— 方向是"排掉更多"，会**让稳态数字变好**，
# 所以必须在这里写明，并且把瞬态组单独报出来（`startup_*`），
# 不许只留一个漂亮的稳态数字。
STARTUP_ROWS = 15


class Failure(RuntimeError):
    pass


def run(adb: str, args: list[str], timeout: float = 30.0) -> str:
    proc = subprocess.run([adb, "shell", *args], capture_output=True, text=False, timeout=timeout)
    return proc.stdout.decode("utf-8", errors="replace").replace("\r", "")


def shell_quote(text: str) -> str:
    return "'" + text.replace("'", "'\\''") + "'"


def cpu_idle_percent(adb: str) -> float | None:
    """读整机空闲率。**这是标签的依据，不是装饰。**

    `--load none` 只是"我没起负载"，不等于"机器空着"。
    实测过一次：以为停掉的 5+16 个忙循环一直在跑（`TaskStop` 只杀了本地 adb 客户端，
    远端 shell 还在），于是标着"空载"的三轮其实是满载跑的。
    所以每轮都把实测空闲率写进证据，并且标签与实测不符时**大声报警**。
    """
    for line in run(adb, ["top", "-bn1"]).splitlines():
        if "%idle" not in line:
            continue
        tokens = line.split()
        # 行首是 `800%cpu`（本机 8 核），空闲数与它同单位。
        # **必须先除再存**：第一版直接把 777.0 当百分比写进证据，
        # 读的人只会看到"777% 空闲"这种不可能的数。
        total = None
        if tokens and tokens[0].endswith("%cpu"):
            try:
                total = float(tokens[0][: -len("%cpu")])
            except ValueError:
                total = None
        for token in tokens:
            if token.endswith("%idle"):
                try:
                    idle = float(token[: -len("%idle")])
                except ValueError:
                    return None
                if total and total > 0:
                    return round(idle / total * 100.0, 1)
                return idle
    return None


def find_layer(adb: str, package: str) -> str:
    """现取 BLAST SurfaceView 层的完整名字（含 `#id`）。取不到就退出，不回落。"""
    listing = run(adb, ["dumpsys", "SurfaceFlinger", "--list"])
    pattern = re.compile(rf"^SurfaceView\[{re.escape(package)}/.*\]\(BLAST\)#\d+$")
    for line in listing.splitlines():
        line = line.strip()
        if pattern.match(line):
            return line
    raise Failure(
        f"没找到 {package} 的 BLAST SurfaceView 层。\n"
        f"`--list` 里与包名相关的行：\n"
        + "\n".join(l for l in listing.splitlines() if package in l)
    )


def dump_latency(adb: str, layer: str) -> tuple[int, list[tuple[int, int, int]]]:
    # **必须加引号。** 层名里有 `#`、`(`、`)`：`#` 在 shell 里是注释起点，
    # 括号会直接语法错误 —— 命令不报错、只返回空，和"层不存在"长得一模一样。
    # 手工试的时候我加了引号所以能跑，脚本里忘了加，于是第一次就撞上。
    raw = run(adb, ["dumpsys SurfaceFlinger --latency " + shell_quote(layer)])
    lines = [l for l in raw.splitlines() if l.strip()]
    if not lines:
        raise Failure("--latency 没有任何输出")
    try:
        refresh_ns = int(lines[0].strip())
    except ValueError as exc:
        raise Failure(f"首行不是刷新周期：{lines[0]!r}") from exc

    rows: list[tuple[int, int, int]] = []
    for line in lines[1:]:
        parts = line.split()
        if len(parts) != 3:
            continue
        try:
            desired, actual, ready = (int(p) for p in parts)
        except ValueError:
            continue
        if desired == 0 and actual == 0 and ready == 0:
            continue  # 环形缓冲里还没被写过的空槽
        rows.append((desired, actual, ready))
    return refresh_ns, rows


def summarize(refresh_ns: int, rows: list[tuple[int, int, int]]) -> dict:
    """把原始行分成三类再算，**不许混在一起**。

    - `scheduled`：desired 是单调时钟量级 → 可以算 Δ；
    - `pending`：actual 是 `INT64_MAX` 哨兵 —— 这一帧已经提交、合成器**还没锁存**，
      它不是"迟到 292 年"，是"还没发生"。混进来会把 max 直接毁掉
      （第一次跑就撞上了：max 报出 9222946887098 ms）；
    - `unscheduled`：desired 是媒体 pts —— 这一帧提交时没带时间戳
      （`cpu_paced` 的常态，或 `surface_at_time` 起播头几帧）。

    最后一类如果非空，说明这段窗口里混了两种提交方式，**必须单独报出来**：
    用它算 Δ 会得到一个物理上不存在的数。
    """
    int64_max = 2**63 - 1
    scheduled: list[tuple[int, int]] = []
    pending = 0
    unscheduled = 0
    for desired, actual, _ready in rows:
        if actual >= int64_max - 1_000_000:
            pending += 1
            continue
        if desired < MONOTONIC_THRESHOLD_NS:
            unscheduled += 1
            continue
        scheduled.append((desired, actual))

    out: dict = {
        "samples": len(rows),
        "scheduled_samples": len(scheduled),
        "pending_samples": pending,
        "unscheduled_samples": unscheduled,
        "refresh_period_ns": refresh_ns,
    }
    if not scheduled:
        out["delta_unavailable_reason"] = (
            "窗口内没有一帧的 desiredPresentTime 是单调时钟："
            "cpu_paced 提交不带时间戳（第一列是媒体 pts），与 actual 不同源，相减没有物理意义"
        )
        return out

    # **按时间顺序**算一遍，再排序取分位数。
    # 这里犯过一次错：先 sorted 再切前三项，丢的是"最小的三个 Δ"，
    # 不是"起播头三帧" —— 等于把下尾削掉，稳态数字会往差里偏。
    chrono = [a - d for d, a in scheduled]
    deltas = sorted(chrono)
    out["delta_ns"] = quantiles(deltas)
    out["vsync_missed_frames"] = sum(1 for d in deltas if d >= refresh_ns)

    # 起播瞬态与稳态分开报，两组都要，不许只留一组。
    startup = sorted(chrono[:STARTUP_ROWS])
    steady = sorted(chrono[STARTUP_ROWS:])
    if startup:
        out["startup_delta_ns"] = quantiles(startup)
        out["startup_vsync_missed_frames"] = sum(1 for d in startup if d >= refresh_ns)
    if steady:
        out["steady_delta_ns"] = quantiles(steady)
        out["steady_vsync_missed_frames"] = sum(1 for d in steady if d >= refresh_ns)
        out["startup_rows_excluded"] = STARTUP_ROWS

    if len(scheduled) > 2:
        request_times = [d for d, _ in scheduled]
        gaps = sorted(b - a for a, b in zip(request_times, request_times[1:]))
        out["request_interval_ns"] = quantiles(gaps)
    return out


def quantiles(sorted_values: list[int]) -> dict:
    return {
        "p01": quantile(sorted_values, 0.01),
        "p10": quantile(sorted_values, 0.10),
        "p50": quantile(sorted_values, 0.50),
        "p90": quantile(sorted_values, 0.90),
        "p99": quantile(sorted_values, 0.99),
        "min": sorted_values[0],
        "max": sorted_values[-1],
    }


def quantile(sorted_values: list[int], q: float) -> int:
    """最近秩分位数。和项目里 C++ 直方图的口径一致：取第 ceil(q*n) 个。"""
    if not sorted_values:
        return 0
    import math

    rank = max(1, math.ceil(q * len(sorted_values)))
    return sorted_values[min(rank, len(sorted_values)) - 1]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--adb", default="adb")
    parser.add_argument("--package", default="com.kivo.cinema")
    parser.add_argument("--activity", default=".ui.PlayerActivity")
    parser.add_argument("--media", required=True, help="设备上的片源路径")
    parser.add_argument("--present", default="surface_at_time",
                        choices=["surface_at_time", "cpu_paced"])
    parser.add_argument("--fps", type=int, default=30)
    parser.add_argument("--settle", type=float, default=4.0,
                        help="起播后等多久再 dump（环形缓冲 128 行 ≈ 4.3s@30fps，别等太久）")
    parser.add_argument("--load", default="none", help="标签：none / busy5 …（只记录，不施加）")
    parser.add_argument("--tag", required=True, help="产物文件名前缀，例如 idle-r1")
    parser.add_argument("--out", required=True)
    parser.add_argument("--evidence", default=None,
                        help="同时让播放器写一份 decode 证据（会给它 kivo.evidence）")
    parser.add_argument("--stall-ms", type=float, default=0.0,
                        help="正向对照：窗口正中间把应用进程 SIGSTOP 这么久再 SIGCONT。"
                             "用途是证明这个量**确实会动** —— 探针对刺激没反应，"
                             "就没资格说'负载下不变'")
    parser.add_argument("--await-evidence-seconds", type=float, default=0.0,
                        help="dump 之后等多少秒再把应用侧证据拉回来。"
                             ">0 时会一直等到片源播完（片源越长等越久）。"
                             "用途：交叉核对'负载下是画面变晚，还是帧被丢掉'")
    args = parser.parse_args()

    package = args.package
    evidence_name = args.evidence or f"{args.tag}.json"

    run(args.adb, ["am", "force-stop", package])
    # force-stop 之后要等一会儿再读空闲率：上一轮的 codec / AAudio 还在释放，
    # 立刻读会把"上一轮的尾巴"记成"这一轮的负载"（实测读到过 66.8%，
    # 而同一轮 dump 时是 93.8%）。空闲率必须反映**本轮**。
    time.sleep(3.0)
    idle_before = cpu_idle_percent(args.adb)

    # 清空所有层的统计：不清就是"上一次运行 + 这一次"混在一起。
    run(args.adb, ["dumpsys", "SurfaceFlinger", "--latency-clear"])
    cleared_at = time.time()

    extras = [
        "am", "start", "-n", f"{package}/{args.activity}",
        "--es", "kivo.media", args.media,
        "--ei", "kivo.pace", "1",
        "--ei", "kivo.drop", "1",
        "--ei", "kivo.fps", str(args.fps),
        "--es", "kivo.present", args.present,
        "--es", "kivo.evidence", evidence_name,
    ]
    if args.load == "none" and idle_before is not None and idle_before < 85.0:
        print(f"[{args.tag}] ⚠️ 标着空载，但实测空闲率只有 {idle_before:.1f}% —— "
              f"机器上有别的东西在吃 CPU，这一轮的 load 标签不可信", file=sys.stderr)

    run(args.adb, extras, timeout=60)

    # 等层出现（Surface 创建 + 首帧）。
    layer = ""
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            layer = find_layer(args.adb, package)
            break
        except Failure:
            time.sleep(0.3)
    if not layer:
        raise Failure("10 秒内没等到 SurfaceView 层出现")

    if args.stall_ms > 0:
        # 正向对照。冻的是**整个进程**（解码线程也在里面），所以这几帧的提交
        # 一定会比 due_at 晚 —— Δ 应当跳出 ~stall_ms 的尖峰。
        # 没这个对照，"负载下 Δ 不变"就只是"探针没反应"的另一种说法。
        time.sleep(args.settle / 2)
        pid = run(args.adb, ["pidof", package]).strip()
        if not pid:
            raise Failure("拿不到 pid，无法注入停顿")
        run(args.adb, ["run-as", package, "kill", "-STOP", pid])
        time.sleep(args.stall_ms / 1000.0)
        run(args.adb, ["run-as", package, "kill", "-CONT", pid])
        print(f"[{args.tag}] 已注入停顿 {args.stall_ms:.0f}ms（pid={pid}）")
        time.sleep(args.settle / 2)
    else:
        time.sleep(args.settle)

    dumped_at = time.time()
    idle_after = cpu_idle_percent(args.adb)
    refresh_ns, rows = dump_latency(args.adb, layer)
    summary = summarize(refresh_ns, rows)

    payload = {
        "schema": SCHEMA,
        "tag": args.tag,
        "captured_at_host_ms": int(dumped_at * 1000),
        "present_schedule": args.present,
        "load": args.load,
        # 实测空闲率：标签的依据。两者不符时以实测为准。
        "cpu_idle_before_percent": idle_before,
        "cpu_idle_after_percent": idle_after,
        "media": args.media,
        "fps_target": args.fps,
        "layer": layer,
        "clear_to_dump_ms": int((dumped_at - cleared_at) * 1000),
        "refresh_period_ns": refresh_ns,
        "summary": summary,
        "rows": [{"desired": d, "actual": a, "ready": r} for d, a, r in rows],
        "app_evidence": evidence_name,
    }

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    print(f"[{args.tag}] layer={layer} idle={idle_before}/{idle_after}%")
    print(f"[{args.tag}] rows={summary['samples']} refresh={refresh_ns/1e6:.2f}ms")
    if "startup_vsync_missed_frames" in summary:
        st = summary.get("startup_delta_ns", {})
        print(f"[{args.tag}] 瞬态({STARTUP_ROWS} 帧) p50={st.get('p50', 0)/1e6:.3f} "
              f"max={st.get('max', 0)/1e6:.3f} ms miss={summary['startup_vsync_missed_frames']}")
    print(f"[{args.tag}] scheduled={summary['scheduled_samples']} "
          f"pending={summary['pending_samples']} unscheduled={summary['unscheduled_samples']}")
    if "delta_ns" in summary:
        d = summary["delta_ns"]
        s2 = summary.get("steady_delta_ns", d)
        print(f"[{args.tag}] Δ 全窗 p50={d['p50']/1e6:.3f} p99={d['p99']/1e6:.3f} "
              f"max={d['max']/1e6:.3f} ms | 稳态 p50={s2['p50']/1e6:.3f} "
              f"p99={s2['p99']/1e6:.3f} max={s2['max']/1e6:.3f} ms | "
              f"vsync_miss={summary['vsync_missed_frames']}"
              f"(稳态 {summary.get('steady_vsync_missed_frames', 0)})")
    else:
        print(f"[{args.tag}] Δ 不可算：{summary['delta_unavailable_reason']}")
    print(f"[{args.tag}] -> {out}")

    if args.await_evidence_seconds > 0:
        deadline = time.time() + args.await_evidence_seconds
        pulled = None
        while time.time() < deadline:
            probe = subprocess.run(
                [args.adb, "shell", "run-as", package, "ls",
                 f"files/evidence/{evidence_name}"],
                capture_output=True)
            listing = probe.stdout.decode("utf-8", errors="replace").strip()
            if listing and "No such file" not in listing:
                raw = subprocess.run(
                    [args.adb, "shell", "run-as", package, "cat",
                     f"files/evidence/{evidence_name}"],
                    capture_output=True).stdout.decode("utf-8", errors="replace")
                if raw.strip().startswith("{"):
                    pulled = json.loads(raw)
                    break
            time.sleep(1.0)
        if pulled is None:
            print(f"[{args.tag}] 应用侧证据没等到（{args.await_evidence_seconds}s）", file=sys.stderr)
        else:
            counters = pulled.get("counters", {})
            print(f"[{args.tag}] 应用侧 recv={counters.get('frames_received')} "
                  f"rend={counters.get('frames_rendered')} "
                  f"drop={counters.get('frames_dropped_late')} "
                  f"completed={pulled.get('outcome', {}).get('completed')}")
            payload["app_counters"] = {
                "frames_received": counters.get("frames_received"),
                "frames_rendered": counters.get("frames_rendered"),
                "frames_dropped_late": counters.get("frames_dropped_late"),
                "frames_discarded_no_surface": counters.get("frames_discarded_no_surface"),
                "completed": pulled.get("outcome", {}).get("completed"),
                "error": pulled.get("outcome", {}).get("error"),
            }
            out.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Failure as exc:
        print(f"失败：{exc}", file=sys.stderr)
        sys.exit(2)
