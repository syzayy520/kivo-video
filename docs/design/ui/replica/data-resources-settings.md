# 数据与资源设置子页（Data & Resources）· 家族树设计（实现前置门闸）

> 来源：用户 2026-11-13 提供的「数据与资源」17 帧截图（根页 ON/OFF、元数据/Logo/海报语言 picker、
> 资源聚合 3 开关、数据来源 8 计数钻入、内容加载 TMDB API、选片资源排序 4 准则 + 当前排序规则 picker、
> 资源聚合(详情)/搜索来源/观看记录/影片信息/片段标记/收藏/观影统计/待看 8 个来源页）。
> 延续 261→262→263→264→265 设置家族节奏。本稿先定家族树与逐行真实能力，**未完成本设计不得实现**。
> 措辞遵守 D6：参考产品名「Rex」一律改称「本机」，不写参考产品名，统一称「参考」。

## 〇、能力审计（2026-11-13 实测）

| 审计项 | 证据 | 结论 |
| --- | --- | --- |
| 每源「首页聚合」开关 | SourceRecord.kt:22 `aggregateEnabled`；SourceRepository.kt:27 `setControl(Control.AGGREGATE)`；SourceEligibility.kt:8 `home()` | **真**，可读写 |
| 每源「聚合搜索」开关 | SourceRecord.kt:23 `aggregateSearchEnabled`；SourceRepository.kt:36 `Control.AGGREGATE_SEARCH`；AggregateSession.kt:48-50 过滤 `!isSmb` | **真**，Emby 可读写；**SMB 不参与聚合搜索** |
| 源记录持久化 | SourceStore.kt:18 `noBackupFilesDir/sources/accounts.bin`（加密 schema 2） | **真**，读模型经 SourceRepository |
| 本机观看记录/续播 | WatchStore.kt:19 `filesDir/watch.json`；MainActivity.kt:146 `recentKeys(10)`+`progressOf` | **真·仅本机**，按 path，无按源区分、无 Trakt |
| 本机观影统计 | WatchStore.kt:21 `dailyMs` 按天累计 | **真·仅本机**，无 30 天跨设备同步 |
| 本机收藏/待看/已看 | FilmMarksStore.kt:86 `filesDir/media/marks/titles.json`（favorite/watchlist/watched） | **真·仅本机**，按 TMDB id，无 Trakt |
| TMDB 影片信息 | TmdbClient.kt:21 `API_BASE="https://api.tmdb.org/3"`；:102 `language=zh-CN` 硬编码 | **真·固定简体中文**，无语言偏好持久化 |
| TMDB Key/代理 | TmdbClient.kt:44 `apiKey()`（读 `tmdb_key.txt`）/:53 `proxyAddr()`（读 `tmdb_proxy.txt`）；SettingsStore.kt:27-28 | **真·只读**，Key 经文件写入，**无 UI 输入/自定义地址/测试连接** |
| 片段标记（跳过片头/片尾） | PlayerPanelForms.kt:42-43 `disabled("跳过片头","片头标记尚未接入")`；SettingsPlaybackSub.kt:111 脚注 | **无**（6 来源全灰） |
| 选片资源排序准则 | AggregateFilter.kt:24 `sortings`=最新更新/最新上映/影片评分（Emby SortBy 0/4/3），在搜索页选 | **无**色彩范围/分辨率/文件大小/码率排序；无「当前排序规则」偏好 |
| 全局聚合偏好 | grep `保持聚合\|优先当前库\|聚合有效` = 0 命中 | **无**（仅每源开关） |
| 搜索来源 TMDB/网络硬盘/AI | AggregateSearchModel 仅 Emby；LocalSearch.kt 仅 MediaStore；grep `aiSearch\|AI 搜索` = 0 | TMDB 非聚合搜索源；WebDAV 未接入聚合搜索；**AI 搜索无** |
| 拖拽排序（≡） | 源顺序为列表固定序，无用户排序偏好持久化 | **无**（不放假拖拽柄，脚注披露） |
| 问题反馈 | grep `反馈\|feedback` 无后端 | **无** |
| 行原语 | SettingsRows.kt：SectionCard:45/ChevronRow:91/ValueRow:119/SwitchRow:155/StepperRow:208/LinkRow:131 | 齐备，+IconChevronRow（图标+计数） |
| 路由 | SettingsNav.kt:20 已有 DATA；SettingsPage.kt:72 分派 SettingsDataSub | 不升级导航栈；钻入用子页内部屏枚举 |

**总判定**：真实部分 = 每源聚合/搜索开关（真读写）、本机记录/统计/收藏/待看（真·只读恒启用）、
TMDB 影片信息（真·固定）、TMDB Key/代理（真·只读）、媒体服务器管理导航。
其余（Trakt/片段标记 6 源/自定义 TMDB API/测试连接/图片修复/语言偏好/全局聚合/排序准则/AI 搜索/反馈/拖拽）**全灰+非空原因**。
**不放假开关、不假计数、不放假拖拽柄、不放 ON 态死 UI**。

## 一、页面结构（根页 + 8 来源钻入页）

### 根页「数据与资源」（长滚动，5 卡）

| 组 | 行 | 参考形态 | 本版本判定 |
| --- | --- | --- | --- |
| 语言偏好（无节头卡） | 元数据语言偏好 | stepper「系统默认⇕」 | **灰 StepperRow** 值「简体中文」+原因「语言固定简体中文，偏好未持久化」 |
| | Logo 语言 | stepper「跟随元数据语言⇕」 | **灰** 值「自动择优」+原因「Logo 语言随自动择优，未持久化」 |
| | 海报语言 | stepper「跟随元数据语言⇕」 | **灰** 值「自动择优」+原因「海报语言随自动择优，未持久化」 |
| 资源聚合 | 聚合有效资源库 | Switch ON | **灰 OFF**+「无全局聚合开关，按资源库单独配置」 |
| | 打开资源库后保持聚合 | Switch ON | **灰 OFF**+同上 |
| | 浏览资源库时优先当前库 | Switch OFF | **灰 OFF**+同上 |
| 数据来源 | 资源聚合(详情) | 图标+「3/3›」 | **真钻入**；计数=非隐藏源中 aggregateEnabled 启用数/总数（实测） |
| | 搜索来源 | 图标+「6/6›」 | **真钻入**；计数=非隐藏 Emby 中 aggregateSearchEnabled 启用数/总数（实测） |
| | 观看记录来源 | 图标+「4/4›」 | **真钻入**；计数「1/1」（仅本机可用，Trakt 未接入不计） |
| | 影片信息来源 | 图标+「4/4›」 | **真钻入**；计数「1/1」（仅 TMDB） |
| | 片段标记来源 | 图标+「6/6›」 | **真钻入**；值「未接入」（不假计数），钻入页六源全灰+原因 |
| | 收藏数据来源 | 图标+「4/4›」 | **真钻入**；计数「1/1」（仅本机） |
| | 观影统计 | 图标+「4/4›」 | **真钻入**；计数「1/1」（仅本机） |
| | 待看来源 | 图标+「1/1›」 | **真钻入**；计数「1/1」（仅本机） |
| 内容加载 | 自定义 TMDB API | Switch ON | **灰 OFF**+「API 地址固定官方域，自定义未接入」 |
| | API 地址 | 文本「https://api.themoviedb.org」 | **真只读** ValueRow 显示真实 `API_BASE`（api.tmdb.org/3） |
| | API Key | 输入框「请输入 API Key」 | **真只读** ValueRow「已配置/未配置」+脚注「经 filesDir/tmdb_key.txt 写入，UI 输入未接入」 |
| | 测试连接并保存 | 蓝链接 | **灰 LinkRow**+「连接测试未接入」 |
| | 图片加载修复 | Switch OFF | **灰 OFF**+「图片加载修复未接入」 |
| | 代理 | （参考无，本版本既有真实项） | **真只读** ValueRow `tmdbProxy ?: "直连"` |
| 选片资源排序 | 当前排序规则 | stepper「自定义⇕」 | **灰** 值「未接入」+「排序规则偏好未接入」 |
| | 色彩范围 | stepper「从大到小⇕」+≡ | **灰**+「按色彩范围排序未接入」（≡ 不放置） |
| | 分辨率 | stepper「从大到小⇕」 | **灰**+「按分辨率排序未接入」 |
| | 文件大小 | stepper「从小到大⇕」 | **灰**+「按文件大小排序未接入」 |
| | 视频码率 | stepper「从大到小⇕」 | **灰**+「按视频码率排序未接入」 |
| （根页底） | 媒体服务器管理 | （参考无，本版本真实导航） | **真 LinkRow**→onOpenSources |

脚注：语言偏好脚注 / 资源聚合脚注（指向每源开关）/ 数据来源脚注（「来源开关仅用于当前环境」改写）/
内容加载脚注（Key 未配置徽章隐藏、代理仅 TMDB 域）/ 排序脚注（聚合搜索实为最新更新/最新上映/影片评分，在搜索页选）。
**参考 ON 态条件行不放置**：picker 弹层（语言/排序）灰 stepper 不可达 ⇒ 不放死 UI；≡ 拖拽柄无后端 ⇒ 不放置（披露 D2）。

### 8 来源钻入页（通用渲染器，数据驱动）

| 页 | 行 | 判定 |
| --- | --- | --- |
| 资源聚合(详情) | 每源（非隐藏） | **真 SwitchRow** checked=aggregateEnabled，onChange→setControl(AGGREGATE)；空态→「还没有资源库」+LinkRow 管理 |
| | 脚注 + 媒体服务器管理 LinkRow | 真导航；脚注「拖拽排序未接入，顺序固定」 |
| 搜索来源 | TMDB | **灰 OFF**+「TMDB 提供评分与首页推荐，不参与聚合搜索」 |
| | 网络硬盘 | **灰 OFF**+「WebDAV 未接入聚合搜索」 |
| | 每源 Emby | **真 SwitchRow** checked=aggregateSearchEnabled→setControl(AGGREGATE_SEARCH) |
| | 每源 SMB | **灰**+「SMB 不参与聚合搜索」 |
| | AI 搜索 | **灰 OFF**+「AI 搜索未接入」 |
| 观看记录来源 | 本机 | **真只读恒启用** SwitchRow checked=true enabled=false+「本机观看记录，始终启用」 |
| | Trakt | **灰 OFF**+「Trakt 未接入」 |
| 影片信息来源 | TMDB | **真只读恒启用**+「固定简体中文」 |
| | 每源 | **灰**+「元数据源优先级未接入」 |
| 片段标记来源 | 本机手动标记/TheOtherDB/Emby 章节/TheIntroDB/IntroDB/PublicMetaDB | **全灰 OFF**+「片头/片尾标记尚未接入」（与 PlayerPanelForms 一致） |
| 收藏数据来源 | 本机 | **真只读恒启用**+「本机收藏，始终启用」 |
| | Trakt | **灰 OFF**+「Trakt 未接入」 |
| 观影统计 | 本机 | **真只读恒启用**+「本机统计，始终启用」 |
| | Trakt | **灰 OFF**+「Trakt 未接入」 |
| 待看来源 | 本机 | **真只读恒启用**+「本机待看，始终启用」 |
| | Trakt | **灰 OFF**+「Trakt 未接入」 |

各页脚注改写为 Android 现实（见 §四）。本机/Trakt 页不列服务器行（记录仅存本机，无按源区分 ⇒ 不放假源行）。

## 二、家族树（新增 3 文件 + 重写 1 + 既有家族小升级，不新建包、不平铺）

| 文件 | 职责 | 动作 |
| --- | --- | --- |
| `ui/design/settings/SettingsDataSub.kt` | 数据根页路由 + 内部 9 屏状态(ROOT+8 来源)+BackHandler；持 SourceRepository + 源列表态 + onControl 写回 | **重写** ≤300 |
| `ui/design/settings/SettingsDataRoot.kt` | 根页 5 卡列表（语言/聚合/数据来源/内容加载/排序），只组合行原语 + 调 Tmdb 卡 | **新增** ≤300 |
| `ui/design/settings/SettingsDataSources.kt` | 8 来源钻入页通用渲染器（数据驱动：本机只读/外部灰/服务器真开关/服务器灰） | **新增** ≤300 |
| `ui/design/settings/SettingsDataTmdb.kt` | 内容加载卡 + 选片资源排序卡（纯 composable，收 store） | **新增** ≤300 |
| `ui/design/settings/SettingsRows.kt` | +IconChevronRow（leading 图标 + 计数「N/N」+ ›，enabled 灰化） | 改（小升级）≤300 |
| `ui/design/settings/SettingsPage.kt` | 根页 DATA 入口标题改「数据与资源」+ 副标题 | 改（小） |
| `ui/design/settings/SettingsNav.kt` | DATA 路由已有 | **不改** |
| `ui/design/settings/SettingsStore.kt` | TMDB Key/代理只读已有；不新增假键 | **不改** |

依赖方向：settings UI → SourceRepository/SourceStore（真读写每源开关）/ SettingsStore（TMDB 只读）/
WatchStore·FilmMarksStore 仅「本机恒启用」语义引用（不读其数据造假计数，本机可用=1 为真）。
**不新增持久化键**：灰开关不持久化；每源开关复用既有 accounts.bin（无新存储）。
内部钻入不升级 SettingsNav 栈：子页内 `rememberSaveable` 屏枚举 + BackHandler（内层先消费）。
源列表态：SettingsDataSub `LaunchedEffect` IO 读 SourceRepository → mutableStateOf；onControl 写后重读。

## 三、默认值与披露

- 灰开关恒 OFF 不可切换；无新增持久化键 ⇒ 无默认值债。每源开关默认值沿用 accounts.bin（aggregateEnabled/aggregateSearchEnabled 默认 true）。
- D1：参考 ON 态条件行（语言/排序 picker 弹层、自定义 API 输入、测试连接）不放置，灰行不可达；接入时补家族。
- D2：≡ 拖拽排序柄无后端 ⇒ **不放置**（不放假拖拽控件），脚注披露「顺序固定，拖拽排序未接入」。
- D3：「Rex」按 D6 改称「本机」；片段标记「Rex 手动标记」→「本机手动标记」（仍灰）。
- D4：数据来源计数为真：资源聚合/搜索按实测源记录算；本机类「1/1」（仅本机可用，Trakt 未接入不计可用）；片段标记「未接入」（无后端不假计数）。设备无源时聚合/搜索「0/0」+空态。
- D5：API 地址显示真实 `API_BASE`（api.tmdb.org/3），与参考 themoviedb.org 措辞异，真实值优先；Key 经文件写入非 UI。
- D6：本机记录/统计/收藏/待看「恒启用」用只读 Switch（checked=true enabled=false，淡绿轨）+副标题，非假可切换开关。
- D7：SMB 在「搜索来源」灰（不参与聚合搜索，AggregateSession 实测过滤）；在「资源聚合(详情)」可真开关（home 聚合含 SMB）。

## 四、脚注改写（形态对齐、内容诚实）

- 语言偏好：「元数据固定简体中文，Logo 与海报按自动择优（优先高清无字、中文标题）。语言偏好持久化尚未接入。」
- 资源聚合：「聚合按资源库单独配置：在「资源聚合(详情)」与「搜索来源」中逐源开关，或在资源库管理页修改。无全局聚合开关。」
- 数据来源：「来源开关仅作用于当前环境。本机记录（观看/收藏/统计/待看）存于本机，不随资源库区分；Trakt 等外部源尚未接入。」
- 内容加载：「未配置 Key 时评分徽章整体隐藏（不显示 0 分）。Key 经 filesDir/tmdb_key.txt 写入；代理仅作用于 TMDB 域，内网源不受影响。」
- 排序：「聚合搜索当前按「最新更新 / 最新上映 / 影片评分」排序，在搜索页选择。按色彩范围/分辨率/文件大小/码率排序与自定义规则尚未接入。」
- 资源聚合(详情)：「控制该资源库内容是否加入首页聚合。顺序固定，拖拽排序未接入。添加/编辑/隐藏资源库在资源库管理页进行。」
- 搜索来源：「控制该资源库是否参与跨服务器搜索。SMB 不参与聚合搜索；TMDB 提供评分与首页推荐，不参与搜索；AI 搜索未接入。」
- 观看记录：「观看记录仅存本机（watch.json），按文件路径记录续播位，「继续观看」读取本机记录。Trakt 同步未接入。」
- 影片信息：「影片信息来自 TMDB，固定简体中文。按资源库区分元数据源与优先级尚未接入。」
- 片段标记：「片头/片尾与片段标记尚未接入，播放器内「跳过片头/片尾」同样置灰。接入后可按来源自动跳过。」
- 收藏/待看：「收藏与待看仅存本机（media/marks/titles.json），按影片标记。Trakt 同步未接入。」
- 观影统计：「观影统计仅存本机，按天累计观看时长。30 天跨设备同步与 Trakt 未接入。」

## 五、闸门

1. 视觉：根页 5 卡 + 8 来源页节头/卡/行形态/脚注/图标计数与参考逐行对位（灰化适配披露）。
2. 无假状态：灰行均带非空原因；无假开关/假计数/假拖拽柄；ON 态死 UI 不放置；本机恒启用为只读非假可切换。
3. 真实复算：资源聚合(详情)/搜索来源每源开关真机切换→accounts.bin 写回→重读复算（含 SMB 搜索灰）；数据来源计数随源记录实测；TMDB Key/代理只读真值；钻入/返回栈真机验证。
4. 行数：3 新文件 + 重写 SettingsDataSub 各 ≤300；SettingsRows 升级后 ≤300。
5. 真机：K40 b6e950ee 截图 + 导航复算 + 设置其余页不回归。
6. 证据：docs/evidence/p266/data-resources/。