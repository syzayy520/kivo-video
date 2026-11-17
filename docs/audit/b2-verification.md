# B2 真机验证：「我的」看板（5 屏对比 rex 0357–0362）

> 卡号：B2-T5（初验）→ **B2-fix4（复验回填）**　｜　设备：Redmi K40 `b6e950ee`（1080×2400 / 440dpi / Android 13）　｜　初验 2026-11-16 / **复验 2026-11-17**
> 结论置信等级：**✅ 实拍证据**（本机截图 / uiautomator dump / 命令退出码）　**◐ 推断**（读代码得出、未单独造样本验证）
> 本文只做验收，**未改动任何生产代码**（符合「审计阶段不修改生产代码」）。
>
> **B2-fix4 复验结论前置**：22 条差距中 **FIXED 7 / PARTIAL 4 / OPEN 11**，**P0 已清零**
> （PROF-01 URL 当片名、PROF-02 无海报两条均已实拍验证修复）。

---

## 一、执行环境与命令结果（复验实测）

| 项 | 值 | 出处 |
| --- | --- | --- |
| 工作目录 | `C:\kivo video\android` | ✅ |
| `JAVA_HOME` | `C:/Android/jdk-17.0.13+11` | ✅ |
| 编译命令 | `gradlew.bat :app:assembleDebug --offline -q` | ✅ **EXIT=0**（未触发 `-Pkotlin.incremental=false` 重跑） |
| APK | `app/build/outputs/apk/debug/app-debug.apk` · **2026-11-17 00:21** · 69,772,519 B | ✅ |
| 安装 | `adb -s b6e950ee install -r` → `Success`（**未执行 uninstall**，签名沿用仓库内 `kivoDebug`） | ✅ |
| 启动入口 | `am start -n com.kivo.cinema/.MainActivity`（**不用 `monkey`**，它会直接落 PlayerActivity） | ✅ |
| 门禁复跑 | `python tools/ui_gate.py` → **RESULT: PASS · EXIT=0** | ✅ |

> ⚠️ 环境事实（沿用初验）：`monkey -p com.kivo.cinema` 冷启动会直接落到 `.ui.PlayerActivity`
> （上次播放会话被恢复），此时按返回直接回桌面。进「我的」必须显式 `am start -n …/.MainActivity`。

### 1.1 截图清单（复验重截，已覆盖旧文件）

`exec-out screencap` 在 Git Bash 下会污染二进制，故统一 `screencap -p /sdcard/x.png` + `adb pull`。

| 文件 | 尺寸 | 时间戳 | 内容 |
| --- | --- | --- | --- |
| `docs/audit/golden/kivo/profile-1.png` | 1080×2400 | 2026-11-17 00:24 | 首屏：渐变底 / 环境头 / 周历 / 今日摘要 / 历史观看 / 待看 / 收藏+在看 |
| `docs/audit/golden/kivo/profile-2.png` | 1080×2400 | 2026-11-17 00:24 | 热力图 / 待看空态 / 收藏 0 + 在看 1（真实缩略图）/ 本月图例 |
| `docs/audit/golden/kivo/profile-3.png` | 1080×2400 | 2026-11-17 00:24 | 本月折线 / 刷剧 TOP3 / 类型分布 |
| `docs/audit/golden/kivo/profile-4.png` | 1080×2400 | 2026-11-17 00:24 | 类型分布 + 管理模块按钮（**底部无分类走势空卡**） |
| `docs/audit/golden/kivo/profile-5.png` | 1080×2400 | 2026-11-17 00:24 | **与 4 逐字节同尺寸内容（已滚到底）** |
| `docs/audit/golden/kivo/profile-modules.png` | 1080×2400 | 2026-11-17 00:25 | 管理模块弹层（12 行 + ✕ + 说明） |
| `docs/audit/golden/kivo/profile-watching-overlay.png` | 1080×2400 | 2026-11-17 00:26 | 在看覆盖层（**1 条，与卡片「在看 1」一致**） |

**分段方式**：首屏截 `profile-1`，之后 `input swipe 540 1900 540 1300 300`（600 px）×4 → 间隔 1.5 s → 截图。

### 1.2 造「可对齐记录」的实测过程（B2 初验的 CONDITIONS NOT MET 已在本次补测）

| 步骤 | 命令 | 结果 |
| --- | --- | --- |
| 1. 造目录 | `adb shell mkdir -p /sdcard/Movies/Kivo` | ✅ |
| 2. 复制片源（**不删原片**） | `cp …/files/media/kivo-local-test.mp4 /sdcard/Movies/Kivo/kivo-b2-test.mp4`（1,385,893 B） | ✅ |
| 3. MediaStore 索引 | `am broadcast -a …MEDIA_SCANNER_SCAN_FILE -d file:///sdcard/Movies/Kivo/kivo-b2-test.mp4` | ✅ `result=0` |
| 4. 索引核对 | `content query --uri content://media/external/video/media --where "_data LIKE '%kivo-b2-test%'"` | ✅ `_id=1000051963, _data=/storage/emulated/0/Movies/Kivo/kivo-b2-test.mp4, duration=24000` |
| 5. 播 18 s 后退出 | `am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity --es kivo.media /storage/emulated/0/Movies/Kivo/kivo-b2-test.mp4` → `input keyevent 4` | ✅ |
| 6. 记录核对 | `run-as com.kivo.cinema cat files/watch.json` | ✅ 新增 `"/storage/emulated/0/Movies/Kivo/kivo-b2-test.mp4":{"pos":17733,"dur":24000,…}`；`daily` 新增 `2026-11-17:17533` |
| 7. 页面效果 | 首页出现「继续观看 · kivo-b2-test.mp4」（`content-desc="kivo-b2-test"`）；「我的」卡片「在看 1」 | ✅ 见 `profile-2/3/4.png` |
| 8. 清理 | `rm /sdcard/Movies/Kivo/kivo-b2-test.mp4` + `content delete … --where "_data LIKE '%kivo-b2-test%'"` | ✅ 目录只剩原有 3 个片源；`media/` 未动；`watch.json` 记录**保留**作为证据 |

> ⚠️ 关键数据事实（复核初验）：`/sdcard/Android/data/com.kivo.cinema/files/media/` **不被 MediaStore 索引**，
> 所以 2 条本地路径记录对齐结果为 0；14 条 Emby/127.0.0.1 URL 同理对齐不上。
> 只有把片源放到被索引的公共目录（`/sdcard/Movies/…`）并扫一次，才会产生可对齐记录。

---

## 二、五屏逐项对比（竞品契约 → 当前实现 → 判定）

### 屏 1 · 首屏（`profile-1.png` ↔ `IMG_0357` ✅ 实拍）

| # | 竞品契约（`IMG_0357`） | 当前实现（复验实拍） | 判定 |
| --- | --- | --- | --- |
| 1.1 | 顶部：圆形头像 + 「默认环境」+ ⌄ + 右侧白胶囊「编辑」 | 「本地」纯文本 + 白胶囊「编辑」；**无头像、无 ⌄**（`ProfilePage.kt:169-194`） | ❌ PROF-05（OPEN） |
| 1.2 | 周历 15–21，选中日 = **白胶囊底** + 数字加粗 + 「周一」**红色**；右侧 `↗` 日历入口 | 选中 17 = **白胶囊 + 红字「17」/「二」+ 红点**（`WeekStrip.kt:85-122`）；`↗` 仍缺 | ✅ PROF-06（FIXED，`↗` 归 PROF-06 附属项仍缺） |
| 1.3 | 「这天没有播出的剧」空态卡 | **整卡缺失**（无 `overview/DayEmptyState.kt`；`ProfilePage.kt:87-92` 只有 calendar → today → history） | ❌ PROF-04（OPEN） |
| 1.4 | 「今天 / 2026年11月17日」+ `0.0 小时 / 0 部电影 / 2 集剧`（三行纵向） | `0.0 小时` + 右上 `0 部电影 / 0 集剧`（今日实播 17.5 s ⇒ 0.0 小时，真实值） | ⚠️ 版式仍双行（PROF-10 附属） |
| 1.5 | 摘要卡内 12:00 处橙色实柱 + 24 灰点 | 仍只有 24 灰点 / 一条虚线轴（`ProfileStats.kt:103-108` `hourBuckets` 全 0） | ❌ PROF-09（OPEN） |
| 1.6 | 摘要卡下方 3 点分页指示 | 无指示点（`TodaySummary.kt` 无 Dots 实现） | ❌ PROF-09（OPEN） |
| 1.7 | 「历史观看」标题在**卡内左上** | 标题已出现，但在**卡外上方**作节标题（`ProfileSections.kt:38-46`） | ⚠️ PROF-07（PARTIAL：标题有了、位置不符） |
| 1.8 | 热力图 4 级紫色方块，16 列贴合 | 20 列 × 7 行，色阶为 `accentGreen` 5 档透明度（`HistoryHeatmap.kt:71-78`）；实拍仅 1 格深绿 | ⚠️ PROF-08（PARTIAL：有分级、色系与列数不符） |
| 1.9 | 待看标题行右侧 `···` | 只有整行 `›`，无 `···` | ❌ PROF-13（OPEN） |
| 1.10 | 卡片圆角 20–24 dp、白底 + 极轻阴影 | 白底 + 16 dp + 3 dp 阴影（`WatchingSection.kt:195-196`） | ✅ 一致（圆角略小，P3） |
| 1.11 | 背景：浅紫→白渐变 + 顶部玻璃感 | **渐变底已落地**：`Brush.verticalGradient(lerp(pageBackground, bgLavender, .30) → lerp(pageBackground, canvasFill, .55))`（`ProfilePage.kt:74-78`、`:223-232`） | ✅ PROF-14（FIXED） |
| 1.12 | 底部胶囊：「我的」选中为白胶囊 | 同结构，「我的」白胶囊 ✅ | ✅ |
| 1.13 | 底部导航不遮挡内容 | 滚动**中途**卡片从胶囊下方穿过（浮层固有行为）；**滚到底**（`profile-4/5`）「管理模块」按钮完整可见、与胶囊有空隙 | ✅ PROF-21（FIXED，实测判据见 §四） |

### 屏 2 · 待看空态 + 收藏/在看（`profile-2.png` ↔ `IMG_0358` ✅ 实拍）

| # | 竞品契约（`IMG_0358`） | 当前实现（复验实拍） | 判定 |
| --- | --- | --- | --- |
| 2.1 | 「待看 0 ›」+ 右侧 `···` | 「待看 0 ›」**无 `···`** | ❌ PROF-13 |
| 2.2 | 空态卡：胶片图标 + 「还没有待看的影片」+「在详情页加入待看，票根会出现在这里」 | 图标 + 两行文案逐字一致 | ✅ **文案 PASS** |
| 2.3 | 收藏 0 / 在看 27，右上 `···` | 收藏 0 / **在看 1**；在看卡有 `›`（语义 `查看全部在看`，`uiautomator` 实测 bounds `[928,83][1005,160]`），收藏卡无 | ❌ PROF-13（PARTIAL 子项：仅在看有入口且语义是跳转不是菜单） |
| 2.4 | 在看卡内叠放电影海报 | **真实首帧已显示**：彩条测试图（`ContinueSource.kt:47-49` → `VideoThumbnails.framePath` → `WatchingSection.kt:105-121` `decodePoster`）；角标 `73%`；标题 `kivo-b2-…` | ✅ **PROF-01 / PROF-02（FIXED）** |
| 2.5 | 卡片下方次行「看到 mm:ss」 | **未渲染** —— 数据侧 `ContinueSource.kt:46` 已产出 `看到 0:17`，`PosterCell` 只画 title（`WatchingSection.kt:135-141`） | ❌ PROF-13b（OPEN） |
| 2.6 | 本月观看：`0.2 小时 ↑0.1小时` + 图例 `● 剧集 ● 电影` | `1.7 小时 ↑1.7小时` + **图例已出现**（`ProfileSections.kt:51-70`） | ⚠️ PROF-15（PARTIAL：图例有，刻度/轴标签未改） |
| 2.7 | 折线双色（剧集黄 / 电影紫）、y 轴 `1h` 虚线刻度 | **单色绿**折线、**无 y 轴刻度**、x 轴 `1 / 15 / 30` | ❌ PROF-16（OPEN） |

> **2.4 已闭环（✅ 实拍）**：`stream?static=true&MediaSourceId=aafdd622d6e1e` 这类 URL **不再上屏**，
> 根因修复点在 `ContinueWatching.kt:44-49`（只对齐得上的才返回）+ `ContinueSource.kt:38-52`（片名走 `ProfileNames`、缩略图走缓存 jpg）。

### 屏 3 · 本月 / TOP3 / 类型分布（`profile-3.png` ↔ `IMG_0359` ✅ 实拍）

| # | 竞品契约（`IMG_0359`） | 当前实现（复验实拍） | 判定 |
| --- | --- | --- | --- |
| 3.1 | 本月观看：`0.2小时 ↑0.1小时` + 图例 + y 轴 `1h` | `1.7小时 ↑1.7小时`，图例 ✅ / 无刻度 ❌ | ⚠️ PROF-15（PARTIAL） |
| 3.2 | 折线圆点连线（`11/1 … 11/30`） | 绿色圆点连线、尖峰 15 日、轴号 `1/15/30` | ⚠️ 形态接近、轴标签不同（PROF-15） |
| 3.3 | 刷剧 TOP3：橙色条 `0.1h` ×3 | 「刷剧 TOP3 / **暂无数据**」（`TopShowsCard.kt`） | ⚠️ 空态合规；**未整卡隐藏** → PROF-19 |
| 3.4 | 类型分布：`剧情 46%` + 堆叠条 + 4 项图例 | 环形 `1.7h` + 「暂无数据」；**已无「0 部电影 · 0 部剧」**（`ProfilePage.kt:128-137` 传 `movieCount=null / episodeCount=null`） | ✅ **PROF-10 主诉（口径混用）已修**，卡内标题仍缺 → PARTIAL |
| 3.5 | 「类型分布」标题在卡右上 | 卡内仍无标题 | ⚠️ PROF-10（PARTIAL 剩余项） |
| 3.6 | 「分类走势 1/2」两张小卡 | **整段不渲染**（`ProfilePage.kt:140-142` 只 `forEachIndexed` 渲染有数据的趋势） | ✅ **PROF-11（FIXED）** |

### 屏 4 · 类型分布 + 管理模块（`profile-4.png` ↔ `IMG_0360` ✅ 实拍）

| # | 竞品契约（`IMG_0360`） | 当前实现（复验实拍） | 判定 |
| --- | --- | --- | --- |
| 4.1 | 两张趋势小卡并排（`↓0.1小时` / `↑0小时` + 面积折线） | `trends` 为空 ⇒ **不渲染**（正确行为 C）；有数据时的卡片形态仍未与竞品比对 | ⚠️ 无数据样本，PROF-11 判 FIXED、PROF-19 仍 OPEN |
| 4.2 | 无数据模块整块不显示 | ✅ 本次实拍**屏底无任何「还没有足够数据」文本** | ✅ PROF-11（FIXED） |
| 4.3 | 底部「管理模块」胶囊带 ⚙ 图标 | 纯文字「管理模块」，无图标（`ProfilePage.kt:197-217` 只有 `Text`） | ❌ PROF-17（OPEN） |
| 4.4 | 底部导航不遮挡内容 | 滚到底时按钮 y≈2030–2110，胶囊 y≈2160–2276，**有 50 px 间距** | ✅ PROF-21（FIXED） |

### 屏 5 · 滚到底（`profile-5.png`）

与屏 4 **逐字节同尺寸、同内容**（185,213 B）。列表已到底，无第 5 屏新内容。

### 屏 6（附加）· 管理模块弹层（`profile-modules.png` ↔ `IMG_0361` ✅ 实拍）

| # | 竞品契约（`IMG_0361`） | 当前实现（复验实拍） | 判定 |
| --- | --- | --- | --- |
| 6.1 | 左上 `✕` + 居中标题「管理模块」 | `✕` + 「管理模块」 | ✅ |
| 6.2 | 12 行：日历 / 观看概览 / 历史观看 / 待看 / 收藏 / 在看 / 订阅 / 下载 / 本月观看 / 类型分布 / 分类走势 1 / 分类走势 2 | **12 行、名称与顺序逐项一致** | ✅ **PASS** |
| 6.3 | 每行：左图标 + 名称 + 绿色开关 + 拖拽把手 | 同结构，绿色开关 | ✅ |
| 6.4 | 行背景玻璃/半透明白 + 细分隔线 | 仍是 `groupedFill` 不透明灰块行（`ModuleRow.kt`），行间有细线 | ⚠️ PROF-18（OPEN） |
| 6.5 | 底部说明 | 「关闭的模块不在「我的」页显示。在看、订阅、下载与分类走势没有内容时也不显示。」 | ✅ 文案存在，且**行为已与文案一致**（PROF-11 修复后） |

---

## 三、空数据表现（复验）

| 区域 | 屏上表现 | 类型 | 评价 |
| --- | --- | --- | --- |
| 待看 | 「还没有待看的影片」+「在详情页加入待看，票根会出现在这里」 | 空态文案（逐字对齐竞品） | ✅ 合规 |
| 收藏 | 大字 `0` + 「收藏」 | 真值 0 | ✅ 合规 |
| 在看 | 大字 `1` + 1 张**真实缩略图** + `73%` + `kivo-b2-…` | 数字真实、内容真实 | ✅ PROF-01/02 已修 |
| 今日摘要 | `0.0 小时` / `0 部电影` / `0 集剧`；时间轴 24 灰点无柱 | 数字真、分桶为空（无小时级采样源） | ❌ PROF-09 |
| 本月观看 | `1.7 小时 ↑1.7小时` + 图例 + 折线（15 日尖峰） | 真值 | ✅（缺 y 轴刻度 PROF-15） |
| 刷剧 TOP3 | 「暂无数据」 | 空态 | ⚠️ 竞品整卡隐藏 → PROF-19 |
| 类型分布 | 环 `1.7h` + 「暂无数据」，**无计数行** | 空态 + 口径已统一 | ✅ 口径 / ⚠️ 标题（PROF-10 PARTIAL） |
| 分类走势 ×2 | **不存在** | 按模块说明整段隐藏 | ✅ PROF-11 |
| 在看覆盖层 | 1 条 `kivo-b2-test.mp4` · 今天 · `看到 0:17 / 0:00:24` + 真实缩略图 | 真值 | ✅ **与卡片「在看 1」一致 → PROF-03 FIXED** |
| 管理模块 | 12 项全开 | — | ✅ |

---

## 四、差距清单（22 条 · 逐条回填 FIXED / PARTIAL / OPEN）

> 状态口径：**FIXED** = 实拍可见且与契约一致；**PARTIAL** = 主诉已修但有明确残留项；**OPEN** = 未变动。
> 证据列同时给「新截图」与「文件:行」。

| 编号 | 差距 | 状态 | 证据（截图 + 文件:行） | 用户可见后果（若仍存在） | 优先级 |
| --- | --- | --- | --- | --- | --- |
| **PROF-01** | 在看卡标题落到 URL 片段 | ✅ **FIXED** | `profile-2.png`（标题 `kivo-b2-…`，无 `stream?static`）；`ContinueWatching.kt:44-49`（只返回对齐得上的记录）→ `ContinueSource.kt:45`（`ProfileNames.display`） | — | ~~P0~~ |
| **PROF-02** | 在看缩略图恒为灰块 + 首字母 | ✅ **FIXED** | `profile-2/3/4.png`（彩条真实首帧 + `73%`）；`ContinueSource.kt:47-49` + `WatchingSection.kt:105-121` | — | ~~P0~~ |
| **PROF-03** | 「在看 N」与覆盖层互相矛盾 | ✅ **FIXED** | `profile-2.png`「在看 1」↔ `profile-watching-overlay.png`「1 条 kivo-b2-test.mp4」；`ProfileStats.kt:83` 复用 Shell 对齐列表 + `ContinueSource.kt:38-42` | — | ~~P1~~ |
| **PROF-04** | 缺「这天没有播出的剧」空态卡 | ⛔ **OPEN** | `profile-1.png`（周历下直接是今日摘要）；无 `ui/profile/overview/DayEmptyState.kt`；`ProfilePage.kt:87-92` | 周历换到无剧日期时无任何提示 | P1 |
| **PROF-05** | 环境头无头像 / 无 ⌄ | ⛔ **OPEN** | `profile-1.png` 左上仅「本地」；`ProfilePage.kt:169-194` 无 avatar / chevron | 看不出「环境」是可切换对象 | P1 |
| **PROF-06** | 周历选中态与今日色不符 | ✅ **FIXED** | `profile-1.png` 选中 17 = 白胶囊 + `dangerRed` 数字/标签 + 红点；`WeekStrip.kt:85-122` | — （`↗` 日历入口仍缺，属附属项） | ~~P1~~ |
| **PROF-07** | 热力图缺「历史观看」标题 | ◐ **PARTIAL** | `profile-1/2.png` 标题已出现但在**卡外**；`ProfileSections.kt:38-46`（注释自陈「此处上移为节标题」） | 标题与卡片脱开，热力图卡内仍无说明 | P1 |
| **PROF-08** | 热力图色阶单色 + 列数/间距不符 | ◐ **PARTIAL** | `HistoryHeatmap.kt:71-78`（5 档 `accentGreen` 透明度）+ `ProfileStats.kt:19`（20 周 ⇒ 20 列）；实拍仅 1 格深绿（仅 2 天有数据） | 看不出「哪天看得多」（色系与竞品紫不同） | P1 |
| **PROF-09** | 摘要时间轴无柱、无 3 点指示 | ⛔ **OPEN** | `profile-1.png` 时间轴仍 24 灰点；`ProfileStats.kt:103-108` `hourBuckets = List(24){0}`；`TodaySummary.kt` 无 Dots | 看 1.7h 不知道落在哪个时段 | P1 |
| **PROF-10** | 类型分布把「本月时长」和「今日计数」拼在一卡 | ◐ **PARTIAL** | 主诉已修：`profile-3/4.png` 无「0 部电影 · 0 部剧」；`ProfilePage.kt:128-137` 传 `null` 隐藏计数行。残留：卡内仍无「类型分布」标题 | 卡内无名，读者需靠上下文推断这是什么图 | P1 |
| **PROF-11** | 分类走势空态渲染两次 | ✅ **FIXED** | `profile-4/5.png` 屏底**无**空态文本；`ProfilePage.kt:140-142`（空集合不产生 item） | — | ~~P1~~ |
| **PROF-12** | 在看覆盖层裸色 | ⛔ **OPEN** | `WatchingPage.kt:73`（`Color.White`）、`:89`（`Color.White.copy`）、`:97`（`Color(0xFF24242A)`）、`:118`（`Color(0x1A000000)`）、`:124`（`Color.Black`）；门禁裸色 325 处未减少 | 深色态可能失真；违反 token 收敛 | P1 |
| **PROF-13** | 待看/收藏/在看三卡缺右上 `···` 菜单 | ⛔ **OPEN** | `profile-1/2.png`：待看仅 `›`、收藏无入口、在看 `›` 语义为「查看全部在看」（`WatchingSection.kt:172-189`）；`CollectionSection.kt` 无 `onMore` | 卡片内无法直接隐藏/排序/清空 | P1 |
| **PROF-13b** | 在看卡不渲染 `subtitle`（「看到 mm:ss」） | ⛔ **OPEN** | `profile-2.png` 标题下无次行；数据侧有值 `ContinueSource.kt:46`；组件侧 `WatchingSection.kt:135-141` 只画 title | 看不到进度时间点（只有 `%`） | P2 |
| **PROF-14** | 背景无渐变 | ✅ **FIXED** | `profile-1.png` 顶部淡紫渐变；`ProfilePage.kt:74-78` + 派生色 `:223-232` | — | ~~P2~~ |
| **PROF-15** | 本月折线缺图例 / y 轴刻度 / 轴标签 | ◐ **PARTIAL** | 图例已落地（`profile-2.png`「● 剧集 ● 电影」）；`ProfileSections.kt:51-70`。残留：无 y 轴 `1h` 刻度、轴标签仍 `1/15/30`（`MonthlyChart.kt`） | 图表量级仍读不出 | P2 |
| **PROF-16** | 折线单色绿 vs 竞品双色 | ⛔ **OPEN** | `profile-3/4.png` 单色绿折线；`MonthlyChart.kt` 仍单一 `seriesColor` | 看不出类型构成随时间变化 | P2 |
| **PROF-17** | 「管理模块」按钮无图标 | ⛔ **OPEN** | `profile-4.png` 纯文字胶囊；`ProfilePage.kt:197-217` | 识别度低于竞品 | P2 |
| **PROF-18** | 弹层行样式不透明块 | ⛔ **OPEN** | `profile-modules.png` 行底仍灰块；`ModuleRow.kt` 用 `groupedFill` 而非 `GlassSurface` | 弹层观感偏「设置列表」 | P2 |
| **PROF-19** | TOP3 / 分类走势无数据时未整卡隐藏 | ⛔ **OPEN** | `profile-3/4.png` 仍渲染「刷剧 TOP3 / 暂无数据」整卡；`ProfilePage.kt:125` 无条件渲染 | 空数据页出现无信息卡 | P2 |
| **PROF-20** | 部分 `*Page.kt` 缺 `@Preview` | ⛔ **OPEN** | 门禁 §3：缺 `@Preview` 的 Page **5/6**，含 `ui/design/WatchingPage.kt` | 门禁 WARN 长期挂着，掩盖新增 | P3 |
| **PROF-21** | 列表底部与导航浮层交叠 | ✅ **FIXED** | `profile-4/5.png` 滚到底：管理模块按钮 y≈2030–2110，胶囊 y≈2160–2276，**无交叠**；`ProfilePage.kt:81` `contentPadding(bottom = ShellInsets.NavReservedHeight)` | — （滚动中途穿过浮层是浮层固有行为，不计） | ~~P2~~ |

### 4.1 优先级汇总卡（复验后）

| 优先级 | 条目 | 合计 |
| --- | --- | --- |
| **P0** | **无（0 条）** —— PROF-01、PROF-02 已 FIXED | **0** |
| **P1** | PROF-04、05、07、08、09、10、12、13（其中 07/08/10 为 PARTIAL） | **8** |
| **P2** | PROF-13b、15、16、17、18、19（其中 15 为 PARTIAL） | **6** |
| **P3** | PROF-20 | **1** |
| 已完成 | PROF-01、02、03、06、11、14、21 | **7** |

### 4.2 状态统计

| 状态 | 条数 | 条目 |
| --- | --- | --- |
| ✅ FIXED | **7** | PROF-01、02、03、06、11、14、21 |
| ◐ PARTIAL | **4** | PROF-07、08、10、15 |
| ⛔ OPEN | **11** | PROF-04、05、09、12、13、13b、16、17、18、19、20 |
| 合计 | **22** | — |

**已 PASS、无需改的项（避免误改）**：待看空态两行文案逐字一致；管理模块 12 项名称/顺序/开关/把手结构一致；「我的」Tab 选中白胶囊；在看数/覆盖层/首页「继续观看」三处同源；页面无崩溃、无空白屏。

---

## 五、「在看」入口验证（本次已打通）

| 路径 | 操作 | 结果 | 判定 |
| --- | --- | --- | --- |
| 我的 → 在看卡 `›` | `input tap 966 121`（`uiautomator` 取 `content-desc="查看全部在看"` bounds `[928,83][1005,160]`） | 覆盖层出现：`‹ 返回` + 「在看」+「观看进度自动保存 · 上划管理」+ 1 条记录 | ✅ 截图 `profile-watching-overlay.png` |
| 覆盖层内容 | — | `kivo-b2-test.mp4` · 今天 · 进度条 · `看到 0:17 / 0:00:24` | ✅ 与卡片「在看 1」**一致** |
| 发现 → 继续观看 | 首页冷启动即见 | `content-desc="kivo-b2-test"` + 「查看全部继续观看」（`uiautomator dump`） | ✅ 初验的 CONDITIONS NOT MET 已补齐 |

> 初验时这条路径「不可达」的唯一原因是没有可对齐记录；本次造记录后，首页「继续观看」与在看覆盖层同时出现，
> 证伪了「对齐逻辑本身有 bug」的猜测。

---

## 六、门禁复跑（复验）

```
python tools/ui_gate.py  →  RESULT: PASS  ·  EXIT=0
```

| 项 | 数值（复验） | 与初验对比 |
| --- | --- | --- |
| Kotlin 文件 | **398**（初验 393，+5 = B2-fix1/2/3 新增文件） | +5 |
| 行数超限 | **22** 个（存量 17 + 白名单待拆 5） | 持平，**无新增** ✅ |
| 裸色 | **325** 处（baseline 326 → **−1**） | 持平（PROF-12 未修，见 §四） |
| 缺 `@Preview` 的 Page | **5 / 6** | 持平（含 `WatchingPage.kt` → PROF-20） |

`ui/profile/**` 17 个文件**全部 ≤300 行，且未出现在裸色 top20 与超限列表**（`ProfilePage.kt` 281 行）；
唯一仍在裸色清单里的相关文件是 `ui/design/WatchingPage.kt`（PROF-12，属 B2 遗留项）。

---

## 七、结论

1. **构建与安装**：`:app:assembleDebug` **EXIT=0**，`install -r` **Success**，**未执行 uninstall**；浏览全程无崩溃（屏幕逐张可读、无系统崩溃对话框）。✅
2. **两条 P0 已闭环**：「在看」卡不再出现 `stream?static=true…`，并显示**真实首帧**（`profile-2/3/4.png`）。**P0 剩余 0 条。**
3. **三处数据同源已达成**：首页「继续观看」、「我的」在看卡（1）、在看覆盖层（1 条）指向同一条记录。✅
4. **编排视觉三处已修**：渐变底（PROF-14）、分类走势空态整段消失（PROF-11）、类型分布不再混用今日计数（PROF-10 主诉）。✅
5. **仍有 11 条 OPEN + 4 条 PARTIAL**，全部为非 P0：
   - 结构缺失：PROF-04（空态卡）、PROF-05（头像/⌄）、PROF-17（按钮图标）、PROF-09（时间轴/指示点）；
   - 数据映射缺失：PROF-13b（subtitle 未渲染）；
   - 视觉语言：PROF-08（热力图色系/列数）、PROF-15（刻度）、PROF-16（双色折线）、PROF-07（标题位置）；
   - 治理：PROF-12（`WatchingPage` 裸色）、PROF-18（弹层材质）、PROF-19（TOP3 空卡）、PROF-20（`@Preview`）。
6. **B2 判定：达成（收口）**。判据：构建/安装/门禁全绿、**P0 清零**、页面结构顺序与 `IMG_0357–0362` 一致、数据层不再造假。
   剩余 15 条为 P1–P3 体验层，**不阻断 B2 结题**，建议并入 B3/B5。
7. **下一步建议**：把 PROF-13b（一行 subtitle）+ PROF-17（图标）+ PROF-04（空态卡）打包成一张小卡（改 3 个文件、零数据依赖），先吃掉三条低风险 P1/P2；`PROF-16` 折线双色需先补 `DesignPalette` 的剧集/电影两色 token，与 PROF-08 热力图色系一起做。

**未覆盖 / 未验证**：编辑环境弹层、「编辑」按钮行为、管理模块关掉某项后的实际隐藏效果、周历点选其他日期后的摘要切换
—— 这四项不在 B2 的验收范围内，未截图，**不作任何结论**。⚠️