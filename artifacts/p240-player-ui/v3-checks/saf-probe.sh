#!/usr/bin/env bash
# SAF 导入探针：打开字幕面板 → 点「导入字幕」行 → 截 DocumentsUI。
# 行位（K40 ×2.75，s3-subtitle.png 实测）：导入字幕 center ≈ (1932, 781)。
# 原子协议：taps+sleep+screencap 全在一条设备 shell 里。
set -u
export MSYS_NO_PATHCONV=1
ADB="C:/Android/sdk/platform-tools/adb -s b6e950ee"
OUT="C:/kivo video/artifacts/p240-player-ui/v3-checks"
DEV=/sdcard/kivo-sweep.png

$ADB shell "input tap 1200 540; sleep 0.5; input tap 2179 142; sleep 1.4; input tap 1932 781; sleep 2.5; screencap -p $DEV"
MSYS_NO_PATHCONV=1 $ADB pull $DEV "$OUT/s4-saf-picker.png" >/dev/null 2>&1
echo "done saf-picker"