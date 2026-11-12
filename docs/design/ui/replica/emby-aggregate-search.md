# Emby 聚合搜索 + 类型墙（256）

2026-11-12；用户给了另一播放器在**平板**上的两张截图，明确"我也需要这样的效果，**聚合搜索很重要的**"。
此卡复刻两屏：①「类型」渐变类型墙；②跨服务器聚合搜索页（搜索栏 + 六排筛选 + 合并结果网格）。
沿用既有约束：不静默降级（非空可见 reason）、不伪造状态、token 离 URL、凭据/TMDB key 不入库、真机取证、每 `.kt` ≤300 行、先家族树后代码。

## 一、构思与参考边界

两张截图来自**另一个播放器**（非 REX），用户授权按其效果复刻。参考的是**结构与信息分组**，不是像素级照搬其私有素材：

- **类型墙**：24 张渐变卡片，每张左上白色粗体类型名、右侧一叠**扇形旋转的真实海报**；底部"— 已经到底了 —"。
  渐变配色按截图取色；海报必须是**服务器真实条目**（按该 genre 拉取），不放假图。
- **聚合搜索**：顶部搜索栏"输入影片名称搜索"；下面六排筛选 chip；再下面合并多服务器的结果网格。
  结果卡：海报 + **左上类型徽标**（电影/纪录片/电视剧）+ **右下评分**（如 7.6）+ 标题 + 日期或"共N季"。

**核心差异（必须做对）**：现有 `EmbyActivity` 是**单服务器**（`kivo.sourceId` 必填）。聚合搜索是**跨服务器**——
要同时登录所有"允许聚合搜索"的 Emby 源、合并它们的结果。所以聚合页**不寄生在单源 EmbyActivity 里**，
而是一个独立的跨源宿主，从资源库（`SourcesScreen`）进入。单源浏览/详情/播放链路一行不动。

**复用既有事实**：`SourceSummaries` 已是"并行登录多源、Semaphore(2) 限流、内存态、失败带 reason"的先例；
`SourceEligibility.search(records)` 已定义"可聚合搜索 = `!hidden && aggregateSearchEnabled`"；
`SourceAccessSession.unlocked` 管隐藏源解锁。聚合会话直接沿用这套口径，不另造权限模型。

## 二、API 实测事实（bangumi 4.8，2026-11-12 钉死，可复算）

`EmbyClient.items()` 现状只支持 `IncludeItemTypes/Recursive/ParentId/单Genres/SortBy(DateCreated|SortName|ProductionYear)/StartIndex/Limit≤100/Fields=ProductionYear/SearchTerm`。
本卡需要扩的参数，**全部已在真服务器验证可用**：

| 能力 | 参数 | 实测 |
| --- | --- | --- |
| 评分排序 | `SortBy=CommunityRating,Id&SortOrder=Descending` | total 2380，首条 CommunityRating=10 ✅ |
| 上映排序 | `SortBy=PremiereDate,Id` | 按 PremiereDate 倒序 ✅ |
| 年份过滤 | `Years=2023,2024` | total 78 ✅ |
| 更早（<1960） | `MaxPremiereDate=1960-01-01` | total 6（后窗 1954…）✅ |
| 富字段 | `Fields=Genres,ProductionLocations,CommunityRating,PremiereDate,ChildCount` | 全部返回 ✅ |
| 季数 | `ChildCount`（Series） | =1 → "共1季" ✅ |
| 地区 | `ProductionLocations` | **英文国名**：`Japan`/`United States of America`/`Canada`/`China` |

**两条诚实约束**：
1. **地区只能客户端过滤**——Emby `/Items` 没有按制作国家过滤的参数。地区 chip 作用于**已加载结果**的 `ProductionLocations`，
   英文国名→中文地区映射见 §四。UI 不假装"地区是全库精确过滤"。
2. `ProductionYear` 在有 `PremiereDate` 时常为 null——年份徽标/日期统一从 `PremiereDate` 取年，缺则回退 `ProductionYear`。

## 三、流程

**入口**：资源库页新增"聚合搜索"入口卡（与"本地影片/SMB/私密区"同层）。点击 → `AggregateActivity`。
类型墙作为聚合宿主的另一屏（顶部"搜索 / 类型"切换），点类型卡 → 回到搜索屏并预选该 genre。

**聚合会话**（进入时一次）：读 `SourceRepository` → `SourceEligibility.search` 过滤（隐藏源需 `unlocked`）→
Semaphore(2) 并行 `login` → 每源得到 `AggregateSource`（记录元信息 + client + auth）或**非空失败 reason**。
全部失败 → 整页可见错误 + 重试；部分失败 → 结果区顶部列出"某源未接入：reason"，不静默吞。

**搜索**：搜索词 debounce 350ms（沿用 `EmbyLibraryModel` 口径）+ 六排筛选 → 翻译成每源查询 →
并行拉取 → 合并（按 `源id+条目id` 去重）→ 排序 → 客户端地区过滤 → 网格。generation 栅栏迟到回包。
媒体库 chip 选择参与哪些源；其余筛选见 §四翻译表。分页：每源 `StartIndex` 递进，合并后按源轮流补页。

**结果点击**：复用 `MovieDetailActivity.embyIntent(sourceId, item, token, userId, serverName)`——
用**该结果所属源**的 token/userId，不用当前单源会话。详情/播放链路完全不变。

**响应式**：手机紧凑单栏搜索栏 + 可横滑筛选排；平板宽屏筛选排展开、结果网格列数随宽度自适应
（`GridCells.Adaptive`，最小卡宽手机 128dp / 平板 150dp）。类型墙手机 2 列 / 平板 3–4 列。

## 四、六排筛选 → 查询翻译（数据驱动，一排一组件六份数据）

每排**单选**（默认"全部/不限"），翻译为 `EmbyClient.items` 参数 + 客户端过滤：

| 排 | chip | 翻译 |
| --- | --- | --- |
| **媒体库** | 各源名（动态） | 选择参与查询的 `AggregateSource` 子集；默认全部已接入源 |
| **全部**（内容类别） | 全部 | `IncludeItemTypes=Movie,Series` |
| | 电视剧 | `Series` |
| | 电影 | `Movie` |
| | 动漫 | `Movie,Series` + `Genres=动画` |
| | 综艺 | `Series` + `Genres=真人秀` |
| | 演唱会 | `Movie` + `Genres=音乐` |
| | 纪录片 | `Movie,Series` + `Genres=纪录` |
| **最新更新**（排序） | 最新更新 | `SortBy=DateCreated` desc |
| | 最新上映 | `SortBy=PremiereDate` desc |
| | 影片评分 | `SortBy=CommunityRating` desc |
| **类型**（genre） | 24 类型（动态自服务器 Genres） | `Genres=<名>`（与类别排叠加时取交集语义：类别已带 genre 则合并） |
| **地区** | 16 地区 | **客户端**过滤 `ProductionLocations`，映射见下 |
| **年份** | 2020年代/2010年代/2000年代/90…60年代 | `Years=< decade 内逐年 >` |
| | 2026…2020（单年） | `Years=<年>` |
| | 更早 | `MaxPremiereDate=1960-01-01` |

**地区映射**（英文国名 → 中文 chip；`欧美` 是集合）：
`China→中国大陆`、`Hong Kong→中国香港`、`Taiwan→中国台湾`、`Singapore→新加坡`、
`United States of America→美国`、`South Korea/Korea→韩国`、`Japan→日本`、`Thailand→泰国`、
`United Kingdom→英国`、`France→法国`、`Germany→德国`、`Italy→意大利`、`Spain→西班牙`、
`India→印度`、`Russia→俄罗斯`；`欧美 = {美国,英国,法国,德国,意大利,西班牙,加拿大,澳大利亚…}`（命中任一即属欧美）。
未识别国名不硬塞，地区筛选时按"无匹配"处理。

**类型墙 24 类型**（与截图一致，服务器 Genres 实测含这些）：
剧情 喜剧 动作 动画 犯罪 惊悚 悬疑 冒险 科幻 爱情 动作冒险 奇幻 家庭 恐怖 历史 战争 纪录 音乐 西部 儿童 真人秀 电视电影 肥皂剧 脱口秀。
渐变配色按截图取色，**一个 `GenreCard` 组件 + 24 份数据**（不写 24 个文件）。

## 五、自然家族树

```text
emby/
  EmbyClient.kt                  扩 items()：Years/Min-MaxPremiereDate、CommunityRating/PremiereDate 排序、
                                 多 Genres、Fields+=Genres,ProductionLocations,CommunityRating,PremiereDate,ChildCount
  EmbyItem.kt                    增可选聚合字段（rating/genres/countries/premiereDate/childCount/dateCreated），默认值不破坏旧调用
  EmbyErrors.kt                  HTTP/网络→中文 reason 的共享翻译（单源会话与聚合共用，不各写一份）
  aggregate/
    AggregateSource.kt           一个已连接源句柄：记录元信息(id/name)+client+auth；建详情 intent
    AggregateSession.kt          跨源连接：读源→eligibility.search+解锁过滤→Semaphore(2)并行登录→句柄+非空reason
    AggregateFilter.kt           六排筛选的数据定义 + chip→查询翻译 + 地区映射（纯逻辑，无 UI）
    AggregateSearchModel.kt      跨源跑查询、合并/去重/排序/分页、客户端地区过滤、主线程态、generation 栅栏
    AggregateViewModel.kt        持有 session+search 两个态机，onCleared 统一关闭（作用域归一）
    GenreWallData.kt             24 类型 + 渐变调色板 + 按 genre 取真实海报
  aggregate/ui/
    AggregateActivity.kt         跨源宿主：主题/解锁门/搜索⇄类型墙切换/结果→详情导航
    AggregateSearchScreen.kt     搜索栏 + 即时状态行（N 个结果/搜索中/空/错误）+ 六排筛选 + 合并结果网格 + 分页脚注
    AggregateFilterRows.kt       一个 chip-row 组件，渲染六排筛选数据（含动态媒体库/类型排）
    AggregateResultCard.kt       海报 + 左上类型徽标 + 右下评分 + 标题 + 日期/共N季
    AggregatePoster.kt           跨源海报加载（LruCache 24MB + decodeImageCapped），搜索卡与类型墙共用
    GenreWallScreen.kt           渐变类型卡网格 + "— 已经到底了 —" 脚注
    GenreCard.kt                 一个数据驱动渐变卡：左上白色粗体名 + 右侧扇形旋转海报堆（窄卡 2 张/宽卡 3 张）
sources/ui/
  SourcesScreen.kt               增"聚合搜索"入口卡（同层于本地影片/SMB/私密区）
docs/evidence/p256/aggregate-search/   真机逐项对照两截图，脱敏结论（凭据/key 不入库）
```

**边界**：单源 `EmbyActivity/EmbySessionModel/EmbyBrowseScreen/EmbyLibraryScreen` 不改；
`backend/`、JNI、Surface、解码循环范围外；不新造协议/解码器；地区客户端过滤的诚实性在 UI 与证据中写明。

## 六、闸门

- 构建固定签名覆盖安装（K40 `b6e950ee`）；平板 `dd137cf7` 若可行补响应式证据（不当 K40 性能证据）。
- 聚合搜索：多源并行登录、合并去重、六排筛选各自生效、搜索 debounce、分页、结果→正确源详情；
  部分源失败带可见 reason、全失败整页错误可重试、取消不被迟到回包带走。
- 类型墙：24 卡渐变 + 扇形**真实**海报 + 到底脚注；点卡预选 genre 回搜索。
- 逐项对照两张截图，达不到就继续做直到持平/超过；动效有过渡（不瞬现瞬消）。
- 证据可复算、脱敏；收口提交 + 推送（0 unpushed）。

## 七、第二轮：入口归位 + 手机形态（2026-11-12 用户反馈）

用户三条反馈：① 聚合搜索只放在资源库不合逻辑，全局搜索入口是底栏右下搜索钮；
② 手机上六排筛选全部竖展开占满首屏，"你需要设计，不是随便装上就行了"；
③ 展开方式拍板：**直接向左滑动**（每排单行横滑），不做展开按钮。

### 7.1 入口归位

- 底栏右下搜索钮（`CapsuleNavBar.onSearch`）原接 `MediaLibraryActivity`（本地库，**无搜索框**）——
  名为搜索实为浏览，正是用户指出的不合逻辑。改接 `AggregateActivity`（搜索模式）。
- 资源库「聚合搜索」绿卡保留为次入口（管理语境：旁边就是服务器开关）。
- 全局搜索必须覆盖"用户拥有的全部片源"，否则接走搜索钮就丢了本地：
  **搜索词非空时**并入本地标题匹配，独立「本地」分区置于服务器结果之前；
  搜索词为空 = 浏览态，只展示服务器最新合并（本地全量在资源库→本地影片）。
- 本地匹配诚实性：私密区标记的条目**必须排除**（231 红线，搜索不能漏私密）；
  无 READ_MEDIA_VIDEO 授权时分区显示非空 reason（"本地未授权…"），不静默消失。
- 本地结果点击复用 `DetailActivity` 既有契约（`kivo.media`/`kivo.durationMs`，与本地库同路）。
- 范围明示：搜索覆盖 本地 + Emby；SMB 不在内（远程逐目录搜索成本另卡），分区标题不夸大。

### 7.2 手机形态：固定搜索框 + 折叠摘要 + 单行左滑（用户三轮参考帧拍板）

参考帧（另一播放器手机搜索页，三张）：① 折叠态——灰底搜索栏**固定顶部不随结果滚动**，
栏下一条居中品牌色摘要「媒体库 · 最新更新 · 全部 ▼」，结果网格紧随其下；
② 展开态——六排单行横滑，媒体库排尾部带「媒体库」label pill，全部/最新更新选中浅底 pill；
③ 左滑态——类型/地区排 label pill 滑出屏外，证明每排独立横滑。
用户原话：「搜索框搞好看点，跟我们播放器风格要统一，最好能像那个播放器那样固定搜索框」。

**固定区（LazyVerticalGrid 之外）**：搜索栏 + 状态行 + 摘要行。结果滚动时三者不动。

- 搜索栏风格与播放器同源：复用 `GlassSurface`（毛玻璃圆钮/胶囊导航同一组件），
  白玻璃 + `hairline` 描边 + `radiusCard` 13dp 圆角；放大镜/占位符 `textSub`，
  输入文字 `textMain`，光标 primary。不再用孤立灰底，视觉语言归队。
- **折叠（手机默认）**：一条居中品牌色摘要，动态拼当前选择：
  `媒体库(全选时)/N 个媒体库 · 排序label · 类别label [· 类型 · 地区 · 年份]` + ▼；
  点摘要展开六排，展开态摘要变 ▲ 可收回。摘要常驻固定区——浏览结果时随时可见当前筛选、随时可折叠。
- **展开的六排是网格首项**：随结果向上滚走（参考帧行为），摘要行不滚走。
- 宽屏（≥700dp）：默认展开、无摘要行（平板参考图形态），左 label 列 + FlowRow 换行。
- 展开态 `rememberSaveable` 保存，旋转不丢。

**chip 形态（窄屏，参考帧②③）**：

- 六排各为**单行水平滚动**（`horizontalScroll`），左滑看更多，行高恒定。
  **选中 = 浅底圆角 pill + 品牌色字，未选中 = 纯文本**。
  媒体库排尾部浅色「媒体库」label pill；类别/排序排无 label（首 chip 自解释）；
  类型/地区/年份三排起头浅色 label pill（不可点）；窄屏不放「不限」chip——
  再点已选中项即清除（模型已支持 toggle-off）。
- 宽屏保持左 label 列 + `FlowRow` + 白 pill/实心选中，与平板截图一致。
- 同一 `ChipRow` 组件按 `wide` 分支布局，chip 点击回写不变（不写两套控件）。
- 状态行（N 个结果/搜索中/空/错误）在搜索栏下；计数 = 服务器 + 本地匹配之和。

### 7.3 家族树增量

```text
emby/aggregate/
  LocalSearch.kt               本地标题匹配（IO）：MediaStore 投影最小列 + 私密区排除 + 授权态 reason；纯数据不持 UI
emby/aggregate/ui/
  LocalResultCard.kt           本地卡：系统缩略图 + 本地徽标 + 时长角标 + 标题；点击→DetailActivity 既有契约
  AggregateFilterRows.kt       增 wide 分支：窄屏单行横滑（媒体库排尾 label pill）/ 宽屏 FlowRow（同一 chip 组件）
  AggregateSearchScreen.kt     固定区（GlassSurface 搜索栏+状态行+折叠摘要）+ 六排网格首项 + 本地分区 + wide 透传
MainActivity.kt                onSearch 改接 AggregateActivity（openLibrary 仍服务资源库本地卡）
```
```
## 八、证据

`docs/evidence/p256/aggregate-search/`（README 闸门表 R1-1…R2-12 + 23 帧 + 复算命令 + 披露）。
