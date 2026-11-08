# P3 详情页交接 · 2026-11-08 深夜

> 首页（Codex 接管）已定稿且**禁改**；P3 详情页代码已实现并编译通过，但**真机验证未完成**。
> 本文档 = 下一会话的完整上下文：状态、文件、坑、命令、队列。执行会话按 §三 收口。

## 一、仓库与设备状态

| 项 | 状态 |
| --- | --- |
| 分支 | codex/kivo-video-p0-ui-01-playback-overlay-v13，已推送至 e4fe271 |
| Codex 首页接管 | 9bd2b17（预告片/下拉缩放/浅淡取色/新导航），用户验收通过，**禁改** |
| P3 实现 | **工作区未提交**（本交接提交）：TmdbDetail.kt + ui/detail/{MovieDetailActivity,DetailSections}.kt + manifest 注册 + MainActivity 一行接线 |
| P3 编译 | `:app:compileDebugKotlin` 通过（最后一轮无 error） |
| P3 真机验证 | **未完成**——捕获序列被污染（见 §三） |
| 设备 | K40 b6e950ee；我方 P3 构建已覆盖安装；前台 = com.kivo.cinema/.MainActivity |
| 设备污染源 | 机上另有其它应用（GLM 购物页等）——验证前必须 force-stop 并确认前台包名 |

## 二、首页（Codex，禁改文件清单）
- `ui/design/home/`（HeroPage/HeroPullZoom/HomeAtmosphere/HomePreferences/NavGlyph/NavigationBackdrop）
- `tmdb/trailer/`、`ui/design/{HeroCarousel,DiscoverPage,CapsuleNavBar,DesignComponents,TmdbImage,PosterPalette,Pages,WatchingPage}.kt`
- `MainActivity.kt` 的 Shell/导航/首页逻辑
- 机制与证据：`docs/evidence/p237/home-takeover/status.md`、`docs/handoff/prompt-claude-hero-ambient.md`
- 唯一例外：`MainActivity.kt` 的 `onOpenTmdbItem` 一行已按 240 §2 计划接线到详情页
  （原 HomeTitleSheet 保留未删，现已不被调用）。

## 三、P3 实现状态

### 已完成（编译通过）
1. 设计稿 `docs/design/backend-core/240-detail-page.md`（E1-E14 帧实测锚点、层序、数据域、家族树、验收）。
2. `tmdb/TmdbDetail.kt`：`/movie|tv/{id}?language=zh-CN&append_to_response=videos,credits,similar,images`
   一次拉全；解析 meta（类型/日期/地区/集数）、官方预告（YouTube 优先级 Trailer>Teaser）、
   cast 前 10（name/character/profile w185）、similar 前 10（有海报者）、backdrop 回退海报。
   已知坑已修：get() 拼 URL 时补 api_key；IMG 引用改为 TmdbHome.IMG；json 判空在解构后。
3. `ui/detail/MovieDetailActivity.kt`：585dp 幕布原色 + 环境色遮罩（α 0@0.55→0.95@0.65→0.97@1.0，
   颜色 = mix(底缘色, pageBackground, S/390) 与首页同构）+ 下缘场 [585,1100]（同源，底部 29% 渐隐）
   + meta/简介/徽章（复用首页锚点常量 HERO_HEIGHT_DP/META_TOP_DP/BADGE_TOP_DP，**需显式 import**）
   + 主操作行（观看胶囊 G9 toast + +/♥ 视觉态 + ⋯）+ 演职员/分类/预告/相似各节 + 返回圆钮。
4. `ui/detail/DetailSections.kt`：SectionHeader/CastRow/GenreChips/TrailerCard/SimilarList。
5. manifest 注册 `.ui.detail.MovieDetailActivity`。

### 未完成（下会话按序收口）
1. **真机验证重做**：唯一一次疑似详情页截图（p3_mid）出现了实现中不存在的元素
   （「继续播放 1:24:17」「播放资源 4K」「HDR 10 卡」）——捕获来源存疑（疑串到其它应用/旧构建）。
   必须：force-stop → 确认前台包名 → 按 §四坐标导航 → 逐屏捕获 → 与 movie_05/06/09/10 并排逐行 diff。
2. 首页点击→详情的接线**行为未确认**（tap 曾打到错误 tab）。
3. 各节与 Rex 帧的逐项对比未做（240 §4.1）。
4. +收藏/♥待看 = 视觉态（无持久化）——v2 接 WatchStore。
5. 观看钮 = G9 toast（TMDB 项无本地资源匹配；本地媒体信息卡 v1 未实现——240 允许整卡隐藏）。
6. 剧照行未实现（数据域已含 images 响应，只差 backdrops 解析 + 横滑组件）。
7. 演职员角色字段：TMDB credits 的 character 与参照帧的「导演/主演」标签口径待对齐。
8. 注释中性化：P3 新文件注释为中性文案 ✓；若补注帧来源一律写 `docs/evidence/p240/`。

## 四、新导航坐标（Codex 改版后实测，nav_0_bottom.png）
底部胶囊：**发现 ≈ x150 / 我的 ≈ x350 / 资源库 ≈ x555 / 设置 ≈ x760**（图标+标签中心 y≈2280），
独立搜索圆钮 ≈ x960。注意与 v14 前的坐标完全不同。
- 进发现：`input tap 150 2280`
- 进详情：发现 tab → 短滑到正在热映（`input swipe 540 1600 540 1100 400`）→ 点海报卡
  （第一张 x≈70-290，y≈1230；后续卡 x≈205/340/…）
- 页内滚动用**短滑**（540 1500 → 540 700）：长滑会触发系统手势/最近任务。

## 五、构建/装机/测量命令
```bash
cd "C:/kivo video/android" && JAVA_HOME="C:/Android/jdk-17.0.13+11" ./gradlew.bat :app:assembleDebug
ADB="C:/Android/sdk/platform-tools/adb.exe"; export MSYS_NO_PATHCONV=1
"$ADB" -s b6e950ee install -r -t app/build/outputs/apk/debug/app-debug.apk
"$ADB" -s b6e950ee reverse tcp:10808 tcp:10808        # TMDB 代理（key 在设备 filesDir/tmdb_key.txt）
"$ADB" -s b6e950ee shell "am force-stop com.kivo.cinema; am start -n com.kivo.cinema/.MainActivity"
sleep 4; "$ADB" -s b6e950ee shell "input tap 150 2280"; sleep 10   # 发现 tab，等 TMDB 数据
```
- 测量：Python/PIL 同列（x=180pt/180dp）逐行 diff，参照 `docs/handoff/prompt-claude-hero-ambient.md` §6 阈值。
- 对照帧：`Pictures/REX/frames/movie_05/06/09/10.png`（详情）、`movie_08.png`（详情滚动态全节）。

## 六、P3 收口后的队列（237 stage C 顺序）
1. P2 媒体库重铸（lib_01/lib_02 帧已测）
2. P6 设置全集（set_a/set_b 已测）
3. P9 SMB/Emby 表单重铸
4. tab 图标 vector 化（237 §5.5；当前 emoji——Codex 新导航仍是 emoji/符号）
5. P4 搜索页（G3 素材缺，待用户录屏）

## 七、铁律提醒
- 禁改 §二清单（Codex 首页）。
- 源码零参照词；帧来源写 `docs/evidence/p240/` 与设计稿。
- 单文件单职责；家族树见 240 §2。
- 铁律门闸顺序：构思→规划设计→家族树→实现（P3 已过前三门，剩实现+验收）。

---

## 八、二轮修正（2026-11-08 深夜，接手会话）

### 8.1 结论先行

首轮"不像 REX"的三个根因已定位并修正，**几何锚点全部 ≤0.6dp**：

| 根因 | 现象 | 修正 |
| --- | --- | --- |
| 幕布截在 507dp，文字落在下方平色 | 海报与页面"割裂" | 幕布 **589.5dp（2:3 海报满宽）**，meta/按钮**叠在海报下缘** |
| 照搬首页遮罩曲线（起点 47%） | 海报标题被洗白 | 遮罩起点下移 72%，只压末段 |
| 环境色取图**底缘**（亮雪地） | 页面底色奶白、白字不可读 | 色场改回首页机制，底色 = 环境色 |

### 8.2 新增口径（已写入 `240-detail-page.md` §6）

- **REX 设计基准 = 393pt 宽**（拆解包 §3.2 启动屏 nib 解码 `0,0,393,852`），本机 392.7dp 为 1:1；
  430pt 那批截图需 ×0.914 折算。
- 参照两批：`Pictures/REX/frames/movie_05..10`（390pt，旧）+ `docs/evidence/p240/refs/`（430pt，新，权威）。
- 拆解包位置：`C:/Users/Administrator/Pictures/REX/rex/rex/Rex_拆解成果_落地包/`
  （`Rex_UI结构复刻说明书.md` / `Rex_动效与交互全量拆解.md` / `Rex_动效参数表.csv` / 设计 token bundle）。
- REX 语义色板（真值）：`pageBackground` #F2F2F7 / #000000，`canvasFill` #FFFFFF / #222222，
  `groupedFill` #F2F2F7 / #1C1C1E，`hairline` 黑 5% / 白 10%，`washFill` #EDEDFB@50%（P3 色域）/ #3D3D41。

### 8.3 本轮实现内容

`TmdbDetail.kt`：地区中文（最多三国）、片长、导演位、剧照、片源 logo、相似作品简介。
`MovieDetailActivity.kt`：幕布 589.5 + 文字叠加、色场随滚动、状态栏随色场翻色、空态块接线。
`DetailSections.kt`：节标题 18sp、演职员（头像 55/间距 18/名 13sp/角色 11sp）、
分类 chips 15sp、预告 16:9、剧照横滑、相似作品单列带三行简介、空态块、片源 logo。

### 8.4 仍未做（下一步按序）

1. **家长指南**：TMDB 无数据 → 需另建数据源，否则整节隐藏（当前已隐藏）。
2. **多源徽章**：需接入猫眼/时光/豆瓣/番茄/爆米花 分值源；当前仅 TMDB。
3. **播放资源 / 媒体信息卡**：需本地媒体库按路径匹配（WatchStore）；当前走空态块。
4. **问题反馈浮动钮**：留待 P6 设置页统一入口。
5. 空态 TV 图标为简化绘制，与参照插画精度有差。

### 8.5 复算与工具

`tools/measure_frames.py`（锚点/行剖面）、`tools/compare_frames.py`（同尺度并排+差异热图）、
`tools/ruler.py`（dp 标尺叠图）。核对记录：`docs/evidence/p240/status.md`。

### 8.6 三轮补充（同日）

- **相似作品改横向卡片流**（此前误判为单列竖排，是"没逐帧看图"造成的方向性错误）：
  16:9 海报 151×85 + 右文，卡宽 302、间距 23、右缘探出下一张。
- **按钮改玻璃材质**：主钮 白 α0.28、副钮/圆钮 白 α0.147、发丝描边 白 α0.10；
  图标改为「收藏夹+」「心形描边」「三点」自绘，不再用 `＋`/`♡` 文本字形。
- **家长指南落地**（用户裁定用 TMDB 真实分级）：`release_dates` → 分级卡（美/英，取 2）+ 内容提示卡
  （描述符中文化，取 2）。**不显示严重度与票数**——TMDB 无此数据，造假即违反 G9。
- 返回钮：top 37→42dp、白 α 62%→55%（对齐参照的更低更透）。
