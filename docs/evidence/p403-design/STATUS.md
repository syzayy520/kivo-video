# G1 · P0 修复验证状态（403 规划第一轮）

时间：2026-11-18 02:33 GMT+8
设备：K40 `b6e950ee`（A13，用户指定机型）
性质：代码已改、已编译装包；**验证强度逐条标注，未达标项不宣称通过**。

## 编译
- `docs/evidence/p403-design/build-g1.log`：第一次 BUILD FAILED（6 处引入错误，已修）。
- `docs/evidence/p403-design/build-g1-final.log`：**BUILD SUCCESSFUL in 12s**。
- 修复过程：①DetailPage 数值插值误用 Color lerp → 改算术插值；②HeroPage 评分 Double→Float；③Grid 缺 getValue import + smart cast + formatDate 可见性 + 缺 fillMaxWidth import。

## P0-2 本地影片页 SU-01 —— ✅ 真机验证通过
- 证据：`p0-2-local-grid-after.png`
- 实测：2 列白卡、圆角、标题去扩展名（exit_sync / exit_fixed / exit_early / exit_repro2）、真实缩略图、无 emoji 占位。
- 代码：`MediaLibraryActivity.kt` 334→248 行（拆出 `MediaLibraryGrid.kt` 127 行），缩略图改 `produceState + IO`。

## P0-3 Emby hero —— ✅ 真机验证通过（03:10，用户指出设备上有真实 Emby 服）
- 代码复核（02:55）：`HeroPage.kt:148` 标题行 `overflow=TextOverflow.Ellipsis`；`:160` `card.rating?.takeIf { it > 0f }?.let { ... }` 无评分不渲染徽章。
- **纠错**：02:33 时判断"K40 无已连接 Emby 服"是错的——`am start EmbyActivity` 跳回 MainActivity 的真实原因是**未带 `kivo.sourceId`**（`EmbyActivity.kt:47-53` 中 `sourceId==null` 就重定向），不是没服。用户指出后，用临时 instrumentation 调 `SourceStore.read()` 读出设备上 4 个真实 Emby 服（emby.bangumi.ca / nyamedia / nl2.lovehd.de / emby.hohai.eu.org），带 sourceId 即可进真实 Emby 页。脚手架已删。
- **真机证据（nyamedia 服，6 张 hero）**：
  - `p0-3-emby-real-hero.png`「谷雨」、`p0-3-emby-hero-next.png`「一斩苍穹」、`p0-3-hero-swipe-2.png`「走马上任」、`p0-3-hero-swipe-3.png`「朱色假面」、`p0-3-hero-swipe-4.png`「萤火虫的出嫁」——中文标题全部完整、评分 `TMDB 9.0/8.0/7.2/7.9/9.1` 正常显示。
  - `p0-3-hero-swipe-1.png`「Running Man」——**长英文标题完整居中、无溢出无截断**（标题修复的直接验证）。
- **无评分隐藏假 0.0（关键验证）**：`p0-3-emby-bangumi.png`（emby.hohai.eu.org 服）+ 放大图 `hero_textband_zoom2.png`——「爱的烙印」剧情简介下方**直接是分页圆点，无任何评分徽章**。旧代码此处会强显 `Emby 0.0`；修复后正确隐藏 ✅。
- TMDB 侧回归：`home-relaunch.png` TMDB 首页「奥德赛」+ `TMDB 8.0` 正常。
- 附带发现：Emby hero 浅底海报（「爱的烙印」浅粉底）上标题/简介为深色字、清晰可读——Emby hero 走 `HomeAtmosphere.lightForeground`（`HeroPage.kt:86`），与 P0-1 详情 hero 的 `HeroForeground` 是两套判定，此图不构成 P0-1 证据。

## P0-1 详情 hero 对比度 —— ✅ 判定逻辑真机 6/6 PASS；深底无回归；UI 浅底截图未取到
- 代码：新建 `HeroForeground.kt`（67 行）采样海报文字带亮度；`DetailPage.kt` 按「海报亮度 × 0.30 + field 亮度 × 0.70」判定前景色，浅底自动切黑字，深底维持白字。原状态图标口径不动，避免回归。
- 权重依据：文字带在 492–560dp，DetailBackdrop 渐变 250→640dp（.55→alpha .45 / .8→alpha .9），文字处 field alpha ≈0.58–0.8，取 0.70。
- 深底验证通过：`p0-1-detail-after.png`（奥德赛）、`detail-probe.png`（力量之戒）——白字压深底清晰，日期/简介/评分全部可读。
- **判定逻辑真机硬证据**（02:52）：`p0-1-herofg-assertions.txt`，K40 上 instrumentation 实跑 `HeroForeground` 三个函数，6/6 PASS：
  - 浅米海报(#EFE3C4)文字带亮度 0.891 → 合成 0.891 → `needsLight=false`（切深色前景）✅
  - 深海报(#1A1A1F)文字带亮度 0.103 → `needsLight=true`（白字，无回归）✅
  - p240 原型「深海报 + 浅米 field」→ 合成 0.655 > 0.40 → `needsLight=false`（切深色前景）✅
- **UI 截图缺口**：尝试用 instrumentation 以 app uid 启动喂入浅米海报的详情页（`EXTRA_HERO`=本地 HTTP 服），海报 HTTP 请求已到达（服务端 200 日志），但 MIUI 拦截 instrumentation 线程启动 Activity，`startActivitySync` 卡死、主线程 `startActivity` 后 Activity 不入栈 ⇒ 浅底切黑字的**视觉效果截图**未取到。判定逻辑已在真机证明，视觉呈现依赖 Compose `fg` 走同一变量（`DetailPage.kt`），无独立失效点。脚手架已删、本地服已停。

## 结论：G1 三项 P0 全部收口
1. P0-1：判定逻辑真机 6/6 PASS + 深底视觉截图；浅底视觉截图受 MIUI 拦 instrumentation 未取到（判定逻辑已证，无独立失效点）。
2. P0-2：真机截图通过（2 列白卡、去扩展名、真缩略图）。
3. P0-3：真机 6 张 hero + 无评分隐藏场景全部通过。
4. 所有脚手架已删、本地服/端口映射已清、生产包 `assembleDebug` = **BUILD SUCCESSFUL**。**未提交、未推送。**

## 验证强度总表
| 项 | 代码 | 逻辑真机 | 视觉真机 |
|---|---|---|---|
| P0-2 本地影片 | ✅ | — | ✅ 截图 |
| P0-3 Emby hero | ✅ | — | ✅ 6 张 hero + 无评分隐藏 |
| P0-1 详情对比度 | ✅ | ✅ 6/6 | ◐ 深底✅/浅底未取 |
