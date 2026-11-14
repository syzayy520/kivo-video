#!/bin/bash
# 273 v43 归一化配对实验：修复前 / 修复后 交替跑，各 2 轮，每轮记录源站基线。
#
# 【为什么必须交替跑】源站是公共的、会波动的（实测单连接 785–3059 KB/s）。
# 只跑修复后那一轮 ⇒ 无法区分"改好了"与"源站当时快"。
# 交替跑 ⇒ 每一对的前后轮隔 1–2 分钟，源站条件近似相同。
set -u

ADB="/c/Android/sdk/platform-tools/adb.exe -s b6e950ee"
TOKEN="$(tr -d '\r\n' < 'C:/Users/codex/AppData/Local/Temp/embywork/token.txt')"
URL="https://emby.bangumi.ca/emby/Videos/233908/stream?static=true"
cd "C:/kivo video" || exit 1

# 参与 A/B 的文件（修复前 = 5014d8cf，修复后 = HEAD）
FILES="android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerActionRouter.kt \
android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerComposeRoot.kt \
android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerSmbStart.kt \
android/app/src/main/java/com/kivo/cinema/net/OriginWindowCache.kt \
android/app/src/main/java/com/kivo/cinema/net/OriginPrefetchKit.kt \
android/app/src/main/java/com/kivo/cinema/net/OriginTimePrefetch.kt \
android/app/src/main/java/com/kivo/cinema/net/HttpOriginSource.kt"

baseline() {
  $ADB shell "run-as com.kivo.cinema curl -s -o /dev/null -w '  源站基线 speed=%{speed_download}B/s time=%{time_total}s\n' -H 'X-Emby-Token: $TOKEN' -r 0-8388608 '$URL'" 2>&1 | tail -1
}

build_install() {
  (cd android && JAVA_HOME="C:/Android/jdk-17.0.13+11" ANDROID_HOME="C:/Android/sdk" \
     ./gradlew :app:assembleDebug --offline -q >/dev/null 2>&1)
  if [ ! -f android/app/build/outputs/apk/debug/app-debug.apk ]; then
    echo "  !! 构建失败，跳过本轮"; return 1
  fi
  $ADB install -r android/app/build/outputs/apk/debug/app-debug.apk >/dev/null 2>&1
  return 0
}

run_one() {
  local label=$1
  $ADB shell am force-stop com.kivo.cinema >/dev/null 2>&1
  sleep 1
  $ADB shell "run-as com.kivo.cinema sh -c ': > files/net-diag.log; : > files/mpv-kernel.log; : > files/mpv-kernel.log.diag'" 2>/dev/null
  $ADB shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
    --es kivo.playurl "$URL" --es kivo.embyToken "$TOKEN" --el kivo.playSize 1935985234 \
    --es kivo.landSeekSecs "4932" --ei kivo.landSeekDelayMs 9000 --es kivo.label "$label" >/dev/null 2>&1
  sleep 32
  local dir="docs/evidence/p273-emby-stutter/$label"
  mkdir -p "$dir"
  $ADB exec-out "run-as com.kivo.cinema cat files/net-diag.log" > "$dir/net.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log" > "$dir/kernel.log" 2>/dev/null
  $ADB exec-out "run-as com.kivo.cinema cat files/mpv-kernel.log.diag" > "$dir/diag.log" 2>/dev/null
  echo "--- $label ---"
  node tools/p273_drag.mjs "$dir" 2>&1 | grep -E "seek→首帧|落点窗口 " | sed 's/^/  /'
  node tools/p273_gaps.mjs "$dir/kernel.log" "$dir/diag.log" 2>&1 | grep -E "实际帧率|缓冲" | sed 's/^/  /'
}

for round in 1 2; do
  echo "########## 轮次 $round ##########"

  echo "[修复前]"
  git -c safe.directory="C:/kivo video" checkout -q 5014d8cf -- $FILES
  # ⚠️ 探针的**挂载调用**要留着（否则探针根本不跑，A/B 就没意义了）：
  #   PlayerSmbStart.kt 里 applyProbeLandingSeek() 那一行必须在。
  #   但 PlayerSmbStart.kt 整个文件回退了 ⇒ 手动补回这一行。
  if ! grep -q "applyProbeLandingSeek()" android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerSmbStart.kt; then
    sed -i 's|^\( *\)applyProbeMpvOptions(session)|\1applyProbeMpvOptions(session)\n\1applyProbeLandingSeek()|' \
      android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerSmbStart.kt
  fi
  # 旧版没有 waitLandingFirstSlice 参数 ⇒ 探针调用改回旧签名
  sed -i 's/seekTo(ms, waitLandingFirstSlice = true)/seekTo(ms)/' \
    android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerProbeLandingSeek.kt
  build_install && { baseline; run_one "pair-before-$round"; }

  echo "[修复后]"
  git -c safe.directory="C:/kivo video" checkout -q HEAD -- $FILES
  # 还原探针里被 sed 改过的签名（HEAD 版本带 waitLandingFirstSlice）
  git -c safe.directory="C:/kivo video" checkout -q HEAD -- \
    android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerProbeLandingSeek.kt
  build_install && { baseline; run_one "pair-after-$round"; }
done

# 收尾：恢复工作区到 HEAD（实验只是临时回退）
git -c safe.directory="C:/kivo video" checkout -q HEAD -- $FILES
git -c safe.directory="C:/kivo video" checkout -q HEAD -- \
  android/app/src/main/java/com/kivo/cinema/ui/compose/PlayerProbeLandingSeek.kt
echo "########## 完成，工作区已恢复 ##########"
git -c safe.directory="C:/kivo video" status --porcelain | head -5
