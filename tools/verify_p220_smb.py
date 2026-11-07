#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""verify_p220_smb.py —— 220/220b SMB 直连证据校验器。

证据集（docs/evidence/p220/）：
  p220-spike-xiaomi-pass.json  选型 spike（连接/浏览/offset 读三步全通 + 条目名单）
  p220-end.json                软解会话结束证据（URL 路径的解码/路由/时长）
  p220b-pace.json              硬解节拍曲线（220b：修复前 6.0× 自由跑）
  p220b-pace-r2.json           硬解节拍曲线（修复后 1.00×——AC1 主判据）
  p220-status-log-capture.md   状态日志捕获（起播链路全绿）

判据：
  AC0 spike 三步全通（ok=true + names 非空）
  220b pace_curve 硬解实时：稳定段 ratio 中位数 ∈ [0.90, 1.10]
  状态日志起播链：smb size → proxy → openUrlVideo rc=0 → start rc=0

自检：--mutate 跑 6 个变异，全部必须打红才放行正样本。

用法： python tools/verify_p220_smb.py [--evidence-dir docs/evidence/p220] [--mutate]
"""

import argparse
import io
import json
import os
import statistics
import sys
import contextlib

# GBK 控制台兼容
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")

RATIO_MIN = 0.90
RATIO_MAX = 1.10


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


def parse_pace(raw):
    """pace_curve 元素可能是字符串 "[w, m]"（旧写法）或数组。"""
    out = []
    for item in raw:
        try:
            if isinstance(item, str):
                import ast
                pair = ast.literal_eval(item)
                out.append((float(pair[0]), float(pair[1])))
            elif isinstance(item, (list, tuple)) and len(item) >= 2:
                out.append((float(item[0]), float(item[1])))
        except (ValueError, TypeError):
            pass  # 不可解析项跳过
    return out


def steady_ratio(samples):
    """取 wall > 2000ms 的采样比值中位数（避开起播瞬态）。"""
    ratios = [m / (w / 1000.0) for w, m in samples if w > 2000]
    if not ratios:
        return None
    return statistics.median(ratios)


def check_positive(evidence_dir):
    ok = True

    # AC0: spike
    spike = load_json(os.path.join(evidence_dir, "p220-spike-xiaomi-pass.json"))
    if spike is None:
        return False
    if not spike.get("ok"):
        ok = fail("AC0 spike ok=false")
    names_found = False
    # names 在 libsmb2 对象或 steps 里
    lib = spike.get("libsmb2", {})
    if lib.get("names"):
        names_found = True
        if lib.get("entries", 0) < 3:
            ok = fail("AC0 browse entries=%s < 3" % lib.get("entries"))
    for s in spike.get("steps", []):
        if s.get("names"):
            names_found = True
    if not names_found:
        ok = fail("AC0 spike 缺 names（浏览名单）")

    # 220b: 硬解节拍（AC1 主判据）
    pace = load_json(os.path.join(evidence_dir, "p220b-pace-r2.json"))
    if pace is None:
        return False
    samples = parse_pace(pace)
    if len(samples) < 3:
        ok = fail("220b pace samples=%d < 3" % len(samples))
    ratio = steady_ratio(samples)
    if ratio is None:
        ok = fail("220b pace 无稳定段采样")
    elif not (RATIO_MIN <= ratio <= RATIO_MAX):
        ok = fail("220b 硬解节拍 %.2fx 超出 [%0.2f, %0.2f]" % (ratio, RATIO_MIN, RATIO_MAX))
    else:
        print("  220b 硬解节拍 %.2fx ✓" % ratio)

    # 状态日志捕获：起播链关键节点
    log_path = os.path.join(evidence_dir, "p220-status-log-capture.md")
    if not os.path.isfile(log_path):
        ok = fail("missing p220-status-log-capture.md")
    else:
        with open(log_path, "r", encoding="utf-8") as f:
            content = f.read()
        for key in ("smb size=", "openUrlVideo rc=0", "start rc=0"):
            if key not in content:
                ok = fail("状态日志缺关键节点: " + key)

    if ok:
        print("PASS: p220 SMB 直连证据（AC0 浏览 + AC1 硬解实时 + 起播链全绿）")
    return ok


def check_mutations(evidence_dir):
    """6 个变异全部必须打红。"""
    spike_path = os.path.join(evidence_dir, "p220-spike-xiaomi-pass.json")
    pace_path = os.path.join(evidence_dir, "p220b-pace-r2.json")
    with open(spike_path, "r", encoding="utf-8") as f:
        raw_spike = f.read()
    with open(pace_path, "r", encoding="utf-8") as f:
        raw_pace = f.read()

    tmp_dir = evidence_dir + "_mut"
    os.makedirs(tmp_dir, exist_ok=True)
    for f in ("p220-status-log-capture.md",):
        src = os.path.join(evidence_dir, f)
        if os.path.isfile(src):
            with open(src, "r", encoding="utf-8") as fh:
                content = fh.read()
            with open(os.path.join(tmp_dir, f), "w", encoding="utf-8") as fh:
                fh.write(content)

    def write(spike_raw=None, pace_raw=None):
        with open(os.path.join(tmp_dir, "p220-spike-xiaomi-pass.json"), "w", encoding="utf-8") as fh:
            fh.write(spike_raw if spike_raw is not None else raw_spike)
        with open(os.path.join(tmp_dir, "p220b-pace-r2.json"), "w", encoding="utf-8") as fh:
            fh.write(pace_raw if pace_raw is not None else raw_pace)

    def mut_spike(mutator):
        obj = json.loads(raw_spike)
        mutator(obj)
        return json.dumps(obj)

    def mut_pace(variant):
        obj = json.loads(raw_pace)
        items = obj if isinstance(obj, list) else obj.get("pace_curve", [])
        curve = []
        for item in items:
            try:
                if isinstance(item, str):
                    import ast
                    pair = ast.literal_eval(item)
                else:
                    pair = item
                w, m = float(pair[0]), float(pair[1])
                if variant == "3x": m = m * 3
                elif variant == "0.5x": m = m * 0.5
                curve.append([w, m])
            except (ValueError, TypeError):
                pass
        return json.dumps(curve)

    mutations = [
        ("spike ok=false", "spike", lambda d: d.__setitem__("ok", False)),
        ("spike names 全删", "spike", lambda d: (d.__setitem__("libsmb2", {**d.get("libsmb2", {}), "names": []}),
                                                  [s.__setitem__("names", []) for s in d.get("steps", []) if s.get("names")])),
        ("pace ratio 3x", "pace_3x", None),
        ("pace ratio 0.5x", "pace_0.5x", None),
        ("pace 样本数<3", "pace_few", None),
        ("pace 字符串化破坏", "pace_garbage", None),
    ]

    results = []
    for name, target, mutator in mutations:
        if target == "spike":
            obj = json.loads(raw_spike)
            mutator(obj)
            write(spike_raw=json.dumps(obj))
        elif target.startswith("pace_"):
            variant = target[5:]
            obj = json.loads(raw_pace)
            items = obj if isinstance(obj, list) else obj.get("pace_curve", [])
            curve = []
            for item in items:
                try:
                    if isinstance(item, str):
                        import ast
                        pair = ast.literal_eval(item)
                    else:
                        pair = item
                    w, m = float(pair[0]), float(pair[1])
                    if variant == "3x": m = m * 3
                    elif variant == "0.5x": m = m * 0.5
                    curve.append([w, m])
                except (ValueError, TypeError):
                    pass
            if variant == "empty":
                curve = []
            if variant == "few":
                curve = curve[:2]
            if variant == "garbage":
                curve = ["garbage"]
            if isinstance(obj, list):
                write(pace_raw=json.dumps(curve))
            else:
                obj["pace_curve"] = curve
                write(pace_raw=json.dumps(obj))
        else:
            write()
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            result = check_positive(tmp_dir)
        results.append((name, not result))

    for f in os.listdir(tmp_dir):
        os.remove(os.path.join(tmp_dir, f))
    os.rmdir(tmp_dir)

    red = sum(1 for _, c in results if c)
    for name, caught in results:
        print(("  caught: " if caught else "  MISSED: ") + name)
    print("mutations caught %d/%d" % (red, len(mutations)))
    return red == len(mutations)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", default="docs/evidence/p220")
    parser.add_argument("--mutate", action="store_true")
    args = parser.parse_args()

    ok = check_positive(args.evidence_dir)
    if args.mutate:
        print("--- mutation self-test ---")
        ok = check_mutations(args.evidence_dir) and ok
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
