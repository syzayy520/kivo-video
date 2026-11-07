# Rex 1.0.2 · UI 结构复刻说明书

> 目标：`Rex_1.0.2.ipa`（SHA-256 `ef2c21cc761dfd65954460601275f677c65c90dacfc54652e50f4ec9565c4485`）
> 分析工具：REA v6.7.0（`open_binary` / `inspect_artifact` / `decode_interface_builder` / `inspect_analysis_view`）+ 本地 `assetutil` / `plutil` / `sips`
> 证据目录：`~/tools/rex-analysis/`（78 份中英文文案 JSON、6 份 Assets.car 元数据）
> 解包目录：`~/tools/rex-extract/Payload/Rex.app`

---

## 0. 证据口径与硬边界（先读）

| 标记 | 含义 | 来源 |
| --- | --- | --- |
| ✅ **确定** | 从随包资源直接读出，可复算 | Info.plist / `.strings` / Assets.car / nib / 文件名与像素尺寸 / REA Evidence |
| ◐ **推断** | 有明确依据的映射或行业默认值，本文逐条注明依据 | 组件枚举、iOS/SwiftUI 系统默认 |
| ❌ **待确认** | 静态包无法观测，必须真机或运行时取证 | 见 §9 |

**三条不可绕过的边界**：

1. **主二进制 FairPlay 加密**。`otool -l`：`LC_ENCRYPTION_INFO_64`，`cryptid 1`，`cryptsize 80,199,680`，`cryptoff 16384`。→ SwiftUI 视图树代码、`.frame()` / `.padding()` / 字号 / 精确间距常量**无法静态读取**。本文的「布局」给的是**组件契约与比例关系**，不是像素级 frame。
2. **业务界面无 nib/storyboard**。全包仅 `Launch Screen.storyboardc`（2 个 nib），其余界面均为 SwiftUI 运行时构建。→ 页面层级由 **39 个本地化模块 × 键前缀** + 资源枚举 + Scene 配置反推。
3. **无真机截图 / 录屏**。→ 运行时状态（滚动后大标题折叠、暗色实际观感、动态列数）不在证据内。

**REA Evidence 索引**

| Evidence ID | 操作 | 结论 |
| --- | --- | --- |
| `ev_1f79a61a52ce4d90d45a6d6e020e9e1c0423794d65e5cf58aecdde6da623e644` | `inspect_artifact` | 1145 节点 / 1215 出现项 / 1214 边，完整性矛盾 0 |
| `ev_6be889ca6994768b9c1b702d604a71fab38608f22f29b2a0572813f1415dd3f7` | `decode_interface_builder` | 2 个 storyboard 文档 / 21 节点 / 6 边 / 4 约束 |
| `ev_defa93b5ca57c3c55cbe836a4fb7218210e7a303bdf2e50f3451dbf9adc6406c` | `inspect_analysis_view` | 启动屏 21 个节点逐项属性 |
| `ev_325a622713d273e9c58915cafedba8dc352c51365b365c6a3a24408eba56ee66` | interface-builder summary | 覆盖率 9 / 层级不完整（私有归档字段） |

---

## 1. 应用身份与运行容器 ✅

| 属性 | 值 |
| --- | --- |
| Bundle ID | `tv.rexnow.app` |
| 版本 | 1.0.2 |
| 最低系统 | iOS 17.0 |
| 架构 | Mach-O 64-bit arm64，单架构，89.9 MB |
| UI 框架 | **SwiftUI**（仅 Launch Screen 走 UIKit storyboard） |
| 设备族 | iPhone + iPad（`UIDeviceFamily` 1/2） |
| 方向 | iPhone：Portrait / LandscapeLeft / LandscapeRight；iPad：四方向全支持 |
| 多 Scene | ❌ 关闭（`UIApplicationSupportsMultipleScenes = false`） |
| Scene Delegate | `Rex.RexSceneDelegate`（主）、`RexPlayerUIKit.PlayerExternalDisplaySceneDelegate`（外接显示器非交互场景） |
| 启动 Storyboard | `Launch Screen` |
| 语言 | 15 个 `.lproj`：ar / de / en / es / fr / it / ja / ko / pt / pt-BR / ru / zh-HK / zh-Hans / zh-Hant → **含 RTL（阿拉伯语），复刻必须处理镜像布局** |
| 包内文件 | 1214 条目；453 `.strings`、162 `.png`、153 `.jpg`、41 `.webp`、49 `.glsl`、6 `.car`、5 `.json`、1 `.ttf` |

---

## 2. 全局外壳与导航

### 2.1 iPhone 主导航：4 Tab ✅

来源 `en.lproj/AppShell.strings`（9 键，全部为外壳定义）：

| 键 | 英文 | 简体中文 | 位置 |
| --- | --- | --- | --- |
| `app.tab.library` | Libraries | 资源库 | Tab 1 |
| `app.tab.discover` | Discover | 发现 | Tab 2 |
| `app.tab.watching` | Watching | 在看 | Tab 3 |
| `app.tab.me` | My | 我的 | Tab 4 |

> ◐ **推断依据**：Tab 顺序按 `.strings` 内 JSON 键出现顺序 + 影视类 App 常规信息架构；实际顺序需真机确认。原生 `UITabBar` / SwiftUI `TabView` 未加密到资源层，无法直接证明。

### 2.2 全局动作 ✅

| 键 | 文案 | 说明 |
| --- | --- | --- |
| `app.action.search` | Search / 搜索 | 全局搜索入口 |
| `app.action.addLibrary` | Add Library / 添加资源库 | 资源库入口动作 |
| `app.search.chooseSources` | Choose Search Sources / 选择搜索源 | 多源聚合搜索 |
| `app.search.person.role.director` / `.lead` | Director / Lead | 人物卡片角色标签 → 详情页「演职员」区块 |

### 2.3 侧栏 / 副 Tab（Mac 与 iPad 变体）✅

| 键 | 文案 | 结构含义 |
| --- | --- | --- |
| `shell.copy.collapse.sidebar` / `mac.sidebar.collapse` | Collapse Sidebar / 收起侧栏 | 可折叠 NavigationSplitView |
| `mac.sidebar.expand` | Show Sidebar / 显示侧栏 | 展开态 |
| `shell.copy.secondary.tab.collapse.title` / `shell.format.secondary.tab.collapse.icon` | 副 Tab 折叠标题 / 折叠图标 %lld | **副 Tab 可折叠为图标**，带序号 |
| `mac.rail.next` / `.previous` / `.scrollHint` | 向后翻 / 向前翻 / 横向滚动 | 横向 rail 滚动，带分页提示 |
| `mac.menu.zoom` | Zoom / 缩放 | 窗口缩放菜单项 |

### 2.4 观影环境切换（跨页全局状态）✅

`watching.copy.tap.the.environment.name.or.avatar.to.switch.to.another.view` = 「点按环境名称或头像，切换到其他观影环境」→ 顶部有**环境名 + 头像**的可点击切换器，影响全 App 内容（首页、进度、日历）。`SettingsEnvironment`(94 键) / `Environment`(73 键) 为其配置面。

---

## 3. 设计系统（确定硬参数）

### 3.1 语义色板 ✅

来源 `RexDesign_RexDesign.bundle/Assets.car`（`assetutil -I`，10 条 = 5 色 × 明暗双外观；`RexDesign_RexDinoCore.bundle` 完全相同，为设计系统副本）。

| Token | Light | Dark | 色彩空间 | 用途推断 |
| --- | --- | --- | --- | --- |
| `canvasFill` | `#FFFFFF` | `#222222` | sRGB | 卡片 / 内容画布 |
| `groupedFill` | `#F2F2F7` | `#1C1C1E` | sRGB | 分组列表底 |
| `pageBackground` | `#F2F2F7` | `#000000` | extended sRGB / sRGB | 页面底色 |
| `hairline` | `#000000 @5%` | `#FFFFFF @10%` | sRGB | 分隔线 / 描边 |
| `washFill` | `#EDEDFB @50%` | `#3D3D41` | **Display P3** / sRGB | 洗色叠加 / 高亮底 |

> ⚠️ 复刻注意：`washFill` 浅色态是 **P3 色域 + 50% 透明度**，用 sRGB `#EDEDFB` 直接替代会有可见色偏；需按 P3→sRGB 转换或保留宽色域。
> ⚠️ `pageBackground` 浅色态是 **extended sRGB**（允许分量 >1），与普通 sRGB 同名值语义不同。

### 3.2 启动屏（唯一可拿到像素级约束的界面）✅

REA `decode_interface_builder` + `inspect_analysis_view` 解出 `Launch Screen.storyboardc`：

| 项 | 值 | 证据 |
| --- | --- | --- |
| 根视图 bounds | **393 × 852 pt** | `UIBounds` 二进制解码 `0,0,393,852` |
| 根视图 center | (196.5, 426) | `UICenter` 解码 |
| 根视图背景 | `RexLaunchBackground` = `#FFFFFF` / `#000000` | nib object 13 |
| Logo tint | `RexLaunchLogoTint` = `#000000` / `#FFFFFF` | nib object 9 |
| Logo 图 | `RexLaunchLogo`（Assets.car 命名资源） | nib object 11 |
| Logo imageView bounds | **76 × 76 pt** | `UIBounds` 解码 `0,0,76,76` |
| 约束 16 | `imageView.centerX = rootView.centerX` | `NSFirstAttribute=9` |
| 约束 17 | `imageView.centerY = rootView.centerY` | `NSFirstAttribute=10` |
| 约束 18 | `imageView.width = 76` | `NSConstant=76`, attr 7 |
| 约束 19 | `imageView.height = 76` | `NSConstant=76`, attr 8 |
| 内容模式 | `UIContentMode = 1`（ScaleAspectFit） | nib object 8 |
| autoresize | 根视图 `UIAutoresizingMask=18`，imageView `=36`，`UIAutoresizeSubviews=false` | nib |

> ✅ 结论：**启动屏 = 纯色底 + 76×76 居中 Logo，Auto Layout 四约束，无边距常量**。这是可直接 1:1 复刻的界面。
> ◐ 393×852 是 nib 归档的设计时画布（对应 iPhone 15/16 逻辑分辨率 @3x → 1179×2556），据此推断全 App 以 **393pt 宽**为设计基准；安全区按系统标准（顶 59 / 底 34 pt）。

### 3.3 字体 ✅

| 项 | 结论 |
| --- | --- |
| 全包唯一字体文件 | `RexPlayer_RexPlayerKSPlayer.bundle/Fonts/PingFangSC.ttf` |
| 用途 | **字幕渲染**（KSPlayer 字幕层），非 UI 字体 |
| UI 字体 | 无内嵌美术字体 → **系统字体（SF Pro / PingFang SC）**，走 Dynamic Type |
| 设计基准 | 与项目既有结论一致：靠字重/字号/字距精控，不用特殊字体 |

### 3.4 分隔与描边 ✅

`hairline` 为唯一描边 token（5% 黑 / 10% 白），`RexDesign` bundle 内无圆角/阴影/间距的数值定义 → **这些常量在加密的 Swift 代码里，属 ❌ 待确认**（见 §9）。

---

## 4. 资源规格与比例（确定）

`sips` 实测全包图片像素：

| 资源类别 | 实测尺寸 | 比例 | 数量 | 复刻建议 |
| --- | --- | --- | --- | --- |
| 竖版海报 / mock poster | 300×450、342×513 | **2:3** | 69 + 19 | 卡片主图统一 `aspectRatio(2/3)` |
| 横版剧照 / backdrop | 1280×720 | **16:9** | 65 | 轮播 / 详情头图统一 `16:9` |
| App 图标 / Dino 图标 | 1024×1024 | 1:1 | 90 | 图标方图 |
| 资源徽章（Badge） | 高 194px 恒定，宽 330 / 339 / 500 / 537 / 570 / 663 / 714 / 729 / 744 / 780 / 798 等 | 高度固定、宽度随文案/图形变化 | ~40 | **胶囊按内容自适应宽度，高度锁定** |
| `@2x` AppIcon | 120×120 px → 60×60 pt | 1:1 | 2 | 系统图标槽 |

**主 `Assets.car`（77 MB，911 项）构成**：

| 类型 | 数量 | 说明 |
| --- | --- | --- |
| Icon Image | 270 | App 图标变体体系 |
| IconGroup | 138 | 图标分组 |
| IconImageStack | 135 | 图标叠层（前景/背景/渐变组合） |
| MultiSized Image | 90 | 多尺寸图 |
| Image | 85 | 普通图 |
| Color | 141 | 绝大多数是 App 图标配色（`RexDinoIcon-*_Assets/Color-N`），**不是 UI 语义色** |
| Named Gradient | 46 | 全部为图标背景渐变，`Gradient Start/Stop = 0.500,0.000 - 0.500,1.000`（垂直向下），`Gradient Stops=[0,1]` |
| Vector | 3 | 矢量图 |

> ⚠️ **易踩坑**：主 `Assets.car` 里 141 个 Color 几乎全是 App 图标变体的灰度调色（`#FFFFFF` / `#313131` / `#141414` / `#ECECEC`），**不要误当作 UI 主题色**。真正的 UI 色板在 `RexDesign_RexDesign.bundle`（§3.1）。
> ✅ 图标体系命名规则：`RexDinoIcon-{颜色}-{medium|wide}-{light|dark}.png` + `halloween-` 限定版 + `{color}` ∈ black/blue/coral/gold/gray/ice/mint/orange/pink/purple/teal → **13 配色 × 2 比例 × 2 外观 × 万圣节变体**；另有 `RexAppIconPrimary-{light,dark}.png`。

---

## 5. 首页组件契约（可直接复刻的枚举）✅

来源 `en.lproj/HomeEditor.strings`（161 键）。首页是**用户可自定义的模块流**，编辑器提供以下样式枚举：

| 样式键 | 中文 | 组件类型 | 布局契约（◐ 推断依据：名称语义 + 2:3 海报比例） |
| --- | --- | --- | --- |
| `content.cover.small` | 内容封面 - 小 | 单封面 | 横向卡片列表，小尺寸 2:3 封面 |
| `content.cover.middle` | 内容封面 - 中 | 单封面 | 横向卡片列表，中尺寸 |
| `content.cover.wide` | 内容封面 - 宽 | 单封面 | 16:9 宽卡 |
| `list.preview.small` | 列表预览 - 小 | 片单预览 | 网格缩略 + 标题 |
| `list.preview.center` | 列表预览 - 中 | 片单预览 | 居中排布 |
| `list.preview.large` | 列表预览 - 大 | 片单预览 | 大图主卡 |
| `list.preview.extra.large` | 列表预览 - 超大 | 片单预览 | 近全宽主卡 |
| `list.preview.wide` | 列表预览 - 宽 | 片单预览 | 横向宽卡 |
| `list.preview.full.width` | 列表预览 - 全宽 | 片单预览 | 撑满容器宽 |
| `viewing.progress.full.width` | 观看进度 - 全宽 | 进度卡 | 全宽 + 进度条 + 集数（`home.accessibility.episode.progress`） |
| `rankings.compact` | 排行榜 - 紧凑 | 榜单 | 紧凑行 + 序号（`home.format.rank = Rank %lld / 第 %lld 名`） |
| `poster.collage.extra.large` | 海报拼贴 - 超大 | 拼贴 | 多张 2:3 海报叠拼 |
| `poster.wall.two.rows` | 海报墙 - 双排 | 海报墙 | **双排**横向滚动网格 |
| `carousel.content` / `enable.carousel` | 轮播内容 / 开启轮播图 | 精选轮播 | `home.accessibility.featured.carousel`；16:9 backdrop 自动分页 |

**分区（Section）操作契约** ✅ —— `Home.strings`：

| 操作 | 键 | 中文 |
| --- | --- | --- |
| 拖动排序 | `home.section.drag.to.sort` | 拖动分区调整顺序 |
| 快速排序 | `home.section.quick.sort` | 快速排序 |
| 上移 / 下移 | `home.copy.move.up` / `.move.down` | 上移 / 下移 |
| 移到顶部 / 底部 | `home.copy.move.to.top` / `.move.to.bottom` | 移到顶部 / 移到底部 |
| 显示 / 隐藏 | `home.section.show` / `.hide` / `.hidden` | 显示 / 隐藏 / 已隐藏 |
| 插入模块 | `home.copy.insert.widget` | 插入模块 |
| 删除分组 | `home.copy.confirm.deletion.the.entire.widget.group.will.be.deleted` | 二次确认「整个模块分组将被删除」 |
| 筛选 | `home.format.filter` | `%@筛选` |
| 大标题列表 | `home.accessibility.large.title.list` | 大卡片单（无障碍标签 → 存在大标题模式） |

**首页内置模块源** ✅ —— `HomeBuiltIn.strings`（37 键）：继续观看、近一月热门剧集/综艺、今日趋势、本周趋势、高分剧集/电影、备受欢迎电影、热门人物、正在播出、正在热映、最新电影、今日动漫、分类浏览、电影公司、播出平台、以及 20+ 类型/平台（动作/剧情/喜剧/科幻/悬疑/惊悚/动画/纪录/儿童/西部/家庭/真人秀/电视剧/电影/优酷/爱奇艺/腾讯视频/芒果TV…）。

---

## 6. 页面结构逐页

### 6.1 Discover / 首页
```
TabView
└─ NavigationStack
   ├─ 顶部：环境切换器（环境名 + 头像，点按切换） / 全局 Search / Add Library
   ├─ 精选轮播 Carousel（可开关，16:9 backdrop，分页吸附）
   └─ 模块流 LazyVStack（用户自定义顺序）
      ├─ Section（标题 + 「更多/筛选」+ Show/Hide 状态）
      │  └─ 组件（§5 枚举之一）
      ├─ 「继续观看」卡片（2:3 封面 + 进度条 + `Continue %@` / `Watched %d of %d episodes`）
      ├─ 排行榜紧凑行（Rank %lld）
      └─ 状态位：Loading More / Widget building in progress / Refresh / 错误重试
```
◐ 推断依据：`Home`/`HomeEditor`/`HomeBuiltIn`/`AppShell` 键前缀交叉；无 nib 佐证 Stack 类型。

### 6.2 Libraries（资源库，460 键，最大模块）
- 库类型枚举：Emby / Jellyfin / WebDAV / SMB(`AMSMB2.framework`) / 百度网盘 / 阿里云盘 / Drivemby / 本地文件 / Trakt
- 分组：`library.groups.new.placeholder`、`library.groups.editor.footer`（拖动排序、隐藏分组不显示）
- Section 展开/收起：`library.section.expand` / `.collapse`
- 文件匹配流程：`library.file.match.*`（读取目录 → 识别 → 保存 `Saving %lld/%lld` → 解绑）
- 进度状态：`library.drivemby.progress.waiting / .recognizing / .uploading / .done`
- Sheet：`library.drivemby.sheet.title/email/sync`（表单 + 邮箱 + 同步目录）

### 6.3 Watching（在看，147 键）
- 顶部环境切换器（同 §2.4）
- 继续观看卡片流（可移除）
- 剧集进度 / 折叠记录（`watching.format.collapse.records = 折叠 %lld 条`、`watching.copy.tap.to.expand`）
- 未识别记录展开 / 收起
- 追剧日历入口（到底部长按查看完整日历，`appWatching.copy.you.ve.reached.the.bottom.long.press.to.view.the.full.calendar`）
- 弃剧（`watching.copy.drop.show.explanation`，隐藏整部剧 + 移出待看）、撤销（`mac.tracking.dropUndone`）

### 6.4 Details（详情，176 键）
- 头图 16:9 backdrop + 2:3 海报叠放（◐ 推断：两类资源同时存在）
- 标题 / 年份 / 上映日期 / 集数 `%lld/%lld`
- 主操作：`Watch`(追剧) / `Mark as Watched` / `Favorite` / `Watchlist`
- **长按可替换主操作**（`details.copy.touch.and.hold.to.replace.this.action`）
- 演职员（Director / Lead，`Known for: %@`）
- 展开媒体信息 / 观看进度 / 剧集列表 / 徽章（§4 Badge）
- 后台保存进度 + 完成通知

### 6.5 Search（127 键）
- 全局入口 → 多源选择（Choose Search Sources）→ 结果按 影视 / 人物 / 类型 分组
- 人物卡片带角色标签（Director / Lead）

### 6.6 Calendar / People / Genres / Catalog / MediaMarks
| 模块 | 键数 | 结构要点 |
| --- | --- | --- |
| Calendar | 31 | 周视图（`%1$@ · %2$@–%3$@` 可见周范围）、更新集数、首播日期、播出提醒 |
| People | 47 | 人物详情 + 代表作 |
| Genres | 29 | 类型网格（TMDB 类型 ID → 本地化名） |
| Catalog | 32 | 目录浏览 |
| MediaMarks | 18 | 收藏 / 待看，**长按节目卡片**加入 |

### 6.7 Settings（699 键 + 8 个子模块）
分组：`Settings`(699) / `AppSettings`(292) / `SettingsPlayback`(197) / `SettingsSubtitle`(133) / `SettingsModules`(102) / `SettingsEnvironment`(94) / `SettingsFeedback`(108) / `SettingsDino`(221)。
- 界面页含 **Reduce Animation Effects（减弱动画效果）** 与 **Haptics（自动/禁用，低电量自动关闭）** → 见动效文档 §1
- `settings.split.placeholder = 在左侧选择一项设置` → **iPad/Mac 双栏 Split View**
- 拖动排序：`settings.aggregated.sources.skip.footer` / `settings.playback.skip.sources.footer`（数据源优先级）

### 6.8 Onboarding（132 键）
4 页引导，文案序列（✅ 确定）：
| 序 | 主标题 | 副标题 |
| --- | --- | --- |
| 1 | Connect All Your Media / 连接你的所有资源 | 统一浏览 Emby、网盘和本地媒体库 |
| 2 | Your Home, Your Way / 首页，由你定义 | 所有首页模块都能自由组合，也可以直接滑动浏览 |
| 3 | Every Watch Leaves a Record / 每一次观看，都有迹可循 | 观看时长、类型偏好和趋势持续汇总 |
| 4 | Your Next Watch Is Coming into View / 想看的，正在来到眼前 | 详情页加入待看 |
- 控件：`Get Started` / `Previous` / `Continue` / `onboarding.copy.onboarding.progress`（引导进度指示）
- 素材：102 条 `onboarding.media.*.title/tagline` 真实影视条目（沙丘、盗梦空间、切尔诺贝利…），用于引导页示例卡片

### 6.9 PRO（123 键）
- 方案卡：月度 / 年度 / 永久授权（`pro.plan.*`）
- 免费 vs PRO 功能对照（`pro.copy.free` / `.unlimited` / `3 per Day` / `7 Days`）
- 功能清单：高画质播放 / AI 字幕 / 智能文件识别 / 追剧通知 / 播出通知 / 观影日历 / 家庭共享 / 家庭成员观影环境
- **孵蛋抽奖**（`pro.lottery.*`）与**购买成功庆祝页**（`pro.celebration.*`）→ 见动效文档 §3.B
- 边界态：邮箱未关联 / 订单冲突 / 待批准 / 设备名额满 / 每日 3 次免费播放上限

### 6.10 Player（播放器）
- 引擎：`KSPlayer` + 自研 `RexPlayer` 模块（`RexPlayer_RexPlayerUIKit.bundle` / `RexPlayer_RexPlayerKSPlayer.bundle`）
- 超分：49 个 Anime4K `*.glsl` + `default.metallib`，模式 A / A+A / B / B+B / C / C+A × HQ / Fast
- 字幕：`PingFangSC.ttf`、`PlayerUIShowcaseSubtitle.srt`、AI 字幕（onnxruntime）
- 弹幕：`OfficialModules/danmu.js`、固定弹幕转滚动弹幕
- 外接显示器：独立 Scene（`Player External Display`）
- 手势与过渡 → 见动效文档 §3.C

---

## 7. 资源引用清单（bundle → 用途）✅

| Bundle | 内容 | 用途 |
| --- | --- | --- |
| `RexDesign_RexDesign.bundle` | 5 语义色 Assets.car、`ShowcaseMediaLibrary.json`、~30 张 showcase 海报/剧照 | **设计系统 + 展示素材** |
| `RexDesign_RexDinoCore.bundle` | `rex-layers.json`（86 KB）、设计色副本 | **小恐龙分层动画**（见动效文档 §2） |
| `RexDesign_RexBadgeCore.bundle` | `ResourceBadgeDefault.json` + 40+ `rex-default-badge-*.webp` | 媒体信息徽章（4K/HDR/DoVi/Atmos/Netflix…） |
| `RexHome_RexHome.bundle` | 20+ `mock-*.jpg`、`Contents.json`（`dayUnderline.png`, template-rendering-intent） | 首页占位/示例卡片、日历下划线模板图 |
| `RexHome_RexHomeCore.bundle` | Assets.car（1.2 MB） | 首页组件图标 |
| `RexPlayer_RexPlayerUIKit.bundle` | Assets.car + `default.metallib` + 8 语言 strings + 字幕 `.srt` | 播放器 UI |
| `RexPlayer_RexPlayerKSPlayer.bundle` | `Fonts/PingFangSC.ttf` | 字幕字体 |
| `RexScript_RexScript.bundle` | `OfficialModules/manifest.json` + `rex-tmdb.js` / `bangumi.js` / `danmu.js` / `nlsearch.js` + `cheerio.bundle.js` | **JS Widget 模块**（首页数据源） |
| `KSPlayer_KSPlayer.bundle` | 49 个 Anime4K glsl + metallib + 本地化 | 视频超分（非 UI 动效） |
| `Rex.ckdb` | 19 KB ASCII，CloudKit schema（`iCloud.tv.rexnow.app` / zone `rex-main`） | 数据模型，非 UI；含 `catalog` 等 Record Type |
| `Frameworks/` | 22 个：FFmpeg 系列 / libass / libplacebo / libdav1d / libdovi / onnxruntime / AMSMB2 / lcms2 … | **无任何第三方 UI 动效库** |

---

## 8. 复刻落地清单

| # | 项 | 等级 | 状态 |
| --- | --- | --- | --- |
| 1 | 393×852 设计基准 + 2:3 / 16:9 比例体系 | ✅ | 可直接落地 |
| 2 | 5 个明暗语义色（注意 P3 / extended sRGB / alpha） | ✅ | 可直接落地 |
| 3 | 启动屏 76×76 居中 Logo + 四约束 | ✅ | 可 1:1 落地 |
| 4 | 4 Tab + 全局 Search/AddLibrary + 环境切换器 | ✅ | 可直接落地 |
| 5 | 首页 15 种组件样式枚举 + 分区操作 | ✅ | 可直接落地（契约层） |
| 6 | 39 个模块的完整中英文案映射 | ✅ | `~/tools/rex-analysis/strings/*.json` |
| 7 | 各页面区块构成 | ◐ | 键前缀反推，需真机校对 |
| 8 | 圆角 / 阴影 / 具体 padding / 字号字重 | ❌ | 加密代码内，见 §9 |
| 9 | 动态列数（iPhone/iPad/Mac）、大标题折叠阈值 | ❌ | 需运行时 |

---

## 9. 待确认项与转确定方法

| 待确认项 | 为什么拿不到 | 转确定方法 |
| --- | --- | --- |
| SwiftUI `.padding` / `.frame` / 圆角 / 阴影数值 | 主二进制 FairPlay 加密 | ① 越狱设备 dump 解密后 `class-dump` / Hopper；② 真机截图 + 像素标尺反算 |
| 字号 / 字重 / 行距刻度 | 同上 + 走 Dynamic Type | 真机逐档 Dynamic Type 截图量测 |
| 首页模块实际默认顺序与默认显隐 | 配置在 CloudKit / 用户数据，随包只有文案 | 全新安装真机首次启动录屏 |
| 各动效 duration / easing / spring 参数 | 同上 | 见动效文档 §6（逐帧录屏 / 运行时 hook） |
| 运行时暗色实际观感 | 静态色值不含材质/模糊叠加 | 真机暗色截图取色 |

---

*本文所有 ✅ 结论均可在 `~/tools/rex-extract/Payload/Rex.app` 与 `~/tools/rex-analysis/` 复算。*
