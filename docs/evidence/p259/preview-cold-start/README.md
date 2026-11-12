# 40 预览内核冷启动 / 重试不重建 / 首开瞬败 · 真机证据

设计稿：`docs/design/ui/replica/preview-cold-start-retry.md`（根因审计①②②'③ + 闸门 P1–P5）。
来源：257 回归披露 2/3（`docs/evidence/p257/emby-convergence/`）——Emby 预览帧未出（冷超时）、
冷超时后 `initFailed` 永久 latch、冷起播首开瞬败（mp4 header EOF）。

凭据红线：本目录不含任何密码 / token / 含密码表单态。真机链路凭据仅运行时使用（见复算命令），
token 离 URL 断言 = 0（内核日志只有 localhost 代理地址）。

设备：Redmi K40 `b6e950ee`（1080×2400，density 440）。
APK md5：`95351dd6de011df298f5f99e14af165f`（装机 == 构建，`pm path` + `md5sum` 复核）。

## 一、闸门表

| # | 闸门 | 判定 | 证据 |
| --- | --- | --- | --- |
| P1 | 冷启动预览：Emby 冷开拖动，预览 30s 预算内出帧 | **PASS（真机）** | 真链 bangumi 9.7GB mkv 冷开：`init_ms=6044`、`cold_first_latency_ms=12520`（<30000）、`frames_acquired=2`、近_seek `t=30 land=28.0 ok`、`decode_failures=0`（`thumb/p1-emby-probe.json`）；受控源预览浮窗真帧截图 `shots/p1-controlled-preview-bubble.png`；真链起播画面 `shots/p1-emby-real-play.png`；主内核 8.335s 解析完头 `logs/p1-kernel-emby-excerpt.txt` |
| P2 | 冷超时后重建：再次拖动触发一次内核重建（`init_retries>=1`） | **机制落地 + 代码核验；真机未触发（披露 D1）** | `MpvThumbnailReader.kt:107-110`（initFailed 后下一次 seek 重建一次）、`:131`（latch 仅在重试耗尽）、`:289-290`（30s / MAX_INIT_RETRIES=1）、`:253`（取证 init_retries）。两测试链 30s 预算内冷启成功 ⇒ `init_retries=0`，未观察到 >=1 |
| P3 | 主重试不泄漏：onRetry 旧 reader release、新 reader 重建 | **机制落地 + 代码核验；真机未直接测线程退出（披露 D1）** | `PlayerSmbStart.kt:167` `thumbnailReader?.release()` 先于换新 reader（`mountNetworkThumbnails`），SMB/URL 两路共用 |
| P4 | 首开不瞬败：主日志不再因空 206 提前 EOF | **PASS（受控注入 + 真机）** | 受控源对 offset {0,1048576} 前 2 次回空 206、第 3 次回真实数据（`logs/p4-empty206-inject.log`：EMPTY206 attempt=1/2 → DATA attempt=3）；主内核仍 `All headers are parsed!`+`playback restart complete`（`logs/p4-kernel-controlled-excerpt.txt`）；真链冷开无 `error reading header`（`logs/p1-kernel-emby-excerpt.txt`）。注入源可复算：`logs/p4-range-server.js` |
| P5 | 行数/红线：三文件 ≤300；凭据/token 不入证据 | **PASS** | `HttpByteSource.kt`=98、`MpvThumbnailReader.kt`=293、`PlayerSmbStart.kt`=295；token 离 URL grep=0；本目录无凭据 |

## 二、复算命令

受控空 206 注入（P4）——任意本地 mkv 均可，闸门与片源内容无关：

```bash
node docs/evidence/p259/preview-cold-start/logs/p4-range-server.js <本地.mkv> server.log 18099 &
adb -s b6e950ee reverse tcp:18099 tcp:18099
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl http://127.0.0.1:18099/<名>.mkv --es kivo.label p40 \
  --ei kivo.thumbProbeSec 30 --ei kivo.thumbDumpSec 14
# 期望 server.log：offset=0 与 1048576 各 EMPTY206 attempt=1/2 后 DATA attempt=3；
# 期望内核日志：All headers are parsed! + playback restart complete，无 error reading header。
grep -E "EMPTY206|DATA offset=0 |DATA offset=1048576 " server.log
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/mpv-kernel.log" | grep -c "error reading header"   # 0
```

真链 Emby 冷开（P1）——凭据仅运行时，不入库：

```bash
# AuthenticateByName（无密码源）取 token/userId（运行时变量，勿写盘入库）；
# Items 取一 Movie id；Range 0-0 断言 206 + Content-Range */总长；
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.playurl "https://emby.bangumi.ca:443/emby/Videos/<id>/stream?static=true" \
  --es kivo.embyToken "$T" --es kivo.label p40 --ei kivo.thumbProbeSec 30 --ei kivo.thumbDumpSec 40
adb -s b6e950ee pull /sdcard/Android/data/com.kivo.cinema/files/evidence/v31-thumb.json -
# 期望 cold_first_latency_ms < 30000、frames_acquired>=1、decode_failures=0。
adb -s b6e950ee shell "run-as com.kivo.cinema cat files/mpv-kernel.log" | grep -c -i -E 'bangumi|api_key|X-Emby|token='   # 0（token 离 URL）
```

行数（P5）：`wc -l` 三文件（98/293/295）。

## 三、披露

- **D1（P2/P3 真机未触发）**：30s 预算放宽后，受控源与真链 bangumi 冷启均在预算内成功
  （`init_retries=0`），故「冷失败→重建」与「onRetry 线程退出」未在真机自然触发；现有取证钩子
  （probe 单次 seek、scan 间距 250ms）无法间隔 >30s 制造两次冷失败。两闸门以代码核验落地
  （行号见闸门表），不宣称真机已触发。若需真机触发，需另加间隔钩子（超出本卡 scope）。
- **D2（远_seek 6s grab 超时，既有独立限制）**：真链远位置（3867s/4447s）预览 grab 在 6s 等待窗内
  超时（`thumb/p2-emby-farseek-timeout.json`：`wait=6001ms timeout`）。这是 grab 等待窗（6s）与慢链
  远簇读取的矛盾，**非**本卡冷启动预算问题；近_seek（30s）成功。超时走 `onUnavailable(非空 reason)`
  （`MpvThumbnailReader.kt:120/172/221`），不静默。
- **D3（px0 黑 ≠ 解码失败）**：真链为 letterbox 电影，`grabs[].px0` 取左上角=黑边，故 `ff000000`
  属预期；有效性以 `frames_acquired` + `land` + `decode_failures=0` 判定。受控彩条源 px0 非黑可对照。
- **D4（source 字段误标）**：URL 播放时 `v31-thumb.json.source` 回退 `currentMediaPath()`
  （`mkv-audio.mkv`），非实际源；实际源以截图 + 受控 server.log / 真链 Range 断言为准。非代码缺陷。
- **D5（真链浮窗截图未得）**：probe 浮窗窗仅 6–11s，短于真链冷启 ~12s；playing 分支控制条 3s 闲置即隐
  （`PlayerSessionBridge.kt:108/133` 无 previewMs 守卫）。故真链浮窗视觉未截到，以受控浮窗截图
  （`shots/p1-controlled-preview-bubble.png`）+ 真链 grabs 数值共同证 P1。不伪造真链浮窗图。
- **D6（端口）**：受控源初选 18080 被外来进程占用（EADDRINUSE），未杀外来进程，改 18099。
- **D7（凭据）**：两测试服凭据 / TMDB key 仅运行时；本目录与内核日志均不含 token（grep=0）。
- **D8（素材）**：受控源为彩条测试图（非个人素材）；真链截图为商业电影帧（非家庭视频），沿用 257 先例归档。