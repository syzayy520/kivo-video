#!/usr/bin/env bash
# v3 sweep round 2 — deterministic protocol:
# taps + sleeps inside ONE device-side shell (ordering guaranteed), screencap separate.
# Every shot starts from known state: bars hidden (auto-hide), no panel.
set -u
ADB="C:/Android/sdk/platform-tools/adb -s b6e950ee"
OUT="C:/kivo video/artifacts/p240-player-ui/v3-checks"

reset() { $ADB shell "input tap 1200 540; sleep 4"; }

panel() { # name x y
  $ADB shell "input tap 1200 540; sleep 0.5; input tap $2 $3; sleep 1.4"
  $ADB exec-out screencap -p > "$OUT/s2-$1.png"
  reset
  echo "done $1"
}

# 00 sanity: whatever is on screen right now
$ADB exec-out screencap -p > "$OUT/s2-00-start.png"

# 01 bars only (wake, no panel)
$ADB shell "input tap 1200 540; sleep 0.8"
$ADB exec-out screencap -p > "$OUT/s2-bars.png"
reset; echo "done bars"

panel speed    2305 956
panel subtitle 2179 142
panel audio    2042 142
panel quality  1904 142
panel skip     1764 142
panel cast     1352 142
panel resource 1921 956
panel episodes 2123 956

# danmaku ops: long-press circle-danmaku (trigger is a guess, marked in evidence)
$ADB shell "input tap 1200 540; sleep 0.5; input swipe 1489 142 1489 142 900; sleep 1.4"
$ADB exec-out screencap -p > "$OUT/s2-danmaku-ops.png"
reset; echo "done danmaku-ops"

# audio-only mode: headphones toggles vid=no; bars stay (REX 0172)
$ADB shell "input tap 1200 540; sleep 0.5; input tap 2317 142; sleep 1.4"
$ADB exec-out screencap -p > "$OUT/s2-audiomode.png"
$ADB shell "input tap 2317 142; sleep 4"
echo "done audiomode"

# paused: floating camera button should appear left-center
$ADB shell "input tap 1200 540; sleep 0.5; input tap 166 956; sleep 0.9"
$ADB exec-out screencap -p > "$OUT/s2-paused.png"
$ADB shell "input tap 166 956; sleep 4"
echo "done paused"

# locked: long-press play locks; only unlock glyph bottom-left
$ADB shell "input tap 1200 540; sleep 0.5; input swipe 166 956 166 956 900; sleep 0.9"
$ADB exec-out screencap -p > "$OUT/s2-locked.png"
$ADB shell "input tap 162 956; sleep 4"
echo "done locked"

echo SWEEP_COMPLETE