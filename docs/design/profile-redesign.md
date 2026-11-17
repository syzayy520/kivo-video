# 「我的」页原创设计规范（kivo · 非复刻）

> 2026-11-17 ｜ 范围变更：**「我的」页不再 1:1 复刻 rex**，改为 kivo 自有设计；其余页面仍以 rex 为齐平基线。
> 目标：**比 rex 更好**，且在信息架构、视觉语言、交互上可举证为原创，避免抄袭指控。

## 0. 为什么可以做得更好

rex 的「我的」是**数据仪表盘**：日历 + 今日柱图 + 20 周热力 + 待看/收藏/在看 + 月度折线 + TOP3 条形 + 类型环图 + 两张趋势卡 + 模块管理。
问题：**10 个统计模块平权堆叠**，首屏看到的是"0.0 小时 / 0 部电影"，而不是"我接下来想看什么"；对大多数用户，数据是副产品，片单才是主资产。

kivo 的差异点（也是产品优势）：播放内核强（4K/HDR/10bit、多实例、SMB/Emby/网盘）、本地片源与远程源并存。
所以「我的」应当回答三个问题，而不是展示十张图：
1. **接着看什么**（继续观看 / 片单）
2. **我收藏了什么**（片单 / 收藏 / 下载）
3. **我的影院怎么样**（一条有洞察的周报，而不是十张图）

## 1. 信息架构（三层，单列滚动）

```
我的
├─ 1. 影院头图（Identity）
│   ├─ 渐变头图：昵称 / 头像 / 设备与源状态（“2 个源 · 在线”）
│   └─ 一句动态：本周看了 3.2 小时 · 连续 5 天
├─ 2. 继续观看（Hero）
│   └─ 一张 16:9 大卡：封面 + 进度环 + 集数 + 「继续」按钮 + 右侧「全部 ›」
├─ 3. 我的片单（Collections）
│   └─ 横向卡：片单封面叠层 + 名称 + 数量；末尾「新建片单 +」
├─ 4. 本周影院报告（Insight，可折叠）
│   ├─ 一行结论： “你本周偏爱情景喜剧”
│   ├─ 三枚指标：时长 / 部数 / 连续天数
│   └─ 展开后：类型偏好环 + 7 日条形（仅 2 个图，不是 10 个）
└─ 5. 管理（Settings entry）
    └─ 「管理我的影院」一行，进入模块与隐私设置
```

明确**不做**：日历周历、20 周热力图、月度折线、TOP3 条形、两张趋势卡、模块开关弹层。
（这些是 rex 的"统计工具"语言；kivo 用「一条周报」替代，信息密度更低、洞察更强。）

## 2. 视觉语言（全部走现有令牌）

| 元素 | 规格 | 令牌 |
| --- | --- | --- |
| 页面底 | 顶部 `bgLavender` → 40% 处 `pageBackground` 垂直渐变 | `bgLavender` / `pageBackground` |
| 头图 | 高 180dp，底部圆角 28dp，叠 1px `hairline` | — |
| 主标题 | 28sp / Bold / `textMain` | `textMain` |
| 副标题 | 13sp / Regular / `textSub` | `textSub` |
| 卡片 | 白底、圆角 16、`hairline` 描边（**不用 groupedFill 填色**，见令牌撞色事实） | `canvasFill` / `hairline` |
| 强调 | 进度环、继续按钮 = `accentGreen`；链接 = `navActive` | `accentGreen` / `navActive` |
| 数据可视化 | 环图 4 档（accentGreen 透明度派生）、条形 6dp 圆角 | — |
| 深色 | 头图 `washFill` 叠深、卡片 `groupedFill`、文字 `White` | `DesignPalette.Dark` |

原创可举证点：
1. **Hero 继续观看**（16:9 + 进度环 + 集数胶囊），rex 用横向小卡列表。
2. **片单（Collections）**作为一级资产，rex 只有"待看/收藏/在看"三个计数。
3. **一条周报**（结论句 + 3 指标 + 2 图），rex 是 10 个平权图表。
4. **影院身份头图**（源状态 + 连续天数），rex 是"默认环境 + 编辑"。
5. **可折叠**：周报默认收起，把首屏留给片单。

## 3. 组件与文件家族树（沿用既有包，不新建工程）

```
ui/profile/
├─ ProfilePage.kt                # 只做编排（≤180 行）
├─ ProfileHeader.kt              # 影院头图（昵称/头像/源状态/一句动态）
├─ ContinueHero.kt               # 16:9 续播大卡 + 进度环
├─ CollectionsRow.kt             # 片单横滑 + 新建入口
├─ WeeklyReport.kt               # 周报卡（结论句 + 3 指标 + 折叠）
├─ WeeklyReportCharts.kt         # 类型环 + 7 日条形（两个图，单一职责）
├─ ManageEntry.kt                # 「管理我的影院」行
├─ ProfileModels.kt              # 保留现有 Dashboard 模型，新增 Collections/WeeklyInsight
└─ ProfileStats.kt               # 保留聚合；新增 insight 结论句生成（纯函数，可单测）
```

**复用**：`ContinueSource` / `VideoThumbnail`（真实首帧）、`ProfileStats`（观看数据）、`ShellInsets`。
**删除**：`WeekStrip`、`HistoryHeatmap`、`MonthlyChart`、`TopShowsCard`、`GenreDonut`（改为 WeeklyReportCharts）、`TrendCard`、`ModuleManagerSheet`、`ModuleRow`、`ModuleGlyph`、`watchlist/collection/watching` 三个旧 section。
> 先新建、接线完成后再删旧文件，避免一次性大爆炸；删除动作单独一张卡。

## 4. 交互与动效

- 下拉刷新：重新生成周报（`HomeMotion.sectionRevealMs`，减弱动效时 0）。
- 片单长按：快速菜单（重命名 / 隐藏 / 删除），44dp 命中盒。
- 继续观看点击：直接续播；右侧「全部」进「在看」覆盖层（现有入口）。
- 周报折叠：`AnimatedVisibility`，`reduceMotion` 时直接切换无动画。
- 空态：头图下直接显示「影院还是空的 · 去发现页挑一部」+ 主按钮，**不渲染 0 值图表**。

## 5. 验收（不依赖 rex 帧）

| 项 | 判据 |
| --- | --- |
| 首屏 | 头图 + 继续观看 Hero + 片单首行 全部可见，无需滚动 |
| 空影院 | 不出现任何 0 值图表；只有一句引导 + 主按钮 |
| 有数据 | 周报结论句与数据一致（纯函数单测 3 例：无数据/单类型/多类型） |
| 深色 | 头图与卡片层级可辨，无白块 |
| 抄袭风险 | 与 rex 的「我的」同屏并排，模块名称/结构/图表组合均不同（本规范 §1 可举证） |

## 6. 实施顺序

1. P1：`ProfileHeader` + `ContinueHero` + `CollectionsRow`（首屏三件套）
2. P2：`WeeklyReport` + `WeeklyReportCharts`（折叠 + 结论句纯函数）
3. P3：`ManageEntry` + 删除旧图表组件 + 门禁/深色回归

> 本轮先确认本规范；确认后从 P1 开工，每步都补 `@Preview` 与真机截图，不引用 rex 帧作为验收标准。