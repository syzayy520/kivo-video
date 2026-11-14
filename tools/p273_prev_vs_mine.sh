#!/bin/bash
# 274 · 「我接手后是否变差」的归一化对照
#
# 做法：
#   ① 改进版（当前工作区：智能取数）→ 240 s 长稳
#   ② 前助手最后版本 3dc441aa → 240 s 长稳
#   每轮前后各测一次源站基线（真机直连 8 MiB）。
#
# 为什么要归一化：源站速度是外部变量，不记基线则数字不可比（规格 §21.12）。
set -u
ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
URL="https://emby.bangumi.ca/emby/Videos/233908/stream?static=true"
cd "C:/kivo video" || exit 1

# 我的新文件（前助手版本里不存在，切过去前必须删掉，否则编译失败）
MY_FILES="android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerProbeLandingSeek.kt
android/app/src/main/java/com/kivo/cinema/ui/compose/LandingSeekState.kt
android/app/src/main/java/com/kivo/cinema/net/OriginLatencyStats.kt
android/app/src/main/java/com/kivo/cinema/net/WarmupPolicy.kt"

build_install() {
  (cd android && JAVA_HOME="C:/Android/jdk-17.0.13+11" ANDROID_HOME="C:/Android/sdk" \
    ./gradlew :app:assembleDebug --offline -q >/dev/null 2>&1)
  if [ ! -f android/app/build/outputs/apk/debug/app-debug.apk ]; then
    echo "  !! 编译失败"; return 1
  fi
  $ADB install -r android/app/build/outputs/apk/debug/app-debug.apk >/dev/null 2>&1
}

baseline() {
  $ADB shell "run-as com.kivo.cinema curl -s -o /dev/null -w '  源站基线 speed=%{speed_download}B/s time=%{time_total}s\n' \
    -H 'X-Emby-Token: $TOKEN' -r 0-8388608 '$URL'" 2>&1 | tail -1
}

soak() {
  local label=$1
  $ADB shell am force-stop com.kivo.cinema
  sleep 2
  $ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
  $ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
    --es kivo.playurl "$URL" --es kivo.embyToken "$TOKEN" --el kivo.playSize 1935985234 \
    --es kivo.label "$label" >/dev/null 2>&1
  sleep 245
  local out="docs/evidence/p273-emby-stutter/$label"
  mkdir -p "$out"
  $ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$out/net.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$out/kernel.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$out/diag.log" 2>/dev/null
  echo "--- $label ---"
  node tools/p273_gaps.mjs "$out/kernel.log" "$out/diag.log" 2>&1 \
    | grep -E "Enter buffering|实际帧率|渲染间隔" | sed 's/^/  /'
}

echo "########## ① 改进版（智能取数，carry 已回退）##########"
baseline
build_install && { soak "mine-soak"; }
baseline

echo ""
echo "########## ② 前助手最后版本 3dc441aa ##########"
git -c safe.directory="C:/kivo video" checkout -q 3dc441aa -- \
  android/app/src/main/java/com/kivo/cinema/net/ \
  android/app/src/main/java/com/kivo/cinema/playback/ \
  android/app/src/main/java/com/kivo/cinema/ui/compose/ \n  android/app/src/main/java/com/kivo/cinema/ui/design/settings/
for f in $MY_FILES; do rm -f "$f"; done
baseline
build_install && { soak "prev-soak"; }
baseline

echo ""
echo "########## 收尾：恢复改进版 ##########"
git -c safe.directory="C:/kivo video" checkout -q HEAD -- \
  android/app/src/main/java/com/kivo/cinema/net/ \
  android/app/src/main/java/com/kivo/cinema/playback/ \
  android/app/src/main/java/com/kivo/cinema/ui/compose/ \n  android/app/src/main/java/com/kivo/cinema/ui/design/settings/
for f in $MY_FILES; do
  [ -f "$f" ] || git -c safe.directory="C:/kivo video" checkout -q HEAD -- "$f"
done
echo "已恢复到当前工作区状态"
