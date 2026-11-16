# 336 · 全局页面加载流畅度验证（2026-11-16 傍晚，K40 b6e950ee）

## 用户诉求
「要全量优化，不止 emby 页面」「还是好几秒才真正显示」「每个影片详情页都要 3 秒」
「把 rex 摸清楚再实现」「不准应付了事」。

## 根因（三层，逐层取证）
1. **TMDB JSON 零缓存**：设备无代理，`api.tmdb.org` 冷连 2.609s / 热 0.7–0.9s（百度 0.15s）。
   详情页 `tmdb.details()` 每次发 5-append 大请求；首页 5 个列表同理。
2. **Emby hero 被评分卡死**：`heroItems()` 里 16 个 TMDB 评分请求 `get()` 完才返回，
   Emby 数据早到（日志时间戳）却因评分阻塞显示空位。
3. **详情页加载期浅色白屏**（录屏才发现的第二层）：`DetailBackdrop` 在 `details==null`
   时铺 `pageBackground #F2F2F7` + 吸色渐变初始浅色 → 白屏转圈，采样像素 `RGB(243,242,247)`。

## Rex 怎么快（吃透到实现层，docs/reference/rex-teardown 静态实证）
- 持久化缓存：`settings.cache.item.general`(Data Request Cache) + `appSettings.copy.detail.cache.duration`
  (详情缓存时长，可配) + 图片落本机（`posterLanguage.cost.message`）。
- 首屏非阻塞：`LazyVStack` 模块流，每 Section 独立 Loading/错误态，互不等候；
  `RexHome.bundle` 内置 20+ mock 占位卡先出骨架。
- 数据源：`RexScript.bundle` 的 `rex-tmdb.js/bangumi.js` JS 脚本 + 多源优先级 + 反向代理，不吊在单条慢链路。
- 结论：Rex 快 = 全链路缓存 + 首屏占位/模块级异步 + 多源/反代。我们落地了①，②部分（不写 UI 未加骨架屏），③属新架构不在范围。

## 修复
- 新建 `TmdbResponseCache.kt`（JSON 磁盘缓存，SHA256 key，TTL 详情/搜索 7d、列表 12h，原子写，失败不落盘）。
- 接入 `TmdbClient.searchFirst` / `TmdbDetail.get` / `TmdbHome.get` 三处；构造时注入 cacheDir。
- `EmbySessionModel` 拆 `heroRaw`(Emby 直连先出) + `enrichHeroRatings`(后台补评分)；
  `EmbyBrowseScreen` 两阶段显示：数据先出、评分后补。
- `DetailBackdrop` 加载期底色改黑 + 抑制吸色渐变；`DetailPage` 加载转圈改白。最终视觉零变化。

## 证据（真机，时间戳/像素/抽帧，非目测）
- **首页缓存命中铁证**（net-diag `[tmdb-cache]`）：
  - 首次（网络+STORE）：`1794827386592 → 1794827389778`，7 请求跨度 **3186 ms**；
  - 二次（全 HIT）：`1794827428970 → 1794827429336`，7 请求批量命中 **366 ms**。
  - ⇒ 首屏数据等待 ~3.2s → ~0.37s。
- **详情页缓存命中铁证**：`movie/1263337?append_to_response=...`
  - STORE `1794827542446`（首次网络）→ HIT `1794827557480`（二次），二次零网络。
- **白屏修复像素铁证**：同一时刻(1.2–1.6s)详情页中心像素：
  - 修复前 `RGB(243,242,247)`（#F2F2F7 白屏）→ 修复后 `RGB(1,0,2)/(0,0,0)`（纯黑）。
  - 抽帧宫格：`detail_in_grid.png`(修复前白屏) vs `detail_final_grid.png`(修复后黑底白圈+正常详情)。
- 编译 `BUILD SUCCESSFUL`；改动 8 文件均 <300 行；无死代码（旧 heroItems 已移除）。

## 诚实边界
- **首次**进详情页仍走真实网络（2.6s 跨境），缓存只解决「返回再进/同片重进」；未做列表页预热详情。
- 加载期仍有 <1 帧重组开销，无法归零；改深色后与最终态一致，不再"白闪"。
- 首页/详情骨架占位按铁律「不写 UI」未加（与 Rex 的客观差距，登记不隐瞒）。
- 多源聚合/反代属新架构，不在本次范围；设备当前 TMDB 直连无代理。

## R2 返工（用户裁决「把白变成黑不是好做法」，2026-11-16 19:25）

### 承认错误
上面第 3 层修复（白改黑）只是换占位底色，**等待期屏幕仍无任何内容**——治标不治本。
应用户要求撤销：`DetailBackdrop` 恢复 `pageBackground`、`DetailPage` 转圈恢复 `fg`，黑色占位改动全部移除。

### 真正根因
`MovieDetailActivity.intent(context, item)` 只传 `id/isTv`，把 `TmdbHome.Item` 已有的
**title/overview/rating/posterUrl/posterHeroUrl/backdropUrl/genres/date** 全丢了。
列表卡片就是用这些渲染的（图已在磁盘缓存）——点击时数据早在本机，却被丢弃再从零发网络。

### 正解：preview-then-replace（与 Rex 先出内容再填同源）
- 新建 `DetailPreview.kt`（70 行，单职责）：intent 字段 → 预览 `TmdbDetail.Details`；
  TMDB 首页/搜索、Emby 入口、相似作品三处 `intent()` 都填全字段。
- `DetailContent` 加 `preview` 标记；`DetailPage` 初值 = preview content ⇒ **首帧即海报+标题+评分+简介**；
  吸色 effect 跟随幕布 URL（preview 上屏当帧就采，避免浅灰块）；`load()` 成功整体替换，失败保留 preview。
  preview 阶段跳过未解析的片源/季集/演员/相似区块（不留"没有匹配本地视频"误导）。
- Emby 图需 token：`EmbyResource.previewLoader()` 透传带鉴权加载器给预览体。

### 真机铁证（K40，清 JSON 缓存的最坏情况，TMDB 详情仍走网络 STORE）
- 日志：`1794828898130 [tmdb-cache] STORE movie/1263337?append_to_response=...` —— 详情主请求确实走网络。
- 抽帧宫格 `preview_final_grid.png`（6×5，每格 0.1s，每格 0.1s）：
  - 第 1–16 格：首页；
  - **第 17 格（≈1.6s）：详情页首次出现，第一帧就是完整海报大图 + "野兽之心"标题**，
    此前此后无任何纯白/纯黑/转圈等待格。
  - 对比最原始版 `detail_in_grid.png`（无缓存无 preview，白屏那版）：详情页出现后有 7–9 格白屏 + 转圈。
  - 目录内 `detail_after_fix_grid.png`/`detail_final_grid.png` 是中间"白改黑"返工版证据，已撤销；
    最终版以 `preview_final_grid.png` 为准（首帧即内容，不再靠换底色掩盖）。
- 标题一致性核对：`api.tmdb.org/3/movie/1263337?language=zh-CN` → `title=野兽之心 / original=Heart of the Beast`，
  与 preview 标题一致，无替换跳变。（海报上烧的"狂野雄心"是海报自带译名，非标题字段，同一部片。）
- 编译 `BUILD SUCCESSFUL`；7 文件均 <300 行（DetailPage 266）；白改黑补丁零残留。

### R2 诚实边界
- preview 的简介/评分是列表快照，完整详情到位后会被更精确值替换（同部片，视觉基本无感）。
- 点击→详情出现约 1.6s 中包含 Activity 切换动画 + tap 响应，非详情页加载等待；详情页本身首帧完整。
- 骨架占位仍未加（铁律不写 UI），但 preview 已用真实列表数据替代了空白，观感上不再是"白屏等待"。
