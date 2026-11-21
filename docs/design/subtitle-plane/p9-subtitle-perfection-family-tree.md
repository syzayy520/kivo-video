# P9 · 字幕完美化（第一波）· 家族树与契约

> 范围：A–I 九项，**不重建 libmpv 二进制**。选项唯一落地点仍是
> `cpp/mpv/mpv_sub_options.cpp`（initialize 前、逐项回执）；运行期改动一律走
> mpv 的 **option → property 自动注册**通道（`command_init` 为每个非
> `M_OPT_NOPROP` 选项注册同名属性），由 `MpvSession.setProperty` 下发。

## 1. 为什么需要这一波

现状是"能用但不一致"：

| 症状 | 根因 |
| --- | --- |
| 字号默认 52，重置写 55，注释说 5% | 三处各写一个数，没有共同真值 |
| 字幕贴着屏幕底边 | `sub-margin-y` 从未设置（mpv 默认 22） |
| 调好的样式重进播放器就丢 | 面板只写内核属性，不落盘；起播也不回放 |
| ASS 开关只有两态且文案误导 | `sub-ass-override` 三态（no/scale/force）被折成 bool |
| 导入的字幕重启后消失 | 落在 `cacheDir/subs`，系统随时清 |
| 只有主字幕 | `secondary-sid` 等能力从未接出 |
| 轨道看不出「强制/听障」；外挂删不掉 | track-list 少读两个字段；`sub-remove` 定义了没人调 |
| 软解/旧内核模式下字幕静默消失 | 没有提示，用户以为是字幕坏了 |

## 2. 文件家族树

```
playback/subtitle/                       ← 播放策略域（内核真值的唯一落盘与下发）
├── SubtitleStyle.kt                    样式模型 + 默认值 + 调色板 + 区间夹取（纯数据，可单测）
├── SubtitleStyleStore.kt               全局样式持久化（subtitle_prefs）+ 旧键 ignore_ass_style 迁移
├── SubtitleStyleApplier.kt             把样式翻成 mpv 属性并在 loadFile 前下发（逐项回执，失败入日志）
├── SubtitleDelayMemory.kt              按片源记忆 sub-delay（片源键 → 秒，孤立片源不串味）
├── SubtitleSourceKey.kt                片源稳定键（URL / SMB 源+共享+路径 / 本地路径）
├── SubtitleSecondaryControl.kt        次字幕语义（secondary-sid/pos/visibility/delay；主次互斥）
└── SubtitleImportStore.kt              导入字幕持久化（filesDir/subtitles、内容哈希去重、按片源登记）

ui/compose/player/subtitle/              ← 播放页写/读两侧（不碰 PlayerPanelRouter）
├── SubtitlePanelRows.kt                行数据装配：入口/显示设置/主次轨道（徽章、语言、关闭行）
├── SubtitlePanelActions.kt             事件入口：选择分派 / 步进 / 重置 / 长按删除 / 关闭字幕
├── SubtitleStyleActions.kt             样式写路径：ASS 三态 / 字号预设 / 调色 / 边框 / 加粗 / 次字幕显隐
├── SubtitleTrackActions.kt             主次选轨：主≠次互斥 / 次字幕位置与关闭
├── SubtitlePanelProbe.kt               取证钩子 kivo.subWrite（与面板同一条写路径）
└── SubtitleTrackLabels.kt              轨道文案：友好语言名 + 强制/听障/外挂徽章

mpv/MpvTrackCatalog.kt      （改）Track 增加 forced / hearingImpaired；外挂字幕可带标题
playback/MpvSubtitleControl.kt （改）补 removeExternalSubtitle（sub-remove + 删除后的主轨兜底）
ui/compose/PlayerSubtitleImport.kt （改）改走 SubtitleImportStore；失败非静默
ui/compose/PlayerSubtitleHint.kt （新）不支持字幕的播放模式只提示一次（I）
ui/compose/PlayerSessionStart.kt / PlayerSmbSoftStart.kt （改）软解 / 平台硬解 / legacy 起播处调用提示 + 文案"无字幕"
cpp/mpv/mpv_sub_options.cpp （改）默认字号 52→40，新增 sub-margin-y=40（与落盘默认同源）
playback/MpvStartSequence.kt （改）loadFile 前应用样式 + 本片源记忆的字幕延迟
ui/design/settings/SettingsPlaybackSub.kt （改）「忽略特效字幕样式」→ 三态「ASS 字幕样式」
ui/compose/player/PlayerGlassMenu*.kt / PlayerPanelHost.kt / PlayerOverlay*.kt
                            （改）新增 onLongPress 通道（外挂字幕长按删除）
ui/compose/PlayerPanelRouter.kt （只减不增）字幕分支整体交给 SubtitlePanelActions
```

## 3. 关键契约

### 3.1 字号与底距（A/B）

- **字号单位是 scaled pixel**：mpv 以 720 高为基准缩放。`40` 在 1080 高画面上
  ≈ 60 px ≈ **5.6%**，落在"5–5.5%"目标带内（52 会到 78 px ≈ 7.2%）。
- 三处同源：C++ 初值、Kotlin 落盘默认、面板重置，全部取 `SubtitleStyle.DEFAULT_FONT_SIZE = 40`。
- **底距** `sub-margin-y = 40`（≈60 px @1080）。它**只作用于非 ASS 文本字幕**
  （mpv 手册：unstyled text subtitles），ASS 字幕的位置由源样式/`sub-pos` 决定
  ⇒ 面板文案必须写"仅文本字幕"，不许暗示 ASS 也受控。
- **`sub-pos` 夹到 0–100**：mpv 允许 0–150，但 >100 会被 libass 裁掉（手册明说）
  ⇒ 超过 100 不是"更高"，是"消失"。夹取在写侧做（`SubtitleStyle.clampPos`）。

### 3.2 ASS 三态（A）

| 面板值 | mpv 值 | 语义 |
| --- | --- | --- |
| 源样式 | `no` | 完全按 ASS 源排版（默认） |
| 仅缩放 | `scale` | 保留源排版，只让字号跟随屏幕缩放 |
| 统一样式 | `force` | 用 sub-* 统一样式覆盖源样式 |

旧键迁移：`playback_prefs.ignore_ass_style = true` ⇒ `force`，否则 `source`；
写入 `subtitle_prefs.ass_mode` 后旧键不再被读取（设置页也改三态）。

### 3.3 落盘与生效时机（C）

- **全局样式**（字号/位置/底距/描边/颜色/边框/阴影/字间距/加粗/ASS 模式/次字幕位置/显隐）
  → `subtitle_prefs`，面板每次成功写入即落盘；起播时 `MpvStartSequence` 在
  `loadFile` 之前回放（`sub-ass-override` 是 UPDATE_SUB_HARD，必须早于开文件）。
- **sub-delay 是片源属性**：按 `SubtitleSourceKey` 记忆（URL / SMB 三元组 / 本地路径），
  不随全局样式走、不跨片源串味；面板步进写内核的同时写记忆。
- 读侧仍只读内核真值（`sub-font-size` 等属性），**不**用 prefs 冒充当前值。

### 3.4 导入与删除（D/H）

- 目录 `filesDir/subtitles`（私有、持久），文件名 `<md5前12位>-<消毒后的原名>`；
  同内容哈希已存在则复用，不重复占空间。
- `subtitle_imports` 记录「片源键 → 路径集合」；删除外挂轨时只清本片源登记的路径，
  且当没有其他片源引用该文件时才真正删盘。
- 删除的是当前主轨时：先 `sub-remove`，再读回 `sid`；无效则选剩余第一条，否则 `sid=no`，
  并把结果告诉用户（不静默）。

### 3.5 次字幕（F）

- `secondary-sid` 选轨、`secondary-sub-pos` 默认 **15**（避开主字幕）、
  `secondary-sub-visibility` 单独开关、`secondary-sub-delay` 单独步进。
- **主次不能同轨**：选次字幕时若等于当前 `sid` ⇒ 拒绝并提示；
  选主字幕时若等于 `secondary-sid` ⇒ 先关次字幕再切主字幕，并提示。
- 次字幕默认 `secondary-sub-ass-override = strip`（mpv 出厂），本波不改。

### 3.6 轨道信息（G）

- `Track` 增加 `forced` / `hearing-impaired`；行副题 = 友好语言 + 编码 + `强制` / `听障` 徽章。
- 内嵌字幕列表首行固定「关闭字幕」（写 `sid=no`）。
- 语言映射覆盖 zh/chi/zho、en/eng、ja/jpn、ko/kor、fr/fre/fra、de/ger/deu、es/spa、ru/rus、ar/ara。

### 3.7 无字幕链路提示（I）

软解（`FfmpegPlaybackSession`）、旧本地内核（`PlaybackSession`）、旧网络硬解
（`NetworkPlaybackSession`）三条链路都没有字幕面。首次进入时给一次 toast：
「当前播放模式不支持字幕（<模式>）」，且 `statusText` 追加「 · 无字幕」；
每次播放页生命周期只提示一次（不刷屏、不静默）。

## 4. 验收映射

| 项 | 验收动作 |
| --- | --- |
| A | 起播日志 `sub-font-size="40"`；面板重置后仍 40；ASS 三态可切 |
| B | 1080p 截图量底距 ≈5%；`sub-pos` + 到 100 后不再增长 |
| C | 改样式 → 杀进程 → 重进日志回放新值；A/B 两片源 delay 互不影响 |
| D | `files/subtitles/` 出现文件；重复导入同内容不新增文件 |
| E | 颜色/背景框/加粗/阴影在文本字幕截图上可见 |
| F | 主次双轨同屏截图；次字幕位置靠上 |
| G | 面板行显示强制/听障徽章与中文语言名；内嵌列表有「关闭」 |
| H | 长按外挂行 → 确认 → 轨道消失；当前轨被删后主轨兜底正确 |
| I | 软解/旧内核起播出现一次"不支持字幕"提示 |

## 5. 返工修复记录（第一波验收 FAIL-1/2/3，2026-11-19）

> 证据目录：`docs/evidence/p9-subtitle-fix/`（每个缺陷 = 根因 + 修复 + 真机证据）。

### 5.1 FAIL-1 · 步进只生效一次（共享组件）

- 根因：`PlayerStepperRow.StepperHalf` / `PlayerToggleRow` 的 `pointerInput(key)` 协程
  闭包冻结首帧 `onClick`；item 快照每帧换新 lambda，但手势协程不重启 ⇒ 第二次起写回
  仍是旧值。字幕步进另有读-算-写竞态：`handleSubtitleStep` 读的是 item 快照，
  连续点击会丢步。
- 修复：`PlayerStepperRow.kt` 用 `rememberUpdatedState(onClick/onToggle)` 在手势协程里
  读最新 lambda（键只留 `enabled`）；语义 `onClick` 每次重组拿新值。
  `SubtitlePanelActions.handleSubtitleStep` 在 `synchronized(subtitleStepLock)` 内
  `mpv.readProperty` 读内核**实时值**再算下一档（不再信 item 快照）；
  `panelRevision` 改为 `mutableStateOf`，暂停态也能触发面板行重载。
- 回归：音频延迟（同一组件、非字幕面板）连点 8 次，kernel 逐 tick 0.05→0.40，重置归 0。

### 5.2 FAIL-2 · 显示设置卡重置行不可达

- 根因：面板壳固定 `heightIn(max=330/358dp)`，锚点在顶部时卡底伸出屏幕；重置行原本
  在滚动列的末端，滚到最大仍在屏外（content-desc 有、bounds 全 0）。
- 修复：`PlayerOverlayPanel.kt` 按锚点算 `availableHeight`（屏高 − 锚点位置 − 边距）
  传给壳的 `heightIn`；`PlayerDetailCard.kt` 让滚动区底部 `padding(bottom=resetReservedBottom)`
  给重置行让位，重置行 `BottomCenter` 固定在卡底、不随滚动走（跳过片段卡 showReset=false 不变）。
- 证据：卡内滚动到底重置行 bounds 非零、可点；重置后 kernel + prefs 回到默认
  （40/100/40/默认色/间距 0/源样式）。

### 5.3 FAIL-3 · 导入字幕不拉起 SAF 选择器

- 根因：`SubtitlePanelActions.SUBTITLE_PANELS` 漏了入口面板 kind `"subtitle"`；
  `导入字幕`(sub-import) / `关闭字幕`(sub-off) 只出现在入口面板，Router 先判面板集合、
  不在则静默 no-op —— 点击被吞，无 toast、无 trace。
- 修复：`SUBTITLE_PANELS` 加入 `"subtitle"`；`PlayerSubtitleImport.kt` 补 launch/落盘/
  失败埋点；`MpvNative.lastCommandError` 记录 JNI 异常原话，失败 toast 不再只有一个布尔。
- 证据链：launch trace + DocumentsUI top → 落 `files/subtitles/<md5-12>-<name>`
  （deduped=false/true）→ force-stop 后文件与 `subtitle_imports` 登记仍在 →
  长按外挂行确认删除：`sub-remove ok ... disk=true`、磁盘/登记双清、
  `sid` 由 38 回落到内嵌轨 1。
- 已知边界：导入轨是会话级（mpv 轨道不跨进程自动重挂，设计未承诺）；
  片源 EOF 时 `sub-add` 被内核拒绝（内核约束，走失败 toast）。

## 6. 后续登记（仅登记，不启动，2026-11-20）

> 用户明确：以下仅登记，不在本轮启动；开工前按 §2 家族树重新核实落位与依赖。

### 6.1 本轮审计遗留（待决策 / 后续）

- ~~**SUB-M5 · 次字幕延迟行**~~ —— **DONE 2026-11-21（接手线）**，口径经用户拍板 = **按片源记忆**（同 `SubtitleSourceKey`、独立键前缀 `s:`）。行可见 / 0.1 s 步进 / 属性回读 / 重启复播复现 / 换片归零 / 重置双清，全部真机 PASS；实现与读数见 §6.8 与 `docs/evidence/p9-subtitle-reaccept/M5-DEVICE.md`。
- **面板 v2 审查低项**（不与字幕 Wave 混做）：低-1 子内容裁剪（`PlayerPanelSurface` / `PlayerPanelScroll` / `PlayerPanelPress`）、低-4 `CastSheet` 硬编码字号/圆角、低-5 滚动期每帧分配、低-2 描边可见宽度 ≈0.5dp 与「1dp」名义值差异。均属视觉/性能优化，需独立真机验收。

### 6.2 AI 字幕第二阶段

- **状态**：登记（未启动）
- **目标**：在已有 AI 字幕设置与 p265 证据基础上，明确第二阶段范围（生成/翻译/缓存/失败可见），不改现有字幕平面契约。
- **落位**：`ui/design/settings/SettingsAiSub.kt` + `docs/design/subtitle-translation-landing.md`；服务端/密钥路径待调研后补。
- **验收**：待第二阶段设计拍板后补；至少含失败非静默、密钥不落明文、离线/在线行为可区分。
- **依赖**：字幕平面 Wave1 收口（本轮）。

### 6.3 Wave2 · minSdk 28 + libmpv 重编

- **状态**：登记（未启动）
- **目标**：`minSdk 26 → 28`；libmpv 重编开启 `-Diconv` / `-Duchardet`，消除 GBK 等编码字幕的乱码路径。
- **落位**：`android/app/build.gradle.kts`（`minSdk`，当前 26，第 15 行）；`android/app/buildscripts/mpv/build-libmpv.sh`（编译开关）；编码回退逻辑在字幕导入/内核选项侧待定位。
- **验收**：GBK/Shift-JIS 样例字幕真机不乱码；UTF-8/ASS 回归；记录 APK 体积与冷启动变化。
- **依赖**：确认目标设备最低 API 覆盖（主测 b6e950ee 满足）。

### 6.4 Wave3 · 编码/字体/网络字幕

- **状态**：登记（未启动）
- **目标**：`sub-codepage` 显式选项、SC 字体随包/回退、网络字幕发现与下载（搜索源、缓存、网络失败提示）。
- **落位**：`android/app/src/main/cpp/mpv/mpv_sub_options.cpp`（`sub-codepage`）；字体资产目录待定（不得与系统字体冲突）；网络字幕发现需新模块 —— 必须先出家族树再实现，禁止平铺。
- **验收**：编码强制覆盖生效；缺字回退可读；网络搜索失败/超时/无结果均可见且不阻塞播放。
- **依赖**：Wave2 完成。

### 6.5 2026-11-21 独立复查发现与处置（字幕修复轮）

> 背景：11 项字幕/审查修复 + M1/M2 补漏已编译（23:56 APK），但真机复验执行失败（无原因、无证据文件）；
> 独立静态审查结论「需要修」。以下为本轮处置登记，修复完成并真机取证后逐条销项。

- **R1（已修复+真机 PASS，见 §6.6）· M1 复用分支绕过主/次互斥**：`PlayerSubtitleImport.kt` 复用已加载轨时直接
  `selectTrack`，若该轨正是 `secondary-sid` ⇒ `sid == secondary-sid` 确定性重影。
  修法对齐 `SubtitleTrackActions.pickMainTrack`：先关次字幕，失败则中止并如实提示。
- **R2（已修复，见 §6.6）· L1 重置未与步进同锁**：重置的内核写 + 落盘未纳入 `subtitleStepLock`，
  与长按连续步进可交错。修法：整段纳入同一把锁（不新增锁、不改锁序）。
- **R3（已修复，见 §6.6）· 导入登记表读改写无锁**：`SubtitleImportStore.remember/forget` 无互斥，
  并发可丢登记、极端顺序误删刚复用文件。修法：类级锁串行化（含跨源引用检查与删盘）。
- **R4（加固）· 路径比较规范化**：`external-filename` 与 `File.absolutePath` 可能因
  `/data/user/0` vs `/data/data` 等规范化差异不相等 ⇒ M1/M2 静默降级。统一 canonical 比较、
  异常回落原字符串；**真机必须先确认该属性非空**。
- **R5（已裁定：不扩展，见 §6.6）· M4 兜底空档**：内核若自选到次轨、或删除次轨后 `secondary-sid` 残留，
  现兜底不覆盖。先在删除成功后记录 `sid`/`secondary-sid` trace，真机取证后再决定是否扩展兜底。
- **R6（已裁定）· 资源卡 meta 图标改色**：C 中-2 顺带把两枚图标改为与相邻文字同色，
  按 §15.1「源卡同族参数」判在范围内，**保留不回退**。
- **复验注意**：HyperOS `adb install` 弹窗 9 秒自动拒绝，需安装同时点「继续安装」
  （1080×2400 约 347,2111，见 `agents.md` §0.0）；上一轮复验失败原因未明，重试时先排除此项。

### 6.6 2026-11-21 修复轮收口（真机复验 ROUND2）

> 证据：`docs/evidence/p9-subtitle-reaccept/ROUND2-DEVICE.md` + 29 个 `R2-*` 文件。
> 被测 APK 2026-11-21 01:55:59（67,880,299 B），覆盖安装 b6e950ee（lastUpdateTime 02:00:50，
> firstInstallTime 不变）。安装弹窗对策生效，上一轮"无原因失败"未再现。

**销项（真机 PASS）**：

- **R1**：次轨=dup-a 时再导入同文件 → `reuse … closedSecondary=true`，内核先
  `secondary-sid="no"` 再 `sid="1"`，全程无主次同轨时刻，画面无叠印。
- **R2/R3**：代码落点复核（重置 `synchronized(subtitleStepLock)`；ImportStore 类级
  `lock` 三处临界区）；并发注入无构造通道，**互斥行为本身未真机取证**（静态+编译证据）。
- **R4**：canonical 比较在真机生效 —— `external-filename` 非空（复用命中、全量 trace
  `dedupe unavailable` 计数 0），`/data/user/0` 路径与登记路径对齐。
- **M1**：同内容改名导入 → 文件数不增（dup-a mtime 未变）、外挂轨数不增、内核无第二次
  `sub-add`。
- **M2**：唯一文件删除 `disk=true`（文件+登记双清）；跨片源共享文件删除 `disk=false`
  （文件保留，仅清本片源登记）——两个分支均实测。
- **L1**：改 4 项后重置 → 内核回读 40/100/40/#FFFFFFFF、prefs 同步、暂停态行立即刷新。
- **L5**：「底部边距 / 仅文本字幕」两行 90px < 64dp 行距，无裁剪。
- **面板 #4（C 低-3 拆分后首次上机）**：步进按住 ≈12%（59,62,67）、控制栏文字钮纯黑区
  按住 (20,20,20)=精确 8%（非 12% 的 31）。

**R5/M4 裁定：不扩展兜底，维持现状。** 依据：删次轨 B 后内核无 `secondary-sid` 残留
（UI 回读「关闭次字幕（当前未启用）」勾选态）；删唯一主轨后兜底正确落 `sid=no`。
未覆盖场景：「主 A + 次 B 时删 A」的内核自选行为（本轮先删了 B）。该场景若内核自选到
B 会跳过现有兜底 ⇒ 登记为观察项 O2，出现实证再修，不盲改。
**O2 已取证（2026-11-21）**：`sub-remove id=2 … sid=1 sec=no` —— 内核自选到原次轨并把次位清空，
无同轨叠印，**裁定维持不扩展**（证据 `docs/evidence/p9-subtitle-reaccept/O2-DEVICE.md`）。

**取证通道缺陷（登记）**：`KivoSub` 走 `Log.i`，本机 MIUI logcat 全空（`-d` 0 行、
实时流 60s 0 字节）⇒ M4 专用日志在真机不可读，本轮以 `sub-remove ok` trace + 内核
`Run command`/`Track removed` 作等价证据。**后续任何字幕埋点一律走 smbTrace
（p220-status.log），不用 android.util.Log**；`MpvSubtitleControl.kt:106` 的 KivoSub
行待下次顺手改动时迁移（单行，不值得单独开轮）。

**新观察项（仅登记，不启动）**：

- **O1 · 次字幕面板锚点缺失**：`PlayerPanelHost.anchorFor` 无 `"subtitleSecondary"`，
  该子面板回落 Center 渲染在屏幕中央而非字幕图标下。UI 几何变更，修前先按面板锚点
  家族确认口径，再补映射 + 真机截图验收。
- ~~**O2 · M4 剩余场景**~~ —— **2026-11-21 已实测销项**（证据 `O2-DEVICE.md`）：
  主 A（外挂 id=2）+ 次 B（内嵌 id=1）时删 A，内核 `Track removed` 后**自选到 B** 并
  `Set property: sid="1"`，同时 `secondary-sid` 被清成 `no` ⇒ **不产生主次同轨**。
  现有兜底（`current == trackId` 才动作）跳过无碍，**维持不扩展**。
- **O3 · 顶栏按压圆有效亮度 ≈5%**：`TopIconButton` 整 Box 套 `barInactiveAlpha=0.62`，
  8% 按压圆合成后 ≈5%（底栏文字钮为精确 8%）。若设计要求顶栏也贴 8%，需把
  PressIndicator 移出 alpha 容器；属设计口径决策，未拍板前不动。
- **O4 · 长按底栏播放键触发锁屏浮层**：面板按压测量已改在「倍速」/顶栏图标完成；
  该行为是否合意待确认。

**未取证清单（无构造手段，非阻塞）**：3c 同文件多轨（M1 复用使 UI 无法构造
shared>1；跨片源保护已实测）、7 失败注入、4b 重置×步进并发。

**工作区状态**：git 脏区仍 127 项（其他助手改动未触碰）；未 commit/push/发布；
设备夹具已清理、样式已重置回默认、`svc power stayon` 已恢复。

### 6.7 2026-11-21 接手 · O1 销项（次字幕面板锚点）

> 类型：**执行遗漏的补全，不是新设计**。`PlayerPanelHost.kt` 的类注释在 O1 之前就已写明口径
> 「钻入子面板（如 audio → audioDelay）自动跟随父面板的图标，锚点不会跳走」；
> `subtitleSecondary` 只是漏登记了映射，于是 `anchorFor` 的 `else` 把它送进 `Center`。

- **落位（唯一改动点）**：`ui/compose/player/PlayerPanelHost.kt` → `anchorFor` 的 `"captions"`
  分支补 `"subtitleSecondary"`。不新增文件，不改 `PlayerPanelRouter` / `PlayerOverlay` / 锚点几何。
- **为什么归字幕图标**：`subtitleSecondary` 的唯一入口是「字幕」卡的第三行
  （`SubtitlePanelRows.entry()` 里 `tag = "subtitleSecondary"`），触发控件就是顶栏 `captions`
  （`TOP_ACTIONS` 下标 6）。与 `subtitleTracks` / `subtitleExternal` / `subtitleDisplay` 同族。
- **几何判据**（1080×2400 / 440dpi ⇒ 横屏 872.7dp 宽；`topBarHeight=74`、`anchorGap=8`、
  `actionPitch=50`、`actionHitBox=44`、`iconRowPad=8`、`panelWidth=248`、`edgeGap=16`）：
  - 面板顶边 y = `topBarHeight + anchorGap` = **82dp ≈ 225.5px**（修复前 Center：竖直居中）；
  - 面板左缘 = `clamp(W−80−124, 16, W−248−16)` = **608.7dp ≈ 1674px**
    （顶栏一侧被右 clamp 吃住，这是既有口径，不是新偏差）；
  - 与「字幕」卡同锚点：两张面板顶边与左缘一致（Δ ≤ 2px）。
- **验收**：真机 `b6e950ee`、`--ez kivo.freeze true` 起播 → 顶栏 captions → 字幕卡 →
  「次字幕」行 → 截图 + Python/PIL 采样面板边界（不用 Read 读图）。
  证据目录 `docs/evidence/p9-subtitle-reaccept/O1-*`。
- **状态**：**PASS（2026-11-21 真机）**。读数：字幕卡 / 次字幕面板顶边同为 226px = **82.18dp**（期望 82dp）、左缘 1674px = **608.73dp**（期望 608.7dp）、宽 682px = 248dp；次字幕面板与「字幕」卡同锚点，不再落 Center。证据：`docs/evidence/p9-subtitle-reaccept/O1-DEVICE.md` + `O1-0{1..5}*.png`（`O1-fixture.srt` 为同名外挂轨夹具）。
  - 边界：Center 对照未实机回退（修复前状态由 `anchorFor` 的 `else` 静态证明）；片源无内嵌字幕轨，完整列表用 `sub-auto=exact` 同名外挂轨构造；面板底边未逐像素量（非判据）。

### 6.8 2026-11-21 接手 · SUB-M5 销项（次字幕延迟行 + 按片源记忆）

**用户拍板（2026-11-21）**：次字幕延迟的持久化口径 = **按片源记忆**（与主 `sub-delay` 一致，
键来自 `SubtitleSourceKey`），不是会话级。

- **读侧**（`ui/compose/player/subtitle/SubtitlePanelRows.kt`）：`display()` 增读
  `secondary-sub-delay`（缺失即 `notReady`），并在「次字幕位置」之后、 「显示次字幕」之前
  加一行 `stepper("次字幕延迟", "secondary-sub-delay", …, " s", digits=1, SECONDARY_SECTION)`。
- **写侧**（`SubtitleStyleActions.persistNumeric`）：`secondary-sub-delay` 分支在写内核之外
  **同时记忆**（`SubtitleDelayMemory.saveSecondary(subtitleSourceKey(), value)`）。
- **记忆**（`playback/subtitle/SubtitleDelayMemory.kt`）：同一 prefs（`subtitle_delay`），
  主键前缀 `d:`（不变）、次键前缀 **`s:`**；新增 `loadSecondary` / `saveSecondary`，
  夹取与空键语义与主延迟完全一致。
- **起播回放**（`playback/MpvStartSequence.kt` + `SubtitleStyleApplier.applyAtStart`）：
  新增 `secondaryDelaySeconds` 参数，非 0 才写 `secondary-sub-delay`，失败项并入同一 failures 列表。
- **重置**（`SubtitlePanelActions.resetSubtitlePanel`）：在 `subtitleStepLock` 内一并写
  `secondary-sub-delay=0`，两项都成功才落盘（主/次记忆同时清），失败项汇总点名。
- **探针**：`SubtitlePanelProbe` 已把 `secondary-sub-delay` 走 `persistNumeric`，改后自动获得记忆行为。
- **验收**：面板行可见 → 步进 0.1 s → 属性回读一致 → 重启进程复播同片源仍为该值 → 换片源回 0。
