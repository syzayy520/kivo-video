# 位置保持（换编码 reload）· 真机补验 · 2026-11-21

> 接手线：CODEC-DEVICE.md §10（设备掉线未取证）→ §11（测到 pos=0.000，根因=读取时机，已改
> 「读取前移 + 6×30ms 重试」但被首页重构编译红堵住）→ §12（第二道阻塞）。本文件是 §11 修复的**闭环证据**。

## 1. 环境

- 设备 `b6e950ee`（Redmi K40），APK = 本轮 `assembleDebug` 产物（65,753,351 B，2026-11-21 11:57，
  覆盖安装 `Success`，含 §11 的 pre-pos 修复 + 首页数据供给恢复）。
- 夹具：`fixture/big5.orig`（55 B，`adb exec-out run-as … cat` 原样拉出；**非 exec-out 的 `adb shell cat`
  会做 LF→CRLF 转换变成 58 B，内容哈希去重会失效——坑，记下**）。
- 供给：PC `192.168.5.142:8123` `python -m http.server`（需临时防火墙规则
  `kivo-sub-fixture-8123`，**测完已删**；子 shell 起夹具会退，用会话后台任务）。
- 起播：`am start -S … --es kivo.media /sdcard/Movies/Kivo/1080p30_60s.mp4
  --es kivo.subUrl http://192.168.5.142:8123/big5.orig --ei kivo.seekSec 30 --es kivo.subCodec GB18030`
  （不 freeze；`kivo.subCodec` 取证钩子起播 3 s 后触发，走与面板**同一** `applySubtitleEncoding`）。
  ⚠️ Git Bash 下 `am start` 的 `/sdcard/...` 与 `adb pull` 的远端路径都必须 `MSYS_NO_PATHCONV=1`，
  否则被转成 `Git/sdcard/...`（本轮实际踩到）。

## 2. 结果：**判据达成（CODEC-DEVICE §11 判据：差 ≤ 2 s）**

trace（`files/evidence/p220-status.log`，seekSec=30，起播后 ~3 s 在播放中段切换）：

```
probe sub-codec=GB18030
sub-enc apply fileKey=748a873bd6b8-big5 codec=GB18030 id=1
sub-enc pre-pos pos=30.1 paused=false          ← §11 修复生效：reencode 之前读到，不再 0.000
sub-enc reencode 748a873bd6b8-big5 codec=GB18030 bytes=63
sub-enc reload pos=30.1 paused=false ok=true
sub-enc seek pos=30.1 seeked=true
sub-enc reloaded pos_after=30.166667 paused_now=no
sub-enc ok(reload) fileKey=748a873bd6b8-big5 codec=GB18030
```

| 量 | 值 |
| --- | --- |
| 切换前 pos | 30.1 s |
| 重开后 pos_after | 30.166667 s |
| **差** | **0.067 s ≤ 2 s** ✅ |
| 暂停态 | `paused=false` → `paused_now=no`，未暂停 ✅ |

## 3. 状态复原（第二轮）

`--ei kivo.seekSec 2 --es kivo.subCodec BIG5`：`pre-pos pos=2.1` → `pos_after=2.166667`（差 0.067 s），
同判据 PASS。截图 `POS-2-big5-restore.png`：字幕显示 **「中文字幕 繁體測試 BIG5」**（繁体正确、无乱码），
时间码 00:00:12.633 正常推进。`subtitle_codec.xml` 记忆回到 `748a873bd6b8-big5 → BIG5`
（上一轮遗留的 GB18030 乱码态已复原）。

## 4. 首页恢复（同包顺带验证）

- 日志 `files/net-diag.log`：`[home] smb=2`、`[home] emby 完成 成功源=3/3 条=13`、
  `[home] homeEntries=10` —— SMB/Emby 数据供给恢复（详见 `home-feed-homeentry-family-tree.md` §1.1）。
- 截图 `../ui-shot/home-nas-restored2.png`：「继续观看」2 张 NAS 卡（金刚：骷髅岛 看到 1:31 /
  Bodyguards Jade Dolls 看到 16:45）、「电影」「剧集」分类行真实海报、hero 正常。

## 5. 收尾

防火墙规则已删、`/sdcard` 临时截图已清、HTTP 服务已停、播放器已退出。
夹具 `fixture/big5.orig` 保留（与设备 `subtitles-orig/748a873bd6b8-big5.orig` 逐字节一致，供复测）。
## 6. 更正（2026-11-21 晚，接手线）：本文 §2/§3 的"判据达成"是**假象**

A1/A3 边界补验轮（`ENC-BOUNDARY-DEVICE.md` §2）暴露 `pos_after≈4.0` 异常，复现轮揭穿真机制：

- `mpv seek <sec>` **默认是相对 seek**——当时根本没有"跳到第 30.1 秒"；
- `loadfile replace` 同一文件命中 demuxer 缓存 = **无缝续播**——位置本来就不丢。

§2/§3 的 0.067s 差是「seek 被竞态吞掉 + 缓存续播」的巧合，不是受控的位置保持。
真修复 = `seek … absolute` + 落位核对重试（`PlayerSubtitleEncoding.kt`，trace
`sub-enc seek-check attempt=N … landed=`），修复后复现轮两轮一次落位。
**以 ENC-BOUNDARY-DEVICE.md §2 为准**；本文其余环境/命令/坑仍有效。