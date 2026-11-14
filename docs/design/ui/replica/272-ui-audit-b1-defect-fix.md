# 272 · UI/交互缺陷修复 · 批次一（纯 bug 修复）

> 来源：2026-11-13 全库 UI/UX 审查（275 个 `.kt`、4 路并行深挖 + 逐条复核）。
> 本卡只做**第一批**：确凿缺陷修复，**不碰视觉语言、不动设计方向**。
> 视觉收口（第二批）、交互与无障碍（第三批）、工程化（第四批）各自另开卡。

## 〇、红线

- 同一工作目录 `C:\kivo video`，不新建项目包。
- 单文件单职责；每个 `.kt` ≤ 300 行；新增文件必须按语义归入既有家族树，不平铺。
- **不许伪造状态**：UI 显示的阶段必须来自内核真值（会话 state / 真实错误串），
  读不到就是"未知"，不能编。
- **不许静默降级**：任何"点了没反应 / 出错看不见"必须带非空 reason 且用户可见。
- 真机 K40 `b6e950ee` 复验；证据落 `docs/evidence/`；**模拟器数字无效**。
- 本卡**不改**：视觉语言（冷灰底 + 扁平纯白卡 + 蓝选中）、设计 token、
  播放器 59 参数函数、面板 kind 字符串体系 —— 那些属于第二/三/四批。

## 一、现状审计（确凿缺陷，带行号）

### A. 播放器阶段判定靠文案子串（最严重，一处错三处坏）

`PlayerComposeRoot.kt:68-69`：
```kotlin
buffering = statusText.contains("缓冲") || statusText.contains("连接中")
errorText = if (statusText.contains("失败") || statusText.contains("中断")) statusText else ""
```
全工程 30 处 `statusText` 赋值中：

| 现象 | 证据 | 后果 |
| --- | --- | --- |
| 缓冲态恒 false | 无任何 statusText 含"缓冲"/"连接中"（实际是"正在连接SMB…"） | `PlayerStatusLayer.kt:53` 的中央波纹是**死代码** |
| 致命错误永不显示 | `PlayerSmbStart.kt:112` "这个片源无法播放"、`:159` "网络视频无法播放"、`:219` "软解打不开这个片源"、`PlayerSessionStart.kt:57` "音频无法启动" | 用户只见黑屏，无原因 |
| 重连误报失败 | `PlayerSessionBridge.kt:157` "网络中断，正在重连…"（含"中断"） | 自动重连时弹出居中「**播放失败** + 重试」 |

### B. 失败/结束态下位置轮询永久停止

`PlayerSessionBridge.kt:119-128`（mpv）与 `:101-107`（软解）在 FAILED/ENDED 分支 `return`，
而 `postDelayed(this, 100)` 排在 `:135` / `:109` —— **被跳过**。
`:131` 的注释自己写着"提前 return 会跳过 postDelayed（真机踩过）"，但 return 仍在前面。

### C. 拖动被取消 ⇒ `scrubbing` 永不复位 ⇒ 弹幕永久消失

`PlayerSeekTrack.kt:145-148` 的 `onDragCancel` 只清 `dragging`，不触发 `onSeek`；
而 `PlayerComposeRoot.kt:113` 只有 `onSeek` 才 `scrubbing = false`；
弹幕可见性 = `danmakuOn && !pipActive && !scrubbing`（`:48`）。
一次系统手势打断拖动 ⇒ 弹幕再也不出现。

### D. 详情页 CTA 副标题与实际播放对象不一致

副标题取当前季下一集：`DetailPage.kt:131-134`；
点击执行 `DetailPage.kt:161` `openPage(playIntent)`，而 `playIntent` 固定为首季第 1 集：
`EmbyResource.kt:118-121`。切季后按钮写"第2季·第5集"、实际播第 1 季第 1 集。

### E. 详情页三个浮层无法用系统返回键关闭

`ui/detail/` 全目录 `BackHandler` 命中 = **0**。
季选择（`DetailSeasonMenu.kt:166`）、更多（`DetailMorePopup.kt:67`）、
日历（`DetailCalendarSheet.kt:84`）都是普通 `Box` 叠加，返回键直接 finish 整个详情页。
设置页已有正确写法（`SettingsAppearanceSub.kt:33`），本页漏。

### F. 「在看」页导航无选中态

`MainActivity.kt:238` `selected = listOf(1, 3, 0, 4).indexOf(page)`；
page 语义 0=资源库 / 1=发现 / 2=在看 / 3=我的 / 4=设置 ⇒ page==2 时 `indexOf(2) == -1`。
四个 tab 全灰，且 `WatchingPage.kt` 全页无返回路径。

### G. 系统栏 inset 缺失（targetSdk 37，Android 15+ 强制 edge-to-edge）

| 位置 | 现状 |
| --- | --- |
| `MainActivity.kt:323` 导航胶囊 | 无 `navigationBarsPadding()`；而资源库页同款胶囊 `SourceWorkspace.kt:27` 有 |
| `MainActivity.kt:264` + `DiscoverPage.kt:105-112` | 发现页显式传 `WindowInsets(0,0,0,0)`，LazyColumn 无 top padding ⇒ 滚过 hero 后文字穿状态栏 |
| `MediaLibraryActivity.kt:199-226` | 顶/底浮钮均无 inset；对照 `EmbyActivity.kt:76`、`AggregateActivity.kt:57` 都用 `safeDrawingPadding()` |

### H. 设置开关写回失败被静默吞掉

`SettingsDataSub.kt:34-41` `repo.setControl(...)` 只挂 `onSuccess`，**无 `onFailure`**。
写盘失败 ⇒ 开关弹回、用户零感知。违反"不许静默"。

### I.「移出待看」无二次确认

`DetailCalendarSheet.kt:160-164` 垃圾桶与"分享"并排、无确认，误触即丢数据。
相邻的"清理缓存"反有 AlertDialog（`SettingsCacheSub.kt:62`）—— 危险等级与保护强度倒挂。

## 二、方案与家族树

### 2.1 家族树（新增 1 个文件，其余为原地改造）

```
ui/compose/                          ← 播放器家族
├─ player/PlayerPhase.kt            【新增】播放阶段唯一真值模型
├─ player/PlayerStatusLayer.kt      改造：参数 (buffering,errorText) → (phase)
├─ player/PlayerOverlay.kt          改造：透传 phase
├─ player/PlayerSeekTrack.kt        改造：onDragCancel → onScrubEnd
├─ PlayerComposeActivity.kt         改造：新增 phase 状态字段（状态所有权仍在此）
├─ PlayerComposeRoot.kt             改造：去 contains 嗅探；接 onScrubEnd
├─ PlayerSessionBridge.kt           改造：按真值写 phase；postDelayed 移入 finally
├─ PlayerSessionStart.kt            改造：起播失败路径写 phase
├─ PlayerSmbStart.kt                改造：起播失败路径写 phase
├─ PlayerActionRouter.kt            改造：控制失败写 phase；重试写 phase
└─ PlayerCastBridge.kt              改造：投屏中断写 phase

ui/detail/                           ← 详情页家族
├─ DetailPage.kt                    改造：BackHandler；CTA intent 同源派生
├─ EmbyResource.kt                  改造：playIntent 按"当前季/下一集"派生
└─ DetailOverlayHost.kt             改造：浮层开关合并 + 移出待看确认

ui/design/                          ← Shell 家族
├─ WatchingPage.kt                  改造：返回栏 + 空态「去发现」按钮
├─ DiscoverPage.kt                  改造：top inset
└─ settings/SettingsDataSub.kt      改造：补 onFailure

MainActivity.kt                     改造：导航选中态 / inset / 渐变短路
library/MediaLibraryActivity.kt     改造：safeDrawingPadding
```

**为什么只新增 1 个文件**：`PlayerPhase` 是"阶段模型"，播放器家族里没有同类物；
状态层渲染**改造**已有的 `PlayerStatusLayer`（不新建 `PlayerPhaseLayer`）——
新建一个并行的层就是"两个 look-alike"，正是仓库反复出现的老毛病。

### 2.2 阶段模型

```kotlin
sealed interface PlayerPhase {
    data object Idle : PlayerPhase          // 未起播
    data object Playing : PlayerPhase       // 正常播放
    data object Buffering : PlayerPhase     // 起播中 / 卡缓冲
    data object Reconnecting : PlayerPhase  // 网络中断，正在重连（不是失败）
    data object Ended : PlayerPhase         // 播完
    data class Failed(val reason: String) : PlayerPhase  // 致命错误，reason 非空
}
```
- `Failed.reason` **必须非空**：空串等同"静默"。
- 阶段只从**内核真值**来：会话 `state`（`MpvPlaybackSession.State` /
  `NetworkPlaybackSession.State`）、`buffering` 标志、`lastError`、`isRunning()`。
- `statusText` 降级为**纯展示文案**（调试/日志），不再参与任何判定。

### 2.3 各阶段渲染（PlayerStatusLayer）

| 阶段 | 渲染 |
| --- | --- |
| Buffering | 中央三点波纹（现有 `BufferingRipple`，从死代码复活） |
| Failed | 居中失败面板 + 原因 + 重试（现有形态保留，只是现在真的会触发） |
| Reconnecting | **底部轻提示**（不挡画面中央）—— 这是 `PlayerStatusLayer` 文件头注释里已经写明、但从未实现的规格 |
| Ended | 居中「播放结束」提示（本期只给提示，"重播/下一集"入口留第三批） |

### 2.4 三个待拍板点的处置（用户授权"按建议推进"，此处记明决定）

| 问题 | 决定 | 理由 |
| --- | --- | --- |
| 「在看」页是 tab 还是子页 | **子页**：nav 保持「发现」高亮（`page==2` 映射到 tab 0），在看页顶部加返回栏 + 空态「去发现」按钮 | 胶囊 4 项时每项 ~64dp，加到 5 项会压到 <48dp；且「在看」本就是发现页的下钻 |
| 深色模式 | **保留设置页那行**（它如实写着"深色色板 v2 未实现，当前固定浅色"），本卡不动；M3 DayNight 迁移留第三批 | 视频浏览器早晚需要深色，但那是独立一张卡；本卡不碰 theme，避免与视觉收口混在一起 |
| 类型墙 / 评分徽章的高饱和色 | **明确豁免**：按 256 复刻规格实现，属设计稿授权；在本卡记明，防止下一轮被当残留清掉 | `docs/design/ui/replica/emby-aggregate-search.md:11` |

## 三、验收判据

### 3.1 静态

- [ ] 全工程 `ui/` 下 `contains("失败")` / `contains("中断")` / `contains("缓冲")` **零命中**（状态判定）
- [ ] `PlayerStatusLayer` 参数不再有 `buffering: Boolean` / `errorText: String`
- [ ] `ui/detail/` 有 `BackHandler`
- [ ] `PlayerPositionPoll.run()` 的每个出口都重新排期（或统一在 `finally`）
- [ ] 每个改动的 `.kt` ≤ 300 行

### 3.2 真机（K40 `b6e950ee`）

| # | 场景 | 期望 |
| --- | --- | --- |
| 1 | 本地文件起播 | 控制条正常出现；无错误面板 |
| 2 | SMB 资源库指向**不存在**的路径 | 出现失败面板，原因非空且能看出是路径/权限问题；点「重试」不崩 |
| 3 | 播放中拖动进度条后**用系统手势打断**（滑回桌面再回） | 回来后弹幕**仍然出现** |
| 4 | 播完 8s 测试片 | 出现「播放结束」提示，不卡在最后一帧无提示 |
| 5 | 详情页打开季选择浮层 → 按返回键 | **只关浮层**，详情页不退出；再按返回才退出 |
| 6 | 详情页 Emby 剧集：切到第 2 季 | 播放按钮副标题与实际起播集**一致** |
| 7 | 首页 →「在看」 | 底部胶囊「发现」高亮；「在看」页顶部有返回；空态有「去发现」按钮 |
| 8 | 发现页下滚过 hero | 节标题**不穿状态栏**；状态栏图标与背景明暗一致 |
| 9 | 本地媒体库页（targetSdk 37） | 顶部返回钮不被状态栏压住；底部不被手势条压住 |
| 10 | 设置 → 数据与资源 → 开关写回失败（断网/存储满） | 有可见错误提示，不是静默弹回 |

## 四、证据

- `docs/evidence/ui-audit-b1/`：每个场景的截图 + 说明（文件名含场景号）。
- 编译日志落 `build-p272-b1.log`（沿用仓库既有 `build-p268*.log` 惯例）。

## 五、明确不做（下一批）

- 视觉收口：粉紫渐变清零、`GlassSurface` 清零、三套 primary 统一、占位色统一、
  `themes.xml` 迁 M3 DayNight、胶囊选中态形状冗余。
- 无障碍：`Role.Button` + `onClick` 语义封装、contentDescription 补齐、触控目标 48dp。
- 交互：暂停不自动隐藏控制条、seek 灵敏度分档、面板返回栈、面板锚定到触发按钮。
- 工程化：59 参数函数对象化、`PlaybackEngine` sealed interface、面板 kind 枚举化、
  token 化（454 处硬编码 dp/sp）。
