# Kivo 搜索页 · 落地设计与文件家族树

> 定稿来源：`search-final.html` / `search-final.png`（四态：① 搜索首页 ② 输入中 ③ 分类筛选 ④ 滚动吸顶摘要）。
> 本文是落地闸门：评审判定 → 落地口径 → 家族树 → 验收。未过家族树不写实现。

## 一、设计评审判定

结论：**可以落地**。视觉与首页 v6 / 观看记录同族（白 66% 玻璃、取色渐变、黄铜星标、克制蓝色），
「三行筛选 → 滚动吸顶摘要」的收缩策略保留。以下 4 处按铁律做偏差修正：

| # | 原稿 | 落地口径 | 理由 |
| --- | --- | --- | --- |
| D1 | 底栏玻璃化（白 66% + 亮边） | **不改全局 `CapsuleNavBar`**，搜索页沿用现有实心胶囊 | 原稿「落地注意」自陈会影响所有页；首页导航另有在途评审稿，不抢跑 |
| D2 | 热搜第 4 行 / 网格末行被底栏遮挡 | 滚动容器 `contentPadding.bottom = 50 + 22 + 16 = 88dp` | 原稿四态均有裁切，属缺陷不是设计 |
| D3 | 热搜「↑ 3」名次升降 | **不实现**。热搜 = TMDB `trending/all/day`；NEW = 上映/首播 ≤30 天（真实可推算） | 无历史名次数据源，伪造状态违反铁律 |
| D4 | 吸顶摘要只显示 类型·年份·排序 | 地区非「全部」、只看可播开启时也进摘要；整条可点展开，展开/收起有过渡 | 摘要必须能表达全部生效条件 |

其余实现口径：

- 分类卡剧照走 TMDB `/discover` 派生端点 + `include_image_language=null`（无文字剧照），复用 12h 缓存口径。
- 分类卡 veil 取色 = `PosterPalette.ambientOf` 封面下部主色**压暗到亮度 ≤20%**（现有 `PosterPalette` 只返平均色，压暗在搜索侧令牌层补，不改详情页在用的原函数）。
- 语音输入 = 系统 `RecognizerIntent`；无可用识别服务时 Toast 明示原因（不静默降级）。
- 海报下信息统一「★ 评分 · 年份」，角标只留 4K / HDR（HDR 仅当本地版本目录有真实 HDR 版本时显示，读不到就不显示，不编造）。
- 键盘、联想词为系统能力，不自绘键盘。

## 二、数据真实口径（不伪造状态）

| 模块 | 数据源 | 空/失败态 |
| --- | --- | --- |
| 最近搜索 | 本机 SharedPreferences，上限 10 条，可单删/清空 | 无记录时整个模块不渲染（真实空态，不写开发备注） |
| 热搜榜 | TMDB `trending/all/day`，取有海报且评分 >0 的前 10 | 加载失败显示错误行 + 重试；不填假片名 |
| 输入中结果 | 本地（NAS 索引 + MediaStore + 已授权 Emby）∪ TMDB `search/multi`，350ms 防抖，取消迟到结果 | 「NAS 可播」绿点 /「TMDB · 未入库」灰点按真实匹配；无结果显示空态文案 |
| 最佳匹配 | 结果集第一条（本地可播优先，其次 TMDB 评分最高） | 无结果不出现 |
| 分类筛选 ③④ | TMDB `discover/movie|tv`（类型/地区/年份/排序）+ 本地可播匹配；动漫/综艺/演唱会/纪录片映射到 discover 的 genre/type 参数，映射不了的分类给可见说明 | 「只看可播」打开时只列本地命中；0 结果给空态 |
| 可播判定 | 标题归一化全等 + 年份相容（复用 `TitleMatch` / `MediaMatch` 口径），NAS 索引 + MediaStore + Emby 已授权源 | 判不了的一律「未入库」，不猜 |

隐私硬门（沿用 L3）：隐藏/禁用聚合搜索的源在**请求前**排除，历史、建议、缓存、迟到结果不得泄露其内容。

## 三、宿主与导航

- 底栏放大镜：由「启动旧 `AggregateActivity`」改为 **Shell 内搜索覆盖层**（与观看记录同机制），
  ①③④ 注入同一个 `CapsuleNavBar`（搜索钮高亮），② 输入中不显示底栏。
- 旧 `emby/aggregate/` 文件**全部保留不动**（资源库聚合搜索设置项与既有回归口径不受影响）。
- 返回键：② → ①；③④ → ①；① → 关闭覆盖层。点结果进详情复用 `MovieDetailActivity` 既有入口，播放只读调用。

## 四、文件家族树

新增两个包：UI 族 `ui/search/`，数据族 `search/`（与 `ui/history/` + `library/history/` 的同构划分一致）。
每个文件 ≤300 行，一个文件一个职责。

```
android/app/src/main/java/com/kivo/cinema/
├── ui/search/                              # 搜索页 UI 族
│   ├── SearchOverlay.kt                    # 覆盖层宿主：三态切换、返回键、过渡动画、底栏注入
│   ├── SearchUiState.kt                    # UI 状态模型与纯函数（摘要文案、筛选是否非默认）
│   ├── SearchTokens.kt                     # 本页尺寸/色令牌（引用 DesignTokens、V6Metrics.brass）
│   ├── SearchGlyphs.kt                     # 本页线框字形（返回/清除/语音/下拉/筛选/播放/放大镜）
│   ├── home/
│   │   ├── SearchHomePage.kt               # ① 编排：大标题/搜索框/最近搜索/分类/热搜
│   │   ├── SearchField.kt                  # 玻璃搜索框（①入口态与②编辑态共用一组件）
│   │   ├── RecentSearchChips.kt            # 最近搜索 chips + 清除
│   │   ├── CategoryCardGrid.kt             # 3 列 × 2 行六分类剧照卡（六张卡 = 一份数据）
│   │   └── HotSearchBoard.kt               # 热搜榜白卡：排名/缩略图/元信息/NEW
│   ├── query/
│   │   ├── SearchQueryPage.kt              # ② 编排：搜索框+取消、最佳匹配、结果列表、空/错/加载
│   │   ├── BestMatchCard.kt                # 最佳匹配取色大卡 + 播放钮
│   │   └── SearchResultList.kt             # 结果行：关键词黄铜高亮、来源签条
│   └── browse/
│       ├── SearchBrowsePage.kt             # ③④ 编排：bar3/tabs/筛选/工具行/网格/吸顶摘要
│       ├── BrowseFilterRows.kt             # 类型/地区/年份三行横滑胶囊（三行 = 一份数据）
│       ├── BrowseSummaryBar.kt             # 共 N 部 + 排序下拉 + 只看可播 + 吸顶摘要 pill
│       └── BrowsePosterGrid.kt             # 3 列 2:3 海报网格（评分·年份、4K/HDR 角标）
└── search/                                 # 搜索数据族（无 Compose 依赖）
    ├── SearchModels.kt                     # SearchEntry / 来源 / 可播 / 分类 / 筛选 / 排序模型
    ├── RecentSearchStore.kt                # 最近搜索持久化（SharedPreferences，≤10 条）
    ├── SearchRepository.kt                 # 查询编排：防抖、取消、本地 ∪ TMDB 合并、最佳匹配选择
    ├── LocalPlayableIndex.kt               # NAS 索引 + MediaStore + Emby 的标题匹配可播判定
    ├── EmbyPlayableIndex.kt                # Emby 渠道：已授权源连接 + 全库标题池（10min 缓存）+ 失败可见原因
    ├── HotSearchRepository.kt              # trending/day → 热搜榜（含 NEW 推算）
    ├── CategoryArtworkRepository.kt        # 六分类无文字剧照（discover + include_image_language=null）
    └── BrowseRepository.kt                 # discover 筛选/排序/分页 + 只看可播过滤

改动的既有文件（最小接线，不重构他人代码）：
├── MainActivityShell.kt                    # 放大镜 → 搜索覆盖层；注入底栏
├── MainActivitySearch.kt                   # 新增：覆盖层宿主接线（仓储装配 + 详情入口 + 底栏 slot）
├── tmdb/TmdbHome.kt                        # 纯增量：pagedItems()（分页元信息；解析/缓存/错误分类复用）
└── tmdb/TmdbSearch.kt                      # 新增：search/multi 与 trending/day 路径拼装
```

## 四·补、实现期登记的偏差（收口时回写）

### 语音卡重设计（2026-11-24 晚，用户「语音识别的卡片有点丑」）

**误改记录**：先把「语音卡」误解为最佳匹配大卡，做了 v2，用户指出后已回滚（大卡维持原稿不变）。

用户指的是**系统语音识别弹窗**（`com.xiaomi.mibrain.speech` 的 AsrActivity，旧式白框对话框），
样式不受 App 控制。改法：**不再调起系统弹窗**，改用 `SpeechRecognizer` 在 App 进程内识别，
自绘 Kivo 风语音卡：

- 模态玻璃卡（居中、圆角 28、白 66% 玻璃 + hairline），背景 scrim 暗化；
- 标题「正在倾听」+ 实时声波（`onRmsChanged` 驱动 7 条竖条，音量映射高度）；
- 主按钮「说完了」（结束并取结果）+ 次按钮「取消」（可逆退出，系统弹窗没有）；
- 状态反馈：监听中 / 识别中 / 无声音 / 无权限 / 不可用，全部非空文案；
- 权限：`RECORD_AUDIO` 运行时申请，拒绝给可见原因；`isRecognitionAvailable=false` 时回退系统 RecognizerIntent。

**v2 改形（用户「看着别扭」复审）**：居中巨卡改为**底部轻面板**（左右 12dp、圆角 28、白玻璃 blur 24、
遮罩 .32），不遮分类/热搜；按钮改「次按钮文字在左 + 主胶囊 weight 在右」；
失败态新增「键盘输入」退路（关卡进②自动聚焦），不再只有取消/重试。

文件家族树新增：
```
├── search/VoiceSearchController.kt          # SpeechRecognizer 封装：权限/监听/结果/错误/释放
└── ui/search/voice/VoiceSearchDialog.kt     # Kivo 风语音卡：玻璃卡 + 声波 + 取消/说完了
```

- **Emby 已集成（2026-11-24 第二批）**：`EmbyPlayableIndex` 对已授权源（非隐藏、开聚合搜索、非 SMB）
  复用 `EmbyAuthCache` 登录，后台拉全库标题池（Movie+Series，100/页、上限 5000 条/源、10 分钟内存缓存），
  ② 搜索、③④ 可播标注与「只看可播」共用同一池；池就绪后自动重搜/重筛。
  部分源失败在②状态区与③「只看可播」页脚显示非空原因；无 Emby 源属正常，不提示。
- **排序没有「最近更新」**：TMDB discover 无此口径，给真实存在的 人气最高 / 最新上映 / 评分最高。
- **② 的「全部 12 ›」改为「共 N 条」纯文本**：结果页即全部结果，不放假跳转入口。
- **最佳匹配「播放」钮打开详情页**（与全 App 点海报口径一致，由详情页完成起播），未入库不显示该钮。
- **分类卡/最佳匹配的取色渐变复用 `ui/history/TintedArtwork`**（模糊副本 + 渐暗），
  未新增 PosterPalette 压暗分支——原稿的 --tint 取色由 TintedArtwork 的模糊取色等效承担。
- **关键词高亮**用黄铜浅底 + 加粗近似稿面的下半部下划线（Compose SpanStyle 无局部渐变下划线）。

复用不重写：`TmdbImage`、`PosterPalette`、`ui/history/GlassSurface.kt` 的 `Modifier.glassSurface`、
`ui/history/TintedArtwork.kt` 的取色渐层、`TitleMatch`、`MotionGate`、`AppearancePrefs`。
若 `history` 的玻璃/取色修饰符参数不足，做**参数扩展**而不是复制第三套实现。

## 五、验收

1. `assembleDebug` 编译通过；`python tools/ui_gate.py` 无新增超限/裸色回归。
2. 真机 `b6e950ee` 四态截图：① 最近搜索/六分类/热搜真实数据；② 输入出结果、关键词高亮、来源签条；③ 筛选生效且结果与条件一致；④ 滚动吸顶摘要含全部非默认条件、点击展开。
3. 底栏无遮挡（D2）、无假数据（D3）、减弱动画开关下过渡降级。
4. 手势/既有页面回归：底栏四 Tab 切换、旧聚合搜索设置项不受影响。