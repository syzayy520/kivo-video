#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""verify_p215_thumbnail.py —— 215 进度条缩略图证据校验器。

证据集（docs/evidence/p215/）：
  p215-thumb.json           缩略图读取器取证（AC1 延迟 / AC3 LRU）
  p215-stats-baseline.json  主链路扫描前 writeStats 快照（AC2）
  p215-stats-stress.json    主链路扫描后 writeStats 快照（AC2）
  p215-drag-k40.png         拖动预览浮窗真机截图（视觉证据）

判据（215 设计稿 §四）：
  AC1 冷启动首现（含解码器 init）≤ 500 ms；warm p95 / max ≤ 500 ms
  AC2 扫描窗口内主链路丢帧不回退：dropped_late / pace.dropped 全程 0，
      且 frames_received 有增量（主链路一直在播）
  AC3 LRU：>50 个不同位置请求 → 淘汰计数 >0 且 cache_size_final ≤ cache_max

自检：--mutate 跑 8 个变异，全部必须打红才放行正样本。

用法： python tools/verify_p215_thumbnail.py [--evidence-dir docs/evidence/p215]
"""

import argparse
import json
import os
import sys

MAX_FIRST_LATENCY_MS = 500
MAX_WARM_LATENCY_MS = 500
MIN_REQUESTS = 61          # 60 点扫描 + 1 cache-hit 探针
MAX_DECODE_FAILURES = 1    # 允许 EOS 边界点（59.5s 末点）失败一次


def fail(msg):
    print("FAIL: " + msg)
    return False


def load_json(path):
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        fail("cannot read " + path + ": " + str(e))
        return None


def check_positive(evidence_dir):
    ok = True
    thumb = load_json(os.path.join(evidence_dir, "p215-thumb.json"))
    base = load_json(os.path.join(evidence_dir, "p215-stats-baseline.json"))
    stress = load_json(os.path.join(evidence_dir, "p215-stats-stress.json"))
    if thumb is None or base is None or stress is None:
        return False
    t = thumb.get("thumbnail", {})
    meta = thumb.get("meta", {})

    # AC1
    cold = t.get("cold_first_latency_ms", -1)
    if not (0 <= cold <= MAX_FIRST_LATENCY_MS):
        ok = fail("AC1 cold_first_latency_ms=%s > %d" % (cold, MAX_FIRST_LATENCY_MS))
    for key in ("warm_p95_ms", "warm_max_ms"):
        v = t.get(key, -1)
        if not (0 <= v <= MAX_WARM_LATENCY_MS):
            ok = fail("AC1 %s=%s > %d" % (key, v, MAX_WARM_LATENCY_MS))
    if t.get("warm_count", 0) < MIN_REQUESTS - 1 - MAX_DECODE_FAILURES:
        ok = fail("AC1 warm_count=%s too small (expect >= %d)"
                  % (t.get("warm_count"), MIN_REQUESTS - 1 - MAX_DECODE_FAILURES))

    # AC3
    if t.get("requests", 0) < MIN_REQUESTS:
        ok = fail("AC3 requests=%s < %d" % (t.get("requests"), MIN_REQUESTS))
    if t.get("evictions", 0) <= 0:
        ok = fail("AC3 evictions=%s must be > 0 (>50 distinct positions)" % t.get("evictions"))
    if t.get("cache_size_final", 0) > t.get("cache_max", 0):
        ok = fail("AC3 cache_size_final=%s > cache_max=%s (unbounded growth)"
                  % (t.get("cache_size_final"), t.get("cache_max")))
    if t.get("cache_hits", 0) < 1:
        ok = fail("AC3 cache_hits=%s must be >= 1 (repeat probe)" % t.get("cache_hits"))
    if t.get("decode_failures", 0) > MAX_DECODE_FAILURES:
        ok = fail("decode_failures=%s > %d" % (t.get("decode_failures"), MAX_DECODE_FAILURES))

    # AC2（主链路）
    def flat(d, pre=""):
        out = {}
        for k, v in d.items():
            if isinstance(v, dict):
                out.update(flat(v, pre + k + "."))
            else:
                out[pre + k] = v
        return out
    fb, fs = flat(base), flat(stress)
    for key in ("counters.frames_dropped_late", "pace.dropped"):
        if fs.get(key) != 0:
            ok = fail("AC2 %s=%s must be 0 (main path zero interference)" % (key, fs.get(key)))
    recv_b, recv_s = fb.get("counters.frames_received"), fs.get("counters.frames_received")
    if not (isinstance(recv_b, int) and isinstance(recv_s, int) and recv_s > recv_b):
        ok = fail("AC2 frames_received %s -> %s: main path not advancing" % (recv_b, recv_s))
    rel_b, rel_s = fb.get("counters.frames_released"), fs.get("counters.frames_released")
    if isinstance(rel_b, int) and isinstance(rel_s, int) and rel_s - rel_b != recv_s - recv_b:
        ok = fail("AC2 released delta != received delta (%s vs %s) — frames dropped"
                  % (rel_s - rel_b, recv_s - recv_b))

    # 截图（视觉证据）
    shot = os.path.join(evidence_dir, "p215-drag-k40.png")
    if not os.path.isfile(shot):
        ok = fail("missing drag-preview screenshot " + shot)
    else:
        with open(shot, "rb") as f:
            head = f.read(24)
        if head[:8] != b"\x89PNG\r\n\x1a\n":
            ok = fail("screenshot is not a PNG")

    # 元数据
    if not meta.get("device_model"):
        ok = fail("meta.device_model missing")
    if "1080p" not in str(meta.get("media", "")):
        print("WARN: media is not the 1080p reference file: %s" % meta.get("media"))

    if ok:
        print("PASS: p215 thumbnail evidence")
        print("  AC1 cold_first=%dms warm_p95=%dms (budget 500ms)"
              % (cold, t.get("warm_p95_ms", -1)))
        print("  AC2 dropped=0, frames %d -> %d during scan" % (recv_b, recv_s))
        print("  AC3 requests=%d evictions=%d cache_final=%d/%d"
              % (t.get("requests"), t.get("evictions"),
                 t.get("cache_size_final"), t.get("cache_max")))
    return ok


def check_mutations(evidence_dir):
    """8 个变异必须全部打红（防校验器静默放过坏数据）。"""
    with open(os.path.join(evidence_dir, "p215-thumb.json"), "r", encoding="utf-8") as f:
        raw = f.read()
    tmp_dir = evidence_dir + "_mut"
    os.makedirs(tmp_dir, exist_ok=True)
    with open(os.path.join(evidence_dir, "p215-stats-baseline.json"), "r", encoding="utf-8") as f:
        raw_base = f.read()
    with open(os.path.join(evidence_dir, "p215-stats-stress.json"), "r", encoding="utf-8") as f:
        raw_stress = f.read()

    def make_thumb(mutator):
        obj = json.loads(raw)
        mutator(obj)
        return json.dumps(obj)

    def make_stress(mutator):
        obj = json.loads(raw_stress)
        mutator(obj)
        return json.dumps(obj)

    def write(dirpath, thumb_raw=None, stress_raw=None, with_shot=True):
        with open(os.path.join(dirpath, "p215-thumb.json"), "w", encoding="utf-8") as f:
            f.write(thumb_raw if thumb_raw is not None else raw)
        with open(os.path.join(dirpath, "p215-stats-baseline.json"), "w", encoding="utf-8") as f:
            f.write(raw_base)
        with open(os.path.join(dirpath, "p215-stats-stress.json"), "w", encoding="utf-8") as f:
            f.write(stress_raw if stress_raw is not None else raw_stress)
        shot = os.path.join(dirpath, "p215-drag-k40.png")
        if with_shot:
            with open(shot, "wb") as f:
                f.write(b"\x89PNG\r\n\x1a\n" + b"0" * 16)
        elif os.path.exists(shot):
            os.remove(shot)

    def setk(root, path, value):
        cur = root
        keys = path.split(".")
        for k in keys[:-1]:
            cur = cur[k]
        cur[keys[-1]] = value

    mutations = [
        ("cold 600ms", lambda d: setk(d, "thumbnail.cold_first_latency_ms", 600), None, True),
        ("warm_p95 600ms", lambda d: setk(d, "thumbnail.warm_p95_ms", 600), None, True),
        ("requests 10", lambda d: setk(d, "thumbnail.requests", 10), None, True),
        ("evictions 0", lambda d: setk(d, "thumbnail.evictions", 0), None, True),
        ("cache over cap", lambda d: setk(d, "thumbnail.cache_size_final", 51), None, True),
        ("decode_failures 5", lambda d: setk(d, "thumbnail.decode_failures", 5), None, True),
        ("main dropped", None,
         lambda d: setk(d, "counters.frames_dropped_late", 3), True),
        ("missing screenshot", None, None, False),
    ]

    results = []
    for name, tmut, smut, with_shot in mutations:
        write(tmp_dir,
              make_thumb(tmut) if tmut else None,
              make_stress(smut) if smut else None,
              with_shot)
        # check_positive 内部用 print 输出 FAIL —— 捕获返回值
        caught = not check_positive_quiet(tmp_dir)
        results.append((name, caught))

    # 清理
    for f in os.listdir(tmp_dir):
        os.remove(os.path.join(tmp_dir, f))
    os.rmdir(tmp_dir)

    red = sum(1 for _, c in results if c)
    for name, caught in results:
        print(("  caught: " if caught else "  MISSED: ") + name)
    print("mutations caught %d/8" % red)
    return red == len(mutations)


def check_positive_quiet(evidence_dir):
    import io
    import contextlib
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        return check_positive(evidence_dir)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", default="docs/evidence/p215")
    parser.add_argument("--mutate", action="store_true",
                        help="跑变异自检（8 个全部打红才算校验器可用）")
    args = parser.parse_args()

    ok = check_positive(args.evidence_dir)
    if args.mutate:
        print("--- mutation self-test ---")
        ok = check_mutations(args.evidence_dir) and ok
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
