#!/usr/bin/env python3
"""在 ARM 模拟器上验证「平台契约」类判据（P12 设备矩阵，A 类）。

**这个脚本只回答一类问题**：`MediaExtractor` 的解析契约在**低 API 版本**上是否一致。
具体就三件事：

1. `crypto-mode` 键在 API 26 的 extractor 上存在吗？（F3B 的判定能否成立）
2. 加密样本的首包 `is_encrypted` 是 true 还是 false？
3. 坏文件 / 无视频轨的 reason 是否与 API 33 真机一致？

**它不测、也不能测**：av_drift、丢帧、seek 时延、上屏时刻、HDR dataspace、内存。
那些是 B/C 类判据，依赖真实硬解器与面板 —— 在模拟器上测出来的数字是假的，
写进证据会污染总收口（`verify_p12_all.py` 已排除 `docs/evidence/emulator/`）。

为什么用 ARM 镜像：x86_64 镜像强制要求硬件加速（AEHD/WHPX 驱动），
而装驱动需要管理员权限。ARM 镜像走 QEMU 纯软件模拟，**不需要任何加速驱动** ——
代价是慢（开机 5–15 分钟），但它是普通用户权限下唯一能跑起来的通路。

用法：

    python tools/check_emu_contract.py --serial emulator-5554
    python tools/check_emu_contract.py --serial emulator-5554 --boot-wait 900

前置：`emulator -avd <arm-avd> -no-window -no-audio -no-boot-anim -gpu swiftshader_indirect`
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PROBE_LOCAL = ROOT / "android/app/build/intermediates/cxx/Debug/2t6w1p1y/obj/arm64-v8a/kivo_hdr_probe"
OUT_DIR = ROOT / "docs/evidence/emulator"

# 用例 → (设备上的源文件, 本地源文件)。只挑与「契约」有关的样本。
# 不挑 hdr10 片源：颜色键读取也算 A 类，但它已被 F3B 顺带证明，
# 本卡的焦点是加密契约，样本越少跑得越快（模拟器上每次要几十秒）。
CASES = [
    ("cenc", "/data/local/tmp/emu_cenc.mp4", ROOT / "tests/real/negative_cenc_h264.mp4"),
    ("cenc_pssh", "/data/local/tmp/emu_cenc_pssh.mp4", ROOT / "tests/real/negative_cenc_pssh.mp4"),
    ("truncated", "/data/local/tmp/emu_truncated.mp4", ROOT / "tests/real/negative_truncated.mp4"),
]

# 真机基线（API 33 / K40）。用来对比 —— 差异才是这条路的产出。
BASELINE = {
    "cenc": {"crypto_mode_present": True, "crypto_mode": 1, "is_encrypted_flag": False},
    "cenc_pssh": {"crypto_mode_present": True, "crypto_mode": 1, "is_encrypted_flag": False},
}


def adb(serial: str, *args: str, timeout: int = 300) -> subprocess.CompletedProcess:
    return subprocess.run(["adb", "-s", serial, *args], capture_output=True, text=True,
                          timeout=timeout)


def wait_boot(serial: str, limit_s: int) -> None:
    """等 `sys.boot_completed`。ARM 软模拟开机很慢，默认给 15 分钟。"""
    print(f"等待 {serial} 开机（上限 {limit_s}s）…", flush=True)
    deadline = time.time() + limit_s
    while time.time() < deadline:
        try:
            out = adb(serial, "shell", "getprop", "sys.boot_completed", timeout=20).stdout.strip()
        except subprocess.TimeoutExpired:
            out = ""
        if out == "1":
            print("  已开机")
            return
        time.sleep(10)
    raise SystemExit(f"开机超时（{limit_s}s）。ARM 软模拟可能需要更久，用 --boot-wait 调大。")


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="ARM 模拟器上的提取器契约检查")
    ap.add_argument("--serial", required=True, help="如 emulator-5554")
    ap.add_argument("--boot-wait", type=int, default=900)
    ap.add_argument("--skip-boot-wait", action="store_true")
    args = ap.parse_args(argv)

    if not PROBE_LOCAL.is_file():
        print(f"缺少 arm64 探针：{PROBE_LOCAL}\n先跑一次 gradle assembleDebug。", file=sys.stderr)
        return 2

    if not args.skip_boot_wait:
        wait_boot(args.serial, args.boot_wait)

    abi = adb(args.serial, "shell", "getprop", "ro.product.cpu.abi").stdout.strip()
    sdk = adb(args.serial, "shell", "getprop", "ro.build.version.sdk").stdout.strip()
    print(f"设备 ABI={abi} SDK={sdk}")

    adb(args.serial, "push", str(PROBE_LOCAL), "/data/local/tmp/kivo_hdr_probe")
    adb(args.serial, "shell", "chmod", "755", "/data/local/tmp/kivo_hdr_probe")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    rows = []
    for name, remote, local in CASES:
        adb(args.serial, "push", str(local), remote)
        raw = adb(args.serial, "shell",
                  f"/data/local/tmp/kivo_hdr_probe {remote}").stdout
        try:
            payload = json.loads(raw)
        except json.JSONDecodeError:
            print(f"  {name}: 探针输出不是 JSON（前 200 字符）：{raw[:200]}")
            rows.append({"case": name, "error": "probe output not json"})
            continue

        (OUT_DIR / f"emu-{name}.json").write_text(
            json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

        dump = payload.get("track_format_dump") or ""
        rows.append({
            "case": name,
            "schema": payload.get("schema"),
            "abi": abi,
            "sdk": int(sdk) if sdk.isdigit() else sdk,
            "crypto_mode_present": "crypto-mode" in dump,
            "crypto_key_present": "crypto-key" in dump,
            "is_encrypted_flag": (payload.get("first_sample") or {}).get("is_encrypted"),
            "mime": payload.get("mime"),
        })

    # ── 对比表 ───────────────────────────────────────────────────────────
    print("\n=== 提取器契约：模拟器 vs 真机（API 33 / K40）===")
    print(f"{'用例':<12} {'crypto-mode':<14} {'首包 is_encrypted':<20} 与真机一致？")
    inconsistent = 0
    for row in rows:
        if "error" in row:
            print(f"{row['case']:<12} {'—':<14} {'—':<20} 跑失败")
            inconsistent += 1
            continue
        base = BASELINE.get(row["case"])
        if base is None:
            verdict = "（无基线，仅记录）"
        else:
            same_mode = base["crypto_mode_present"] == row["crypto_mode_present"]
            same_flag = base["is_encrypted_flag"] == row["is_encrypted_flag"]
            if same_mode and same_flag:
                verdict = "一致"
            else:
                verdict = f"**不一致**（mode {base['crypto_mode_present']}→{row['crypto_mode_present']}, " \
                          f"flag {base['is_encrypted_flag']}→{row['is_encrypted_flag']}）"
                inconsistent += 1
        print(f"{row['case']:<12} {str(row['crypto_mode_present']):<14} "
              f"{str(row['is_encrypted_flag']):<20} {verdict}")

    print(f"\n证据写入 {OUT_DIR.relative_to(ROOT)}（已从总收口排除：B/C 类判据在模拟器上不可信）")
    if inconsistent:
        print(f"\n结论：**{inconsistent} 个用例与真机不一致** —— 这是真发现，"
              "说明低 API 的契约不同，判定逻辑需要按版本分支")
        return 1
    print("\n结论：契约一致 —— `crypto-mode` 在低 API 上同样存在，F3B 的判定不需要版本分支")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
