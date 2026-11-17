# Kivo Video 全项目审查报告

时间：2026-11-18 01:10 GMT+8
范围：分支治理 + 代码可维护性 + 视觉/UX/交互
方法：只读审查。分支数据来源 `git for-each-ref` / `rev-list` / `merge-base` 实跑；代码与界面结论均来自实际读文件、看真机截图，证据写到「文件:行号」或截图路径。

---

## 0. 结论先行

1. **分支多不是偶然，是流程产物。** 执行卡明确要求「每张卡开一条分支（Branch/Base/Clean Gate）」。P0/P2/P3/P4/P5A/P7/P8 每阶段一条，修复再开 `-repair-001`、收口再开 `-final-closure-001/002/003`、救火再开 `rescue/*`。37 条本地+远程引用就是这么堆出来的。
2. **真正危险的不是分支数量，是三件事：**
   - ~~4 条游离分支含 17,591 行独有工作~~ → **经逐文件复核已推翻，见 §1.3：99% 早已在主线，真正缺的只有 5 文件 / 529 行**。
   - 工作区有 **71 个已改文件 + 207 个未跟踪文件**未入库（含整套 `ui/components` 组件库、`ui/design/home/sections`、`MediaTechProbe`、`TmdbGenres/People/Providers/Studios`）。
   - 本地有 **2 个提交未推送**（`8bd768e2`、`82e98e91`，即 338/339），与 `AGENTS.md:70`「每张卡收口都要推」的约定相悖。
3. **设计层面的系统性缺陷**：新旧两套令牌/组件并存且调用点未迁移完（`DesignTokens.kt:71-76` 明写「接连接口由 B-2 起改调用点」），两套玻璃实现（`ui/components/GlassSurface.kt` 与 `ui/compose/player/PlayerGlassSurface.kt`）规格不同且前者疑似无调用点。
4. **文档严重过期**：`docs/roadmap/current_state.md` 停在 2026-06-27，`AGENTS.md` 关键状态停在 2026-10-31。每次开工先读这些会误判「什么已做」，这是重复劳动的直接原因。

---

## 1. 分支地图

### 1.1 全量分支（按最后提交时间）

| 分支 | 最后提交 | 日期 | 与当前主线关系 |
| --- | --- | --- | --- |
| `codex/kivo-video-p0-ui-01-playback-overlay-v13` | `82e98e91` | 11-16 | **当前工作分支**（本地领先远程 2 提交） |
| `codex/p7-p8-playback-subtitle-plane` | `da982b74` | 06-29 | 已包含 |
| `kivo-video-p5a-contract-layer-001` | `48504f7f` | 06-29 | 已包含（但内容是 p6de，非 p5a） |
| `kivo-video-p3-preflight-gate-repair-001` | `2f6694e7` | 06-28 | 游离，但独有内容 99% 已在主线（见 §1.3） |
| `kivo-video-p2-remote-range-runtime-http-webdav-001` | `2506fdd5` | 06-28 | 已包含（但最新提交是 p5a） |
| `kivo-video-p2-source-runtime-final-closure-001/002/003` | `5794cd10`/`a82fa51e`/`9ac330a4` | 06-27 | **游离，仅文档**，三条为同一件事反复修 |
| `kivo-video-p2-local-file-source-runtime-001(-repair-001)` | `e9ac923b`/`c7f82900` | 06-27 | 已包含 |
| `kivo-video-p2-real-001-local-ffmpeg-slice` | `cc3765ec` | 06-25 | 已包含 |
| `kivo-video-p2-real-001-repair-ffmpeg-proof` | `532ad9b7` | 06-26 | 已包含 |
| `kivo-video-p2-source-core-skeleton-001(-repair-001)` | `9a579246`/`5696edf9` | 06-26 | 已包含 |
| `master` / `origin/main` | `44924330`/`7718d67b` | 06-25/06-24 | 已包含（主线比 master 多 397 提交，master 独有 0） |
| `heads/rescue/p2-001..003`、`heads/rescue/p3-001` | 同 closure 分支 | 06-27/28 | **纯别名**，与 closure 分支指向同一提交 |

### 1.2 为什么会这么多（因果链，全部有实证）

1. **执行卡制度强制开分支**：`docs/roadmap/current_state.md:61` 写「Execute `SRC-CORE-SKELETON-001` starting with `SCS-001 Branch/Base/Clean Gate`」。每张卡第一步就是开分支。
2. **一张卡拆多个后续卡**：P2 又分 source-core / local-file / remote-range / final-closure，每个都建分支。
3. **同一分支被跨阶段复用 → 名实不符**：
   - `kivo-video-p2-remote-range-runtime-http-webdav-001` 最近提交是 `feat(p5a): add video plane contract layer`（`2506fdd5`），分支名还停在 p2-remote-range。
   - `kivo-video-p5a-contract-layer-001` 最近提交是 `fix(p6de)`（`48504f7f`）。
   - 结论：**不能靠分支名判断里面有什么**。
4. **收口文档反复修，每修一次开一条新分支**：`final-closure-002` 同一天连续 6 个提交 `total_commits 4→5→6→7→8→10`，并同时存在 001/002/003 三条分支。
5. **`rescue/*` 是同提交别名**：`rescue/p2-001` = `final-closure-001`（`5794cd10`），`rescue/p3-001` = `p3-preflight-gate-repair-001`（`2f6694e7`）。同一份工作被两个名字引用，看起来像两套。
6. **状态文档不更新**：干完的卡没从 `current_state.md` 的 Next Work 里划掉，下一个人照着旧清单又开一条分支。
7. **大量工作根本没进版本控制**：71 改 + 207 未跟踪，包括整套 UI 组件库。人只能靠肉眼看目录判断有没有，于是重复写。

### 1.3 【重要修正】p3-preflight 分支的 17,591 行到底是什么

初判说它「含 1.75 万行独有工作」是**只看 `git diff --stat` 行数导致的误判**。逐文件复核后结论完全相反：

**什么时候做的**：2026-06-27 18:36 → 06-28 02:15，一个晚上 7.5 小时，20 个提交。分叉点是 06-27 05:31 的 `c90cfdc9`。做的是 P3「playback-ready 输入系统桥接」+ P4「播放控制面合同层」，全程 194 个测试 PASS。

**里面是什么**（按扩展名）：

| 类型 | 行数 | 文件数 | 内容 |
| --- | --- | --- | --- |
| `.hpp` | 5,873 | 121 | P3/P4 C++ 合同头文件（控制面、reader/memory/buffer、恢复/失败/DRM、输出边界等） |
| `.md` | 5,428 | 12 | 执行卡与收口报告（P4 执行卡单文件就 2,409 行） |
| `.cpp` | 4,580 | 16 | 对应测试 |
| `.json` | 1,284 | 28 | 证据包 |

**为什么行数看着多却没有独有工作**：`git diff HEAD...branch` 比的是「分叉点之后的全部改动」，但主线后来又独立提交了一批**内容完全相同**的文件。逐文件 md5 比对结果：

- 137 个 C++ 文件中 **132 个内容完全一致**（diff 只差 CRLF/LF 换行符）——P3/P4 合同层早已进主线。
- 只有 **5 个文件主线确实没有**，共 **529 行**：
  - `backend/include/kivo/playback/control_plane/runtime_fake/` 下 4 个 fake runtime：`controlled_clock.hpp`(45)、`fake_buffer_engine.hpp`(50)、`fake_event_bus.hpp`(40)、`fake_reader.hpp`(115)
  - `backend/tests/playback/control_plane/p4_soak_perf_stress_tests.cpp`(279)
  - 全部来自最后一个提交 `26e7f1d4`（06-28 02:14），是 P4 的 fake runtime + 浸泡/性能/压力测试。

**结论**：不存在「重做 1.75 万行」的风险。真正待决的只有这 529 行 fake runtime/压测代码——主线有等价的 P3 fake（`backend/include/kivo/p3/providers/fake/fake_providers.hpp` 等）和 P4 测试目录，但**没有同名的 soak/perf/stress 测试**。若认可「P4 压测不需要」，可直接打 tag 归档删分支；若要保留，cherry-pick `26e7f1d4` 即可。

> 教训：`git diff --stat` 的插入行数会把「分叉后重做但已合并的相同文件」算成独有工作，判断重复劳动必须逐文件比对（md5 或 `diff`），不能只看行数。

### 1.4 防重复的落地方案（按优先级）

| 优先级 | 动作 | 具体做法 |
| --- | --- | --- |
| P1 | **归档 P3/P4 游离分支** | 逐文件复核后确认只缺 5 文件（fake runtime 4 个 + 1 个测试，共 529 行）。要么把这 5 个 cherry-pick 回主线，要么确认不需要后打 tag `archived/p4-contract-2026-06` 删分支 |
| P0 | **清理 71+207 工作区改动** | 逐个确认：要保留的提交，临时产物删除或进 `.gitignore`；尤其 `ui/components/*`、`ui/design/home/sections/*` 要确认是「正在用的新组件」还是「被替代的旧组件」 |
| P0 | **推送本地 2 个提交** | `git push`（当前 fast-forward：`origin` 落后 2） |
| P1 | **建立单一事实来源** | 用 `docs/handoff/CURRENT.md`（或新建 `BRANCH_MAP.md`）维护一张表：分支 → 包含阶段 → 是否已并入主线 → 是否可删。每收口一张卡更新一次 |
| P1 | **停止按卡开长期分支** | 改为「一个阶段一条长期分支 + 卡用提交 message 标记」；确实要隔离的工作用 `worktree` 并当天合并或删除 |
| P2 | **分支命名带阶段而非任务号** | 例如 `stage/p4-control-plane`，避免分支名在 3 个阶段后失效 |
| P2 | **删别名与已并入分支** | `rescue/*` 4 条（纯别名）、9 条 `INCLUDED` 分支可删（删前打 tag 保底） |

> 注：以上是建议，本次审查**未删除任何分支、未提交任何改动**。

---

## 2. 工作区与备份状态（审查时实测）

| 项 | 数值 | 风险 |
| --- | --- | --- |
| 已跟踪文件改动 | 71 个文件，+1232 / -665 行 | 未提交 = 机器故障即丢失 |
| 未跟踪文件 | 207 个 | 不在 git 里，`git clone` 拿不到 |
| 本地领先远程 | 2 提交（`8bd768e2`、`82e98e91`） | 339/338 的修复只在本地 |
| 主线 vs master | 主线多 397 提交，master 独有 0 | master 可视为历史基线，别从 master 开工 |

未跟踪文件分类（207 个，含本报告自身 1 个）：

| 目录 | 数量 | 性质 |
| --- | --- | --- |
| `docs/audit/` | 97 | b0–b4 / calendar 验证文档 + 大量 golden 对比图 |
| `android/app/` | 78 | **78 个 Kotlin 源码**，含整套 `ui/components/`、`ui/design/home/sections/`、`MediaTechProbe`、`TmdbGenres/People/Providers/Studios` |
| `docs/evidence/` | 10 | 首页修复前后真机截图（home-fix） |
| `docs/design/` | 10 | 设计稿/逆向文档 |
| `backend/` + `third_party/` | 8 | C++ 合同与 llama.cpp 适配 |
| 其他 | 4 | `tools/ui_gate.py`、`.arts/settings.json` 等 |

> 注意：78 个 Kotlin 文件未入库意味着 `git clone` 出来的工程**编译不出来**（引用了不存在的类）。这是最高优先级的备份风险。**逐文件分类结论见 §2.1。**

### 2.1 149 个未入库文件的入库判定（逐文件核查，未擅自 add）

**判定方法**：① 逐文件查 import 链，看是否被已跟踪代码引用；② 检查是否有空文件/半成品；③ 实跑 `./gradlew :app:compileDebugKotlin` 验证这 3908 行能否编译。

**编译结论：BUILD SUCCESSFUL（2m18s）** —— 证明这是完整可编译工作，不是草稿。

**A. 78 个未跟踪 Kotlin（3908 行）——全部应入库，一个都不能丢**

| 分组 | 文件数 | 证据 |
| --- | --- | --- |
| `ui/profile/**` "我的"页面整套 | 39 | `MainActivityShell.kt:50-53` import 了 `ProfilePage`/`ProfileStats`/`ProfileDashboard`，缺则编译失败 |
| `ui/design/home/sections/**` 首页分区 | 15 | `DiscoverPage.kt:42-54` 直接 import 了 13 个（ContinueRow/GenreCards/PosterGrid/WideSection…） |
| `ui/components/**` 通用组件 | 6 | `WatchingPage.kt:32` import `HitBox`；视觉审查也依赖 `EmptyState`/`ErrorState` |
| `ui/tokens`/`ui/motion`/`ui/shell` 基础设施 | 6 | `DesignTokens.kt:3` import `DesignPalette`；`HeroCarousel.kt:23`、`PlayerBarsHost.kt:26` import `MotionGate`；`SourceWorkspace.kt:17` import `ShellInsets` |
| `library/tech/**` 技术信息 | 3 | `DetailActivity.kt:40-41` import `MediaTechInfo`/`MediaTechProbe` |
| `tmdb/**`、`ui/detail/**`、其他 | 9 | 被 20 个已跟踪文件 import 的 tmdb 包的一部分；详情卡片为完整实现 |

完整性检查：无空文件，最小 13 行（`ShellInsets.kt`）、14 行（`ProfilePageGradient.kt`），都是正常的小工具类；无 `.tmp`/`.bak`/`build/` 产物混入。

**B. 71 个已改文件（+1232/−665）——是一套连贯重构，应整体入库**

不是散乱试验：`DiscoverPage.kt`（118 增 247 删）把首页拆到 `home/sections/**`；`MainActivityShell.kt`（+198）接入"我的"页；播放器 6 个面板文件（`PlayerBottomBar`/`PlayerOverlayPanel`/`PlayerPanelHost`/`PlayerTopBar` 等）同步改；`SettingsPage.kt`（+172）、`DesignTokens.kt`（±58）配套。A 和 B 是**同一次重构的的两半**，只提交一半会编译失败。

**C. 未跟踪的非 Kotlin 文件——分类处理**

| 类别 | 数量 | 判定 |
| --- | --- | --- |
| `backend/` 字幕翻译 C++ 合同（4 hpp + 2 cpp + 2 test） | 8 | **应入库**：有对应测试，属完整 P8 字幕翻译能力 |
| `third_party/llama.cpp-android/`（README + build.sh） | 2 | **应入库**：本地 AI 依赖的构建脚本 |
| `tools/ui_gate.py` | 1 | **应入库**：UI 门禁工具 |
| `docs/audit/**`（97） | 97 | **应入库**：b0–b4/calendar 验证 + golden 对比图，是审查证据 |
| `docs/evidence/**`（10 首页修复截图）、`docs/design/**`（10） | 20 | **应入库**：真机证据与设计稿 |
| `docs/review/`（本报告自身） | 1 | 待你确认后一并入库 |
| `.arts/settings.json` | 1 | **不入库**：`{"clawMode.mode":"claw"}` 是本机工具状态，建议加进 `.gitignore` |

**D. 推荐入库命令（按顺序，可复查）**

```bash
cd "C:/kivo video"
# 1. 代码（编译相关）—— 一次提交
git add android/app/src/main/java android/app/src/main/AndroidManifest.xml \
        android/app/src/main/cpp/CMakeLists.txt backend tools/ui_gate.py \
        third_party/llama.cpp-android
git commit -m "402: 入库首页/我的页重构与播放器面板改造（78 新 Kotlin + 32 改）"

# 2. 文档与证据 —— 另一次提交
git add docs/audit docs/evidence docs/design docs/review
git commit -m "402: 补入库验证文档、golden 对比图与审查报告"

# 3. 本机工具状态 —— 不入库，加忽略
echo ".arts/" >> .gitignore

# 4. 推送
git push
```

> 说明：以上只是**建议命令，本次未执行**。`git add` 目录时 Git 会自动跳过 `.gitignore` 内的文件；执行前建议先 `git status --short | wc -l` 确认清单。

---

## 3. 视觉 / UX / 交互审查

取证：实看 12 张真机截图（p237 发现/在看/导航胶囊、p240 详情/弹层、p236 设置/资源库、p225 Emby、p213 播放器、p10 横屏面板），逐条对到代码行号。视觉定稿实际在 `style-unification.md`、`ui/replica/nav-capsule-v3.md`、`apple-tv-principles.md`、`274-design-system.md`（27x 最新三份是 HDR/性能，不含视觉规格）。

### 3.1 P0

**P0-1 详情页 hero 文字对比度塌陷（白字压浅底）**
- 证据：`ui/detail/DetailPage.kt:120-123`（`fg` 由 `sheetColor.value.luminance()` 决定）；`DetailBackdrop.kt:53-64`（实际背景是 `field.value` 渐变叠海报，startY=250dp→endY=640dp）；截图 `docs/evidence/p240/03_detail_top.png`。
- 现象：类型/日期/剧情简介是近白色，压在浅米色渐变上几乎读不清；同一类文字压深色海报时可读。根因是**前景色取自 `sheetColor`，文字却渲染在另一套 `field.value` 渐变上，两者不同源**。
- 影响：详情首屏关键信息不可读，对比度远低于 4.5:1。
- 改法：让 `fg` 基于文字实际合成色判断（把 `field.value` + 海报亮度纳入 `chromeDark`），或统一给 hero 文字加暗色 scrim / 1dp 描边；`DetailBackdrop.kt:56-63` 渐变起点上移到文字带下方并加大暗段。

**P0-2 本地影片页仍是 3 列 8dp 小格 + emoji 占位 + 10sp 字 + 文件名**
- 证据：`library/MediaLibraryActivity.kt`（`GridCells.Fixed(3)`、`RoundedCornerShape(8.dp)`、`Text("🎬", fontSize=32.sp)`、标题 `10.sp`）；`docs/design/style-unification.md:38` SU-01 已标 **P0「3 列小格、8dp 圆角、裸灰占位、标题带扩展名、主线程取图」**，状态仍"🔄 重构中"。
- 现象：`p237-home.png`「本地影片」标题截成 `1gb_720p....`/`1080p30_...` 且带 `.mp4`，缩略图为彩条测试图。
- 改法：`GridCells.Adaptive(128.dp)` 或 2 列、`16.dp` 圆角、去扩展名、异步缩略图、标题 13sp；与 `EmbyPosterCard.kt:31` 的 12dp 统一到同一 token。

**P0-3（审查者第一手补充）Emby 切服后 hero 中英文标题重叠、英文溢出屏幕**
- 证据：截图 `docs/evidence/p338-server-switch/switched_server.png`（iLoveHD 服「财神窰占」）。
- 现象：中文标题「财神窰占」与英文注音「Long.Cai.Shen.Dou.Zhan」**叠在同一行**、英文左右溢出屏幕边界；上方还有孤立的「封面加载失败，点击重试」提示占据大片空白 hero；评分位显示绿色方块「Emby 0.0」，与首页 TMDB 官方绿标的视觉语言不统一。
- 改法：hero 标题区改 `Column` + 主标题/副标题分行，`maxLines=1` + `TextOverflow.Ellipsis`；封面失败时在 hero 区内显示小型重试 chip，不要留出整屏空场；无 TMDB 评分时**隐藏评分位**而不是显示 `0.0`（此前 P5 已定"空分类隐藏"，评分同理）。

### 3.2 P1

**P1-4 返回键至少 4 种样式，未统一**
- 证据：详情页 `DetailHeroContent.kt:87-106`（42dp 半透明白圆 + 黑 chevron）；Emby `EmbyBrowseTopBar.kt:61-64,119-135`（44dp `#E7EDFB` 实色 + `#CFD4E0` 描边）；日历 `CalendarPageChrome.kt`（44dp 白 60%）；播放器 `PlayerTopBar.kt:78-93`（裸 ✕ + 主页图标，无玻璃底）；Emby 目录页 `p225-browse.png` 是灰色文字胶囊「上一级」。
- 改法：新建 `ui/components/KivoBackPill`，按定稿单一实现，替换上述 5 处；播放器 ✕ 属"关闭"语义可保留但要与返回键区分说明。

**P1-5 触摸目标系统性 <48dp**
- 证据：`DetailHeroContent.kt:90` 42dp；`ui/components/HitBox.kt:27` 下限 44dp；`PlayerTokens.kt:108,258,149,218,223,226`（44f、`trackHitHeight=24f`、close 38f、stepper 30f、chip 30f）；`DetailCalendarGrid.kt` 40dp；`274-design-system.md:388-403` 已列 7 类违规并规定硬下限 48dp。
- 改法：`HitBox` 下限改 48dp 并全量套用；`trackHitHeight` 视觉轨保持 6dp、用透明 Box 撑 48dp 命中区；`DetailHeroContent` 42→48；`WatchingPage.kt:67-75`「去发现」≈32dp → 48dp。

**P1-6 玻璃风弹层不统一（实色扁纸 vs 假玻璃 vs 白卡）**
- 证据：`ui/components/GlassSurface.kt:22-41`（半透明白+描边，自注无真模糊）；`PlayerGlassSurface.kt:19-68` + `PlayerTokens.kt:50`（黑 75% 假玻璃）；`DetailActionSheet.kt:55-59`（30dp 圆角 `#F5F4F2` 实色扁平）；`EmbyBrowseTopBar.kt:200-206`（纯白 16dp + 阴影）；`style-unification.md` 与 `detail-popups-flat.md` 口径互相矛盾。
- 改法：定一份 `MaterialSpec`（玻璃或扁平二选一写死），全部弹层收口到同一组件/参数，同步修正 3 份互相矛盾的设计文档。

**P1-7 设计 token 多套方言，doc 与 code 不一致**
- 证据：`DesignTokens.kt:41-43`（`radiusCard=13`/`radiusPoster=12`/`elevation=1`）；`style-unification.md:11-12`（16/10、卡间距 12）；`274-design-system.md:320-328`（20/12）；`EmbyPosterCard.kt:31`（12dp）、`GenreCard.kt`/`MediaLibraryActivity`/`PrivateActivity`（8dp）、`PlayerTokens.kt:194`（16dp）、设置页 20dp。字阶 `DesignTokens.kt:62-76` 7+ 种，274 审计全仓 20 种。
- 补充：`DesignTokens.kt:71-76` 明写「REX 实测新字号……接连接口由 B-2 起改调用点」——**新旧两套 token 共存且迁移未完成**，这是重复劳动的直接来源。
- 改法：建 `design/tokens/` 唯一真相源，先让 doc 与 code 数值一致，再逐页替换调用点。

**P1-8 播放页次要文字对比度 token 未落地**
- 证据：`PlayerTokens.kt:20` `textSecondary=0xFF8A8A8E`；`274-design-system.md:420` A6 已明确"on 75% 黑 ≈3.1:1 不达标 ⇒ 提到 #B4B4B8"，代码未改。
- 改法：改 `0xFFB4B4B8`，全仓 `#8A8A8E` 收敛到 `DesignTokens.textSub`。

**P1-9 导航选中态与定稿文档冲突**
- 证据：`CapsuleNavBar.kt:21-35,57` 当前是"白 60% pill + 近黑前景，不是蓝色 tint"；`nav-capsule-v3.md:38` 定稿是"纯白 pill + `#0A84FF` 蓝图标/文字"；历史截图是蓝色，当前代码近黑。
- 改法：产品裁决写进 `style-unification.md`，按裁决改代码或改文档；补 2dp 顶部指示条做形状冗余（274 §267）。

### 3.3 P2

- **P2-10 测试内容/文件名污染主界面**：`p237-home.png` 继续观看/本地影片全是 SMPTE 彩条 + `1080p30_60s.mp4`；错误被丢到 LazyColumn 最后一项、"重试"仅 26dp 高。改：统一 `EmptyState`（已有 `ui/components/EmptyState.kt`）+ 骨架，无海报时渐变卡+标题，错误就近显示且按钮 ≥48dp。
- **P2-11 详情页 CTA 滚走即消失 + 固定坐标溢出**：`DetailPage.kt:173-235` CTA 在 LazyColumn item 内；`DetailHeroContent.kt:110` `DETAIL_TITLE_TOP_DP=404.5f`、`DetailBackdrop.kt:40` `height(585.dp)` 写死。改：CTA 提为 `Scaffold bottomBar`，尺寸改按宽度/内容推导 + `heightIn(min=)`。
- **P2-12 TalkBack 语义缺口**：`DetailCalendarGrid.kt:84-124` 整月 Canvas 零 Text 节点；`PlayerTopBar.kt:204-210` `pointerInput` 无 `Role.Button`。改：补 `KivoSemantics`、role + onClick + stateDescription。
- **P2-13 Emby 海报卡无取色背景**：`EmbyPosterCard.kt:31-36` 占位是 `primary.copy(.06f)`，详情 `DetailPage.kt:95-101` 用 `PosterPalette.ambientOf()`。改：抽 `rememberPosterAmbient(bitmap)` 复用。
- **P2-14 设置页残留开发术语**：`p236-tab-settings.png` 有「解码：硬解优先 · 失败报 reason」「本卡为设置页骨架…(236 §六)」。改：全局搜 `reason`/`236 卡`/`§`/`骨架` 改自然语言（该图为历史骨架，需真机复核当前版）。

### 3.4 最该先修 5 件事

1. **详情页 hero 文字对比度**（P0-1）：让前景色与文字实际背景同源。
2. **本地影片页按 SU-01 收口**（P0-2）：项目自标 P0 至今未做。
3. **Emby hero 中英文标题重叠/溢出**（P0-3）：一行改两行 + 省略 + 失败态收窄 + 隐藏 0.0。
4. **统一返回键 + 48dp 命中**（P1-4/5）：新建 `KivoBackPill` 替换 5 处。
5. **冻结唯一 token 源 + 统一弹层材质**（P1-6/7）：消除文档互相矛盾再逐页收口。

> 验收口径提醒：若以「Apple Music 玻璃胶囊 + 玻璃弹层 + Emby/TMDB 官方图标 + 海报取色」为标准，当前只有 Emby 返回键、官方 Emby logo、详情页取色三处达标，其余均为偏离点。

---

## 4. 代码可维护性审查

> 取证说明：本节硬数据为审查者独立重跑（`wc -l` 遍历全部 Kotlin + `git` + 逐文件 Read），不依赖代理转述。代理报告的后半段正文未送达，故凡未能独立复核的条目一律不写入。

### 4.1 行数铁律：21 个 Kotlin 文件超 300 行

铁律要求单 Kotlin 文件 ≤300 行（`CMakeLists.txt:533` 有 B5 守卫注释，**但它只检查 `.cpp`，不检查 `.kt`**，所以 Kotlin 侧实际无人拦截）。

| 行数 | 文件 |
| --- | --- |
| **901** | `ui/PlayerActivity.kt` |
| 454 | `ui/detail/EmbyResource.kt` |
| 427 | `mpv/LocalRangeResponder.kt` |
| 406 | `ui/compose/PlayerActionRouter.kt` |
| 403 | `bridge/FfmpegPlaybackSession.kt` |
| 402 | `net/OriginPrefetchKit.kt` |
| 387 | `playback/NetworkPlaybackSession.kt` |
| 382 | `ui/compose/PlayerComposeActivity.kt` |
| 381 | `playback/PlaybackPrefs.kt` |
| 380 | `playback/MpvPlaybackSession.kt` |
| 379 | `net/HttpOriginSource.kt` |
| 365 | `bridge/PlaybackSession.kt` |
| 359 | `emby/EmbyClient.kt` |
| 350 | `net/OriginConnectionLease.kt` |
| 343 | `thumbnail/MpvThumbnailReader.kt` |
| 341 | `net/HttpOriginClient.kt` |
| 338 | `playback/MpvStartSequence.kt` |
| 334 | `library/MediaLibraryActivity.kt` |
| 320 | `bridge/NativePlayer.kt` |
| 312 | `playback/PlaybackMetrics.kt` |
| 307 | `net/OriginWindowCache.kt` |

临界 300 整（下一步就会破线）：`ui/profile/ProfileStats.kt`、`ui/compose/PlayerProbeHooks.kt`、`ui/design/MainActivityShell.kt`，以及 299 的 `emby/EmbySessionModel.kt`（338/339 刚改过，已顶到线）。

**最该拆的 3 个**：`PlayerActivity.kt`（901，是它的 3 倍）；`EmbyResource.kt`（454）；`NetworkPlaybackSession.kt` / `MpvPlaybackSession.kt` 与 `FfmpegPlaybackSession.kt`（三套播放会话各 380–403，职责高度重叠）。

### 4.2 重复实现 / 死代码

**M-1（P0）两套播放器 Activity 并存且都注册在 Manifest**
- 证据：`AndroidManifest.xml:48` `PlayerComposeActivity`、`:54` `PlayerActivity`、`:75` `MultiPlayerActivity`。`PlayerActivity.kt:901` 是 View 体系旧宿主（`PlayerSurfaceView.Listener` + `TextView statusView` 现场投影），`PlayerComposeActivity` 是 Compose 新宿主。
- 现象：同一能力两套入口，`PlayerComposeRoot` 里还有 `maybeStartPlayback` 绕过 Activity 生命周期的现象（项目长期笔记已记）。
- 改法：确认 Compose 版为唯一入口后，删 `PlayerActivity` + `PlayerSurfaceView` + `statusView` 投影链，Manifest 只留一个；`MultiPlayerActivity` 若仅用于能力探针，移出主包到 `tools/` 或保留但标注 `@Deprecated`。

**M-2（P0）两套详情页 + 两套同名 `GlassSurface`**
- 证据：`ui/detail/DetailActivity.kt:56`（241 行 companion，Manifest 里 `exported=true`）与 `ui/detail/MovieDetailActivity.kt:20-122`（实际调 `DetailPage`）并存；`ui/design/DesignComponents.kt:90` 定义 `GlassSurface(radius: Float, colorOverride)`，`ui/components/GlassSurface.kt:23` 定义 `GlassSurface(radius: Dp, color: Color)` —— **两个包里各有一个同名 Composable，签名不同**。
- 影响：调用点靠 import 区分，改名/改材质必漏一处；这正是"玻璃风不统一"的代码根因。
- 改法：定一份 `ui/design/components/`，删掉另一套，全仓改 import；两个 DetailActivity 合并为一个入口。

**M-3（P1）三套播放会话实现**
- 证据：`bridge/PlaybackSession.kt:365`、`playback/MpvPlaybackSession.kt:380`、`playback/NetworkPlaybackSession.kt:387`、`bridge/FfmpegPlaybackSession.kt:403`，且 `playback/MpvStartSequence.kt:338`、`net/OriginPrefetchKit.kt:402` 大量逻辑围绕"起播/预读"。
- 改法：抽 `PlaybackSession` 接口 + 单一实现分发（mpv/ffmpeg 只是 backend 策略），把公共的起播序列/预读/指标抽到独立文件，每个文件回落到 ≤300。

### 4.3 错误处理：静默吞异常（会让故障隐形）

- `emby/EmbyClient.kt:170` `catch (_: Exception) { emptyMap() }` —— 影视信息抓取失败返回空 Map，UI 只会显示"无数据"，不知道是网络错、解析错还是权限错。
- `emby/EmbySessionModel.kt:99,102,107,108` 及 `:221,249,256`：hero/resume/views 全部 `runCatching{…}.getOrDefault(emptyList())`。
- `emby/aggregate/GenreWallData.kt:46`、`emby/aggregate/AggregateFilter.kt:67` 同类兜底。

**为什么是问题**：项目长期笔记里已有铁律「静默早退 = 取证黑洞，每个 `return false` 都要留日志」，但这几处仍在吞异常。用户看到"空分类/0.0 分"时无法判断是服里没有还是取数失败（本次 P0-3 截图里的 "Emby 0.0" 就是典型）。
**改法**：`getOrDefault` 前统一走 `NetDiag.log(tag, "failed: ${e.message}")`，并把失败态与"真空"态用不同 UI 状态区分（错误 chip 而非空列表）。

### 4.4 Compose 状态与生命周期

- 正面案例：`EmbySessionModel` 的 `activeId` 已在 338 改为 Compose state、`EmbyBrowseScreen` 取数监听 sourceId；339 把清理移到 `onDestroy(isFinishing)`。—— 这两个坑刚填过。
- 风险点：`ui/detail/DetailPage.kt` 初值用 preview、`LaunchedEffect` 加载后替换，preview 阶段跳过片源/季集/演员区块；若 `load` 返回失败则永久停留在 preview（只有标题/简介，无片源），且无"重试"入口（P2-10）。
- 风险点：`DesignComponents.kt:48` `LaunchedEffect(Unit)` 里 `while(true){ delay(100) }` 做 20s 渐变相位，页面不可见时仍每 100ms 重组一次；改 `infiniteRepeatable` 动画或按 `Lifecycle` 暂停。

### 4.5 测试覆盖

- 共 52 个测试文件：8 个单元测试（`src/test`，覆盖 `smb/*` 地址/错误/读环/重试门、`hls/HlsMasterTest`、`ui/brand/BrandTimelineTest`），44 个真机 instrumentation（`src/androidTest`，覆盖 mpv 控制/seek/字幕/SMB 播放/Emby 源选择等）。
- **空白**：详情页（`ui/detail/*`）、Emby 首页/切服（338/339 刚改的部分无 instrumentation）、设置页、导航胶囊、玻璃组件 —— 恰好是本次视觉审查发现最多问题的区域，也都没有回归测试。
- 改法：为 338/339 的切服/退出路径补一个 `EmbySwitchInstrumentation`（切 A→B→A，断言服名/hero/无错误态），把本次真机逐帧验证固化下来。

### 4.6 最该先修 5 件事（代码侧）

1. 拆 `PlayerActivity.kt`（901 行）——单文件单职责的最大违反者。
2. 合并两套详情页、两套 `GlassSurface`、两套播放会话入口，消除同名组件与双注册。
3. 给 `EmbyClient` / `EmbySessionModel` 的静默兜底加日志 + 失败态区分。
4. 把 B5 的 300 行守卫从 `.cpp` 扩到 `.kt`，阻止新增超标文件。
5. 为切服/退出/详情 preview 补 instrumentation 回归测试。
