# 编码面板验收边界补齐 · A1–A5 + B1 · 2026-11-21

> 接手线：CODEC-DEVICE.md §4 登记的三条未验边界 + 暂停态保持 + 首页在看页目检。
> 全部真机取证（b6e950ee），夹具目录 `fixture/`（sjis.srt / u16le.srt / big5.ass，内容互不相同
> ——归一化口径是 UTF-8 内容哈希，同文本会被去重合并，夹具必须各不相同）。

## 1. 结果总表

| # | 项 | 判据 | 结果 |
| --- | --- | --- | --- |
| A1 | Shift-JIS 真实片源 | 手选后画面正确 | ✅ `ENC-A1-sjis.png`：「中文字幕 シフトJIS テスト成功」逐字可读无乱码 |
| A2 | UTF-16LE(BOM) | 同上 | ✅ `ENC-A2-u16le.png`：「中文字幕 UTF16LE 测试成功」；导入即 BOM 探测命中（`converted codec=UTF-16LE`），强切 LE 复验一致 |
| A3 | `.ass` 重编码实测 | reencode 走通 + 画面 | ✅ BIG5 .ass 自动探测被 GB18030 吃掉（与 srt 同理，证手选必要）；切 BIG5 后 `reencode 96f56ff3653a-big5 codec=BIG5 bytes=594`，同路径覆盖 .ass；画面「中文字幕 繁體測試 ASS-BIG5」+ toast「字幕编码已切换为 BIG5」（`ENC-A3-ass-timing.png`） |
| A4 | 无外挂字幕切编码 | toast 分支不崩 | ✅ trace `sub-codec req=BIG5 … key=null` 后无 `sub-enc apply`、无崩溃；toast 机制同一 `showToast` 已由 A3 截图证实（A4 自身截图未抓到显示窗口，如实注明） |
| A5 | 暂停态切换 | 暂停+位置双保持 | ✅ `pre-pos pos=12.4 paused=true` → `pos_after=12.4 paused_now=yes`，零偏差（§3 探针扩展） |

## 2. 重大更正：位置保持的旧"PASS"是假象（机制认知修正）

A1/A3 轮 `pos_after≈4.0`（差 1.9s 贴判据边缘）引出的复现轮揭穿了真机制：

- **`mpv seek <sec>` 默认是相对 seek**（不是跳到第 sec 秒）；
- **`loadfile replace` 同一文件命中 demuxer 缓存 = 无缝续播**（位置根本不回 0）。

两者叠加下，旧验证（POS-DEVICE.md §2/§3 记录的 0.067s 差）其实是「seek 被竞态吞掉 +
缓存续播」的**巧合**，不是受控的位置保持；异常轮（2.067→4.0）才是 seek 真生效、相对前进
pos 秒的真回跳。POS-DEVICE.md §6 有更正记录。

**修复**（`PlayerSubtitleEncoding.kt`）：seek 带 `absolute` 标志 + 落位核对重试
（seek 后轮询 time-pos，|now−pos| ≤ 0.5s 视为落位，未落位重 seek，最多 3 次，
每次留 `sub-enc seek-check attempt=N seeked=… target=… landed=…` trace；3 次不落位
回退实时读数且 trace 可见——不静默）。

修复后复现验证（原必现异常场景 seekSec=2 + big5.ass/sjis.srt）：
两轮均 `attempt=1 landed`（2.1→2.166667、2.2→2.266667），一次落位、差 0.067s。

## 3. 取证探针扩展（PlayerProbeHooks.kt）

`--ei kivo.subCodecDelayMs`（默认 3000 不变）：A5 需要在 `kivo.freeze` 的 pause（onCreate+5.2s）
**之后**触发切换才能测到 paused=true 分支，固定 3s 排不进时序。取证钩子文件的参数化扩展，
不碰产品路径。

## 4. B1 · 在看页/看板目检 → 抓到并修复一个真回归

**现象**：修复前在看页空态（截图 61KB，纯背景）。
**根因**：首页重构收尾者把 `WatchingPage` / `ProfileStats.build` / `ContinueSource.continueEntries`
三个签名从 `List<HomeEntry>` 改成 `List<Pair<MediaItem, Progress>>`，而
`SmbContinueSource.recent()` 构造 `HomeEntry` **不带 mediaItem**（SMB 路径不是本地媒体模型）
⇒ Shell 里 `mapNotNull { it.toPair() }` 把 SMB/Emby 条目**全部滤掉**——「首页有、在看没有」
的老问题以新形态重演；且 `WatchingPage` 的 D378 功能（LazyColumn、长按删除对话框）被连带删掉。
**修复**：三文件恢复 HEAD（收尾者版本备份在 `overshadowed/`），Shell 调用恢复 HEAD 形态
（`entryPoster` / `openHomeEntry` / `removeWatching`）。
**修复后真机**：`B1-watching-fixed.png` 在看页 3 条 NAS（犯罪都市4 0:20/1:49:10、金刚 1:51/1:58:23、
Bodyguards E01 16:45/44:33）+ 3 条本地，缩略图/进度条/长按副标题齐全；`B1-profile-fixed.png`
「我的」看板正常（在看 10、日历热力图有数据、收藏/待看 0）。
**孤儿文件**：`WatchingSwipeRow.kt` / `WatchingIcons.kt`（收尾批次配套，恢复 HEAD 后无引用）
——未删（不是本线产物），待原作者处理。

## 5. 收尾

HTTP 服务/防火墙规则/设备临时截图已清理；`fixture/` 三份夹具保留（内容哈希与设备库内文件
逐字节可复算）。ui_gate 复跑 RESULT: PASS（含手势修复批次）。