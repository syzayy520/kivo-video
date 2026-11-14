#!/bin/bash
# 273 v32 实验：在**真机**上量"并行范围请求"能不能加速。
#
# 【为什么必须真机】PC 上的 Node 探针给出"单连接 285 KB/s、6 连接聚合 1625 KB/s"，
# 而真机上 curl 单连接只有 308 KB/s —— 两条路径都要在真机上复核，
# 结论不能外推。
#
# 【token 怎么送】每条 adb shell 自己的 stdin 管道 ⇒ token 不出现在命令行、不出现在日志里。
set -u
export PATH="$PATH:/c/Android/Sdk/platform-tools"
export MSYS_NO_PATHCONV=1

BASE=1876000000
URL='https://emby.bangumi.ca/emby/Videos/233908/stream?static=true'
TOKEN_FILE='C:/Users/codex/AppData/Local/Temp/embywork/token.txt'
TOKEN="$(tr -d '\r\n' < "$TOKEN_FILE")"

# 一路：把 token 喂给这条 shell 的 stdin
one() {
  local s="$1" e="$2"
  printf '%s' "$TOKEN" | adb shell "read -r T; curl -s -o /dev/null -w '%{http_code} %{size_download} %{time_starttransfer} %{time_total} %{speed_download}\n' -H \"X-Emby-Token: \$T\" -r $s-$e '$URL'"
}

# 并发 N 路
arm() {
  local parts="$1" piece="$2"
  local start_ms end_ms
  start_ms=$(date +%s%3N)
  local i=0 pids=""
  while [ "$i" -lt "$parts" ]; do
    local s=$((BASE + i * piece))
    local e=$((s + piece - 1))
    ( one "$s" "$e" >/dev/null ) &
    pids="$pids $!"
    i=$((i + 1))
  done
  for p in $pids; do wait "$p"; done
  end_ms=$(date +%s%3N)
  local total=$((parts * piece))
  echo "  ${parts} 路 × ${piece} B ＝ $((total / 1024)) KiB　用时 $((end_ms - start_ms)) ms　聚合 $((total * 1000 / (end_ms - start_ms) / 1024)) KB/s"
}

echo "=== 真机并行范围下载（emby.bangumi.ca，curl）==="
echo "臂 A：单路 4 MiB（真机单连接基准）"
one "$BASE" "$((BASE + 4194303))" | sed 's/^/  /'
echo "臂 B：单路 1 MiB"
one "$BASE" "$((BASE + 1048575))" | sed 's/^/  /'
echo "臂 C：4 路 × 1 MiB"
arm 4 1048576
echo "臂 D：8 路 × 512 KiB"
arm 8 524288
echo "臂 E：8 路 × 1 MiB"
arm 8 1048576
echo "臂 F：单路 4 MiB（复测，确认基准稳定）"
one "$BASE" "$((BASE + 4194303))" | sed 's/^/  /'
