# v3.1f · 清晰度分档统一 + 源卡显示来源 + 进程内唯一播放页

日期：2026-11-12　设备：Redmi K40 `b6e950ee`（真机，横屏 2400×1080）
内核：libmpv（253）　片源：SMB `XiaoMi/视频/kivo-test/shelter.mkv`（hevc **1920×804** 23.976fps，
见 `mpv-thumb-v31c-round1.log:481`；用户口述"870 多"实为 804 的记忆偏差）
参考：REX 截图 0169（资源面板：`播放资源:` + chips + 源卡显示服务器名 `nyamedia` + 绿勾，**不是文件名**）

用户原话（本轮三个输入）：
1. "就是底下那个选择清晰度的，明明是选的 1080P，然后里面怎么只有 870 多还是多少来着？"
2. "底下那个你最好是像 REX 那个样子显示是来自哪里，而不是这个是什么文件……来自 NAS 是哪一个 NAS……是 emby"
3. "发现bug 当前后台两个同时在播放，你查看下日志有问题"

## 一、清晰度分档统一（修投诉 1）

**根因**：底栏徽章按**宽**分档（≥1900→1080P），资源面板按**高**分档（≥1000→1080P，否则报实际高度）。
1920×804 的宽银幕片源 ⇒ 底栏 1080P、面板 804P，同一屏两个口径。

**修法**：分档规则收敛为唯一函数 `resolutionTier(width, height)`，取**长边**
（宽银幕裁的是高、竖屏裁的是宽，档位留在长边上）：
`android/.../ui/compose/player/PlayerResolution.kt:14`（≥3800→4K / ≥1900→1080P / ≥1200→720P / 否则空串）。

三个调用位全部收敛，不再各写一份：
- 底栏徽章 `PlayerActionRouter.kt:33`（空串时沿用上次真值，音频-only 不瞎报）；
- 资源面板 chip `PlayerPanelItems.kt:175`（空串兜底报实际高度 `"${h}P"`，<720 的片源不冒充标准档）；
- 详情页匹配卡 `ui/detail/MediaMatch.kt:35`（空串兜底 `SD`）。

兜底词各显示位自决、**不静默编造标准档**；标准档三处必然一致。

**真机**：`origin-v31f-seed-panel.png` 与 `origin-v31f-realpath-panel.png` ——
面板 chip 均为 **1080P**，与底栏徽章同口径；804P 不再出现。

## 二、源卡显示"来自哪里"（修投诉 2，REX 0169 对齐）

**根因**：源卡此前显示**文件名**，偏离 REX 0169 的"服务器名 + 绿勾"语义。

**修法**：新增单一职责文件 `player/PlayerSourceOrigin.kt`，优先级链（`resolve`，:23）：
**显式服务器名（kivo.sourceName）> 源记录名（SourceStore 按 id 查）> SMB 共享名 > URL 主机 > "未知来源"**。
- `from(intent)` 只读 intent，主线程安全；`resolve` 查磁盘，**仅在 IO 线程调**
  （调用点在 `PlayerPanelItems.enumeratePanel` 的 `Dispatchers.IO` 内）。
- Emby 入口补显式名：`emby/EmbyActivity.kt:84` `.putExtra("kivo.sourceName", model.serverName)`
  （serverName = 用户保存名 ?: URI host ?: "Emby"，与 Emby 库页头显示一致）。
- 查不到任何来源时显示"未知来源"，**不编造**。

**真机两条链路**：
- _seed 链路_（adb 注入、id 不在源记录里）：`origin-v31f-seed-panel.png` 源卡 **XiaoMi** ⇒ 走 smbShare 兜底分支；
- _真实链路_（资源库 → XiaoMi → 视频 → kivo-test → shelter.mkv，`SmbBrowserActivity.kt:175`
  传真实 `kivo.sourceId`）：`origin-v31f-realpath-panel.png` 源卡 **XiaoMi** + 绿勾，
  与源列表记录名一致（`origin-v31f-sourcelist.png`），**不再是文件名**。

**诚实注记**：本机记录名与共享名可能同为 XiaoMi，单张截图不能隔离"记录名/共享名"两分支；
但两分支返回的都是**来源名**而非文件名，需求在任一分支下成立。记录名分支的查表逻辑经代码核对
（id 存在于 SourceStore，schema 2，`SourceStore.kt:49` 读 name）。

## 三、进程内唯一播放页（修投诉 3，双音频叠播）

**根因**：`PlayerComposeActivity` 无 onStop/onPause 暂停语义（214 后台播放与 PiP 依赖会话存活），
launchMode=standard ⇒ 再开一个播放页时旧页继续播 ⇒ 两条 AudioTrack 同时出声（用户真机听到）。

**修法**（`PlayerComposeActivity.kt`）：companion 持 `WeakReference<PlayerComposeActivity>`（:176）；
新页 `onCreate` 先 `finish()` 旧页（:181）再登记自己（:182）；`onDestroy` 清引用（:258）。
旧页 finish 走既有 `closeNetworkPlayback()` ⇒ `mpv?.stop()`，音频随之停。
**不选 onStop 暂停**：那会杀掉 214 后台播放与 PiP，取舍写明在此。

**真机证据**（本目录）：
- `single-player-log.txt`：旧 AudioTrack 会话 `stop` 在前、新会话 `start` 在后，无重叠出声窗口；
- `single-player-stack.txt`：Activity 栈 Hist 仅 1 个 PlayerComposeActivity。

## 四、REX 逐项自检索引（真机，全轮汇总）

| 项 | 证据（本目录） | 结果 |
| --- | --- | --- |
| 装机 + 播放回归 | `01-launch-bars.png` | ✅ |
| 拖动预览"拖到哪帧跟到哪" | `preview-v31d-loading.png` / `preview-v31d-t300-loaded.png` / `thumb-v31d-frame40.png` / `v31-thumb-v31d-scan.json` | ✅ 第二 mpv 实例出帧 |
| 顶栏图标语义 | `topbar-v31d-icons.png` | ✅ 逐枚核对 |
| 画中画 | `pip-v31d-enter-firstframe.png` / `pip-v31d-live.png` | ✅ 进 PiP 首帧即活 |
| 弹幕 | `02…08-danmaku-*.png` | ✅ 开关/操作/字号/区域/重置 |
| 交互动效对照 REX | `v31e-motion-selfcheck.md` + `hud-v31e-*.png` | ✅/◐/❌ 分级明示 |
| 清晰度分档统一 | `origin-v31f-seed-panel.png` / `origin-v31f-realpath-panel.png` | ✅ 本轮 |
| 源卡来源名（REX 0169） | 同上 + `origin-v31f-sourcelist.png` | ✅ 本轮（Emby 见五） |
| 进程内唯一播放页 | `single-player-log.txt` / `single-player-stack.txt` | ✅ 本轮 |
| 字幕面板/轨道/渲染/设置 | `sub-v31f-*.png`（见五） | ✅ 本轮（PGS 源，ASS 对照受限） |

资源面板结构对照 0169：`播放资源:` 标签 + 圆角 chip + 左下源卡（SDR / 大小 / 码率 / 来源名 + 绿勾）✅ 结构一致。

## 五、字幕真机取证（本轮补证）

渲染唯一来源＝libmpv 内置 libass（v3 设计 §1.6），Compose 不自绘。真机链路：

| 步骤 | 证据 | 事实 |
| --- | --- | --- |
| 面板入口行序 | `sub-v31f-panel.png` | 内嵌>／外挂>／搜索（置灰+「字幕搜索服务尚未接入」）／导入／操作头／显示设置，与 0160/0161 行序一致 |
| 内嵌轨道列表 | `sub-v31f-tracks.png` | 内核真值 4 轨（en / TX-CHS / TX-CHS/ENG / TX-CHT，均 hdmv_pgs_subtitle） |
| 选轨回读 | `sub-v31f-selected-entry.png` | 选 TX-CHS 后入口行副题＝轨名（行状态＝内核真值，非 UI 自记） |
| 渲染 | `sub-v31f-render.png` | 对白帧底部居中中文 PGS 字幕「先休息吧」，合成进视频帧 |
| 设置卡形态 | `sub-v31f-settings.png` | 标准统一样式 toggle + 字号/垂直位置/字幕延迟/描边 4 stepper + 重置 |
| stepper 真写 | `sub-v31f-step.png` | 字号 52→53（`sub-font-size` 步长 1，写后读回） |
| toggle 真写 | `sub-v31f-toggle.png` | 开＝`sub-ass-override=yes`（`PlayerPanelRouter.kt:190`） |
| 重置真写 | `sub-v31f-reset.png` | 字号回 55、toggle 回 off（五属性一次写回，失败 toast 非空 reason） |

**诚实边界**：shelter.mkv 字幕为 **PGS 位图轨**，`sub-ass-override`/sub-* 样式只作用于文本字幕 ⇒
ASS 特效保留与统一样式观感对照**本片源无法行使**，需带 ASS 轨片源补证；
本轮证明的是：面板/轨道/渲染/设置全链路真接内核、无静默降级。

## 六、诚实声明（不静默、不伪造）

1. **Emby 真服务器往返未测**：本机源列表"媒体服务器 0 个"（`origin-v31f-sourcelist.png`），
   无 Emby 源可点。`kivo.sourceName` 显式名分支仅代码核对（`EmbyActivity.kt:84`），未真机行使。
2. 两截图码率不同（11.69Mb vs 3.37Mbp）是 mpv **实时采样**随画面变化，非缺陷。
3. 记录名/共享名分支隔离见二之诚实注记。
4. 横屏 screencap 坐标换算 ×1.2（2400×1080 显示 2000×899）；控制条 ≈3s 自动隐藏，
   取证须"唤出 + 点 chip"同一命令链完成。

## 七、复算路径

```bash
adb -s b6e950ee shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
  --es kivo.smbShare XiaoMi --es kivo.smbPath 视频/kivo-test/shelter.mkv --es kivo.label shelter.mkv
# 控制条可见时点清晰度徽章（横屏 1932,960）⇒ 面板 chip 1080P + 源卡 XiaoMi
adb -s b6e950ee exec-out screencap -p > panel.png
# 真实链路：资源库 tab → XiaoMi → 视频 → kivo-test → shelter.mkv，同法取证
# 叠播复算：播放中再开一个播放页 ⇒ logcat 旧 AudioTrack stop 先于新 start；dumpsys activity activities 栈=1
```

家族树（铁律）：新增两个单职责小文件（`PlayerResolution.kt` 21 行 / `PlayerSourceOrigin.kt` 44 行），
调用位收敛不堆砌；改动后 `PlayerComposeActivity.kt` 274 行、`PlayerPanelItems.kt` 180 行，均 ≤300。