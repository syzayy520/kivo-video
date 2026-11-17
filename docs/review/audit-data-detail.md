# 数据层 / 详情页 / 设置页 未入库改动质量审查（Explore-3）

- 审查时间：2026-11-18
- 工作区：C:/kivo video，分支 codex/kivo-video-p0-ui-01-playback-overlay-v13
- 审查方式：只读（git diff / Grep / Read / 内联 Python 复算），未修改任何产品代码
- 报告路径：C:/kivo video/docs/review/audit-data-detail.md

## 总体评价

这一批改动的工程质量总体高，尤其是数据层：TmdbGenres/TmdbPeople/TmdbProviders/TmdbStudios 明确复用了既有 TmdbHome.get（没有重复造鉴权/缓存/超时/错误分类的车轮），MediaTechProbe 有 finally release 和类型容错，C++ prompt/writer 的契约实现与测试经我离线复算三个 FNV 摘要全部匹配，测试是真测试、不是空壳。所有新增 Kotlin 文件均 ≤300 行且职责单一，引用链完整（没有写了没用的孤儿 UI 文件）。

但有三个必须正视的问题：(1) 字幕翻译后端是半成品——引擎只有纯虚接口、两个静态库没被任何 target 链接、没有 JNI 入口，功能实际不可用（P0）；(2) 项目存在两套详情页，新增的技术信息/推荐卡片只接进了简单的 DetailActivity，主用详情体系 MovieDetailActivity + DetailPage 完全没有这些能力（P1）；(3) P0 遗留的 hero 白字压浅底本批未触及，仍待真机验证（P1）。此外 CMake 里硬编码了开发者本机绝对路径 C:/re-tools/...（P1）。

## 维度一：铁律（行数 / 职责单一）

### 1.1 【合规】单文件行数全部 ≤300
【证据】wc -l 实测：TmdbGenres.kt 92、TmdbPeople.kt 55、TmdbProviders.kt 87、TmdbStudios.kt 75、ContinueWatching.kt 50、MediaTechInfo.kt 80、MediaTechProbe.kt 175、DetailInfoCards.kt 154、DetailRelatedVideos.kt 196、DetailTechRows.kt 194、TechBadges.kt 211、DetailActivity.kt 250、TmdbHome.kt 216。
【现象】无一超过 300 行上限。
【为什么是问题】不是问题，铁律该项通过。
【可落地改法】无需改动。

### 1.2 【P2】DetailActivity 单文件职责过多，逼近上限
【文件:行号】C:/kivo video/android/app/src/main/java/com/kivo/cinema/ui/detail/DetailActivity.kt:107-239
【现象】一个 Composable 内承担 hero + 元信息 + 技术徽章 + 评分 + 简介 + 播放按钮 + 私密区 + 文件信息卡 + 技术明细 + 同目录推荐 10 类内容。
【为什么是问题】250 行已逼近 300 红线，继续加卡片会破线；职责不再单一，与项目已抽出的 DetailPage.kt 家族做法不一致。
【可落地改法】把 hero（DetailActivity.kt:123-163）抽成 DetailLocalHero、把正文卡片列（165-235）抽成 DetailLocalBody，Activity 只留编排。

## 维度二：数据层正确性

### 2.1 【合规/亮点】TMDB 新客户端复用既有链路，未重复造轮子
【文件:行号】TmdbGenres.kt:39,54,66、TmdbPeople.kt:34、TmdbProviders.kt:44,76、TmdbStudios.kt:54,67 全部调用 TmdbHome(client).get(...)；TmdbHome.kt:106,122-148 把 auth/get 由 private 改 internal 供同包复用。
【现象】新拉取器只做 JSONObject 到模型的映射；鉴权、8s/15s 超时、12h 列表缓存、五类错误分类全部来自 TmdbHome.get。
【为什么是问题】不是问题——正是项目铁律期望的复用；TmdbClient.kt:26-28 的 staticOpen 也保持了统一出网口。
【可落地改法】无需改动。

### 2.2 【P1】首页冷启动 TMDB 请求风暴，无并发去重 / 限流
【文件:行号】TmdbStudios.kt:50-75（每家 2 次串行，6 家共 12 次）、TmdbProviders.kt:42-86（movie + 前 4 家 titles = 5 次串行）、TmdbGenres.kt:81-92（最多 6 次串行）、MainActivityShell.kt:171-198（6 个 produceState 并行发起）。
【现象】首次进入发现页（无缓存）累计约 26 次 TMDB 请求；虽有 12h 磁盘缓存，但首屏全落网。
【为什么是问题】都在 IO 线程，不阻塞主线程（这点是对的），但跨境高延迟下观看平台/热门演员/制片厂等模块可能数秒才出；TmdbHome.get 没有任何限流或 in-flight 去重，6 个 produceState 之间也无协同。
【可落地改法】给 TmdbHome.get 加进程级 in-flight 去重（同 URL 合并）与最小间隔；TmdbStudios.featured 改为只展示 company 信息、discover 剧照延后到模块可见时再拉。

### 2.3 【P2】错误分类被所有新拉取器丢弃，UI 无法给出准确空态
【文件:行号】TmdbGenres.kt:39,54,66、TmdbPeople.kt:34、TmdbProviders.kt:44,76、TmdbStudios.kt:54,67 均只取 .first；TmdbHome.kt:113-120,123-148 已算出 errorKind。
【现象】TmdbHome.get 返回的 Pair 第二项（offline/timeout/server/secure/lost）被丢弃，6 个模块失败时统一表现为空列表。
【为什么是问题】与不许静默降级铁律的错误按语义上报有差距：用户分不清无 TMDB key、断网还是服务异常，只能看到整段消失。设计注释说空则整段隐藏，属可接受取舍，但可观测性弱。
【可落地改法】新拉取器返回 Result 或把 errorKind 透传到模块空态文案；至少打一条 NetDiag 日志。

### 2.4 【P2】流媒体平台 region 硬编码 US
【文件:行号】MainActivityShell.kt:174-181，TmdbProviders.kt:72 默认 region 同为 US。
【现象】注释明确 watch_region=CN 实测 0 条，故取 US 段展示 Netflix/Disney+ 等。
【为什么是问题】大陆用户会看到无法订阅的平台，属产品口径取舍（注释已承认是推断）；不是 bug，但需产品确认。
【可落地改法】把 region 提为用户设置项或按 locale 自适应，避免硬编码。

### 2.5 【合规】MediaTechProbe 无资源泄漏、线程正确
【文件:行号】MediaTechProbe.kt:74-103（runCatching + finally extractor.release）、169-175（intOrNull/stringOrNull 类型容错）、DetailActivity.kt:110-113（produceState + withContext(Dispatchers.IO)）。
【现象】只建 MediaExtractor、不建 MediaCodec；异常路径也会 release；调用方确实在 IO 线程。
【为什么是问题】不是问题——探测的耗时/泄漏/异常三个风险点都处理到位。
【可落地改法】无需改动。

### 2.6 【P2】MediaTechProbe 异常静默吞且无日志；位深 profile 口径待真机确认
【文件:行号】MediaTechProbe.kt:74-77、58-61,110-121。
【现象】probe() 用 runCatching{}.getOrNull() 吞掉全部异常返回 null，无任何日志；bitDepthOf 用裸 profile 值 0x02/0x2000/0x2001/0x10 比较 KEY_PROFILE。
【为什么是问题】(1) 返回 null 是明确失败信号、UI 据此隐藏（不算把错误当成功），但线上若某机型 setDataSource 大面积失败无从观测；(2) 若平台返回 profile 左移 16 位与 level 的组合值，裸值比较会失配导致 10bit 恒判不出——好在偏保守不误报，但功能会静默缺失。
【可落地改法】catch 里加 NetDiag 日志；用真机导出 KEY_PROFILE 实际值校准常量（或同时兼容裸值与组合值）。

### 2.7 【P2】评分加载用裸 Thread，无生命周期绑定
【文件:行号】DetailActivity.kt:69-75。
【现象】Thread { tmdb.ratingFor(...) }.start() 而非 lifecycleScope 加协程。
【为什么是问题】Activity 销毁后线程仍可能回写 rating；无异常日志。
【可落地改法】改用 lifecycleScope.launch(Dispatchers.IO)。

## 维度三：与既有实现的整合（引用链逐文件核对）

### 3.1 【合规】所有新增文件都被真正接入
【证据】
- TmdbGenres/People/Providers/Studios → MainActivityShell.kt:172,178-179,185,191,198，全部 withContext(Dispatchers.IO)。
- ContinueWatching → MainActivityHomeFeed.kt:61、ContinueSource.kt:38、CalendarSource.kt:180，ProfilePage/ProfileNames.kt 有文档引用。
- MediaTechProbe → DetailActivity.kt:112（IO 线程）。
- TechBadges → DetailActivity.kt:172；DetailTechRows → :224；DetailRelatedVideos → :226；DetailInfoCards（提供 DetailFileInfoCard）→ :217。
【现象】无孤儿 UI 文件（我最初怀疑 DetailInfoCards.kt 是孤儿，核实后确认其导出函数叫 DetailFileInfoCard，已被调用）。
【为什么是问题】不是问题。
【可落地改法】无。

### 3.2 【P1】两套详情页并存，新能力只覆盖其中一套
【文件:行号】本批：新增卡片只在 DetailActivity.kt:172,217,224,226。既有主详情体系：MovieDetailActivity.kt:19,122 → DetailPage.kt:59，入口 MainActivityShell.kt:216（onOpenTmdbItem）、EmbyActivity.kt:126、AggregateSource.kt:23。
【现象】项目里 DetailActivity（本地片源详情，本批改动对象）与 MovieDetailActivity + DetailPage（TMDB/Emby 详情，功能丰富、入口更多）是两套独立详情页。技术徽章/技术明细/文件信息卡/同目录推荐只加进了前者。
【为什么是问题】TMDB/Emby 详情页（用户从首页点 TMDB 条目进入的主路径）完全看不到这批新功能；产品预期若是详情页统一有技术信息，当前只做了一半。MediaTechProbe 只支持本地路径（注释 MediaTechProbe.kt:21-22 承认网络源未纳入）也是只接本地页的原因，但 UI 层可复用。
【可落地改法】明确产品口径：若只服务本地片源，则保持现状并文档化；若要全覆盖，把 TechBadges/DetailTechRows 下沉为公共组件，接入 DetailPage 的资源槽/下半屏节（DetailPageSections.kt）。

### 3.3 【P2】两套相关推荐口径不同、新旧共存
【文件:行号】DetailRelatedVideos.kt:152-163（本地同目录，最多 6 条）vs DetailPageSections.kt:91-93（TMDB similar）。
【现象】同一产品概念相关视频有两套数据源与实现。
【为什么是问题】口径不一致（本地文件 vs TMDB 网络），长期维护易混淆。
【可落地改法】统一命名（如同目录影片）或在 DetailPage 里明确两者定位差异。

### 3.4 【P2】MediaTechInfo.is4K/hasHdr 为未使用死代码且口径与 TechBadges 矛盾
【文件:行号】MediaTechInfo.kt:76,79-80（is4K = width>=3840 || height>=2160 按长边）、TechBadges.kt:83-92（按短边 minOf(width,height)>=2160）。
【现象】is4K/hasHdr 全仓无引用（Grep 仅命中定义行）；两个 4K 判定口径语义冲突：1080×2400 竖屏在 is4K 为 true、在 TechBadges 显示 1080P。
【为什么是问题】目前没触发是因为 DetailTechRows 不显示分辨率行；但这是埋着的地雷——将来谁用 is4K 画徽章，同一片源两处结论就会不同，正是 DetailTechRows.kt:36-37 注释声明要避免的。
【可落地改法】删除 is4K/hasHdr 死代码，或统一改为短边判定并让 TechBadges 复用它。

## 维度四：C++ 字幕翻译（4 hpp + 2 cpp + 2 test）

### 4.1 【P0】引擎无实现 + 库未链接 + 无 JNI = 功能实质不可用
【文件:行号】subtitle_translation_engine.hpp:45-83（仅纯虚接口，全仓无派生实现）；android/app/src/main/cpp/CMakeLists.txt:100-109,129-143（add_library kivo_core_subtitle_translation STATIC、llama_ext IMPORTED）；同文件全篇 target_link_libraries 仅见 llama_ext INTERFACE（133/139/141）与既有业务库，没有任何一处把这两个新库链进 kivo_player/kivo_mpv 主 so；全仓无 JNI/Kotlin 调用 SubtitleTranslationEngine/build_subtitle_translation_prompt（后者只在 CMake 注释 cpp/CMakeLists.txt:96 出现）。
【现象】prompt 契约与 SRT 写出做完了，但真正调用推理内核的那一层没有；静态库即使编出也不进产物；无入口。
【为什么是问题】注释 subtitle_translation_engine.hpp:9-13 自称这是一条缝，另一侧要新写——从交付角度这就是半成品：桌面单测全绿会给人功能完成的错觉，而 App 里 AI 字幕完全不可用。与写了没用同属最严重的整合缺陷。
【可落地改法】若本批就是阶段提交，需在 PR 描述/文档明确标注引擎未接线、不可用于 App；若要可用，必须补 llama.cpp 封装派生类 + 行数校验/CJK 守卫/块调度 + JNI + 在主 so 里 target_link_libraries。

### 4.2 【P1】CMake 硬编码开发者本机绝对路径，他机静默跳过
【文件:行号】android/app/src/main/cpp/CMakeLists.txt:122-146，set(KIVO_LLAMA_ANDROID_DIST 为 C:/re-tools/llama.cpp-android/dist CACHE PATH)。
【现象】llama 静态库产物路径写死为作者机器路径；注释明说目录不存在时整个目标静默跳过。
【为什么是问题】其他开发者/CI 机器上该路径不存在 → llama_ext 整个不定义 → 翻译链路彻底不编译，且只有 message(STATUS) 无告警，构建产物在不同机器上不确定。这与不许静默降级铁律相悖（构建层的静默降级）。
【可落地改法】改为相对仓库路径 third_party/llama.cpp-android/dist 或读环境变量；找不到时 message(WARNING) 或要求显式 -DENABLE_SUBTITLE_TRANSLATION=ON 才启用。

### 4.3 【合规/亮点】接口实现完整、内存安全、测试真实可运行
【文件:行号】
- 头文件声明全部有 cpp 实现：build_subtitle_translation_prompt / clean_subtitle_translation_output / subtitle_translation_has_cjk / fewshot_examples / output_prefixes（subtitle_translation_prompt.cpp:126-260）、write_translated_srt（translation_result_writer.cpp:35-58）。
- 内存/生命周期：C 数组输出带 capacity 防溢出 + nullptr 检查（subtitle_translation_prompt.cpp:126-152）；SRT 时间戳用 std::snprintf 定长 buffer（translation_result_writer.cpp:26-30）；序号不跳号、CRLF（46-54）。
- 测试真实：backend/CMakeLists.txt:466-529 注册两个独立 executable + add_test + TIMEOUT 60，各自链接 kivo_core_subtitle_translation，避开了双 main 冲突。
- 摘要复算：我用只读内联脚本按 build_subtitle_translation_prompt 的拼接逻辑复算了 FNV-1a 64：system=fd02f9ebd70db0e1、single=b4969d0b8e8f3a37、fewshot=0f2b30ad34d46177，与 subtitle_translation_prompt_test.cpp:61-63 三个期望常量全部一致；examples 解析为 8 条，与 test_fewshot_examples_present 的 assert(count==8) 一致。
【现象】契约实现与 Python 真源一致，测试断言在摘要/形状/清洗/CJK/清单各层都成立。
【为什么是问题】不是问题——这是本批质量最高的部分。
【可落地改法】无需改动；建议把三个摘要的复算固化为 CI 步骤。

### 4.4 【P2】CJK 守卫未校验 UTF-8 continuation bytes
【文件:行号】subtitle_translation_prompt.cpp:198-237（尤其 226-229）。
【现象】手写 UTF-8 解码只检查 index+length > size 越界，未验证后续字节是否在 0x80..0xBF。
【为什么是问题】畸形序列可能被误判为含 CJK 而放过回吐英文失败样本；只影响守卫精度，不会误删正常译文。
【可落地改法】对每个 continuation byte 加 (b 与 0xC0) 等于 0x80 校验。

### 4.5 【P2】SRT 负时间钳位边界与注释易误解
【文件:行号】translation_result_writer.cpp:19-31,41、subtitle_translation_prompt.hpp:9 vs subtitle_translation_prompt_test.cpp:61-63。
【现象】起止时间均为负时，过滤判断（41 行，原始值）在前、钳位（19-31）在后，可能产出 0 时长行；另外 hpp 注释写的是 SHA256，测试用的是 FNV-1a，算法不同但注释未说明。
【为什么是问题】均为低概率边界/易读性问题，不影响主流程。
【可落地改法】先钳位再过滤；hpp 注释补一句测试用 FNV-1a，本 SHA256 是 prompt 源指纹，二者用途不同。

## 维度五：回归风险

### 5.1 【合规】AndroidManifest +6 无新权限，新增 Activity 真实存在
【文件:行号】AndroidManifest.xml:41-46；CalendarActivity.kt:19、themes.xml:5（Theme.KivoCinema）、ProfilePage.kt:92-93（入口）。
【现象】新增 .ui.profile.calendar.CalendarActivity，exported=false，主题 @style/Theme.KivoCinema（注释明确避开不存在的 Theme.Kivo）。
【为什么是问题】不是问题：类、样式、入口都在，无新权限，不会导致安装/编译翻车。
【可落地改法】无。

### 5.2 【P1】P0 遗留 hero 白字压浅底本批未触及，仍待真机验证
【文件:行号】本批 DetailActivity.kt:123-163 在 diff 中无任何变化；当前标题 Text(... color = Color.White) 在 :158-162，仅靠 :143-145 的 Brush.verticalGradient(Transparent → Black 0.55f) 遮罩。另主详情体系 DetailPage.kt:122（fg = if (chromeDark) Color.White else textMain）、DetailHeroContent.kt:49 本批也未改。
【现象】浅色缩略图下 55% 黑渐变对白字的对比度是否足够没有实证。
【为什么是问题】team-lead 点名的 P0 对照项；本批改的是详情页内容，不是 hero，问题若仍在则未关闭。
【可落地改法】真机用浅色剧照（如白色海报）验证白字可读性；必要时把渐变底 alpha 提到 0.7 以上或给标题加 text shadow/scrim。

### 5.3 【合规】Emby 0.0 未引入
【文件:行号】DetailActivity.kt:175。
【现象】RatingBadges(if (rating != null) listOf(rating!!) else emptyList())，rating 仅来自 TMDB，无值即空列表。
【为什么是问题】不是问题——没有把 0.0 当评分显示。
【可落地改法】无。

### 5.4 【合规】backend/CMakeLists +66 注册正确
【文件:行号】backend/CMakeLists.txt:466-529。
【现象】两测试各一 target、add_test、TIMEOUT 60、单独 /utf-8，与仓库既有 60+ 测试惯例一致。
【为什么是问题】不是问题。
【可落地改法】无。

### 5.5 【P2】PrivateActivity FLAG_SECURE 加固合理，但 onDestroy 清标志冗余
【文件:行号】PrivateActivity.kt:56-72（onCreate add / onDestroy clear）、原 :111-118（删除 SideEffect 条件式）。
【现象】从仅解锁后挂 FLAG_SECURE 改为全程挂，密码键盘页也防截屏。
【为什么是问题】安全加固方向正确；但 FLAG_SECURE 是 per-window，不会把整个 App 一起禁截图，onDestroy 的清除属冗余（且配置变更时新旧实例 window 不同）。无害，仅提示。
【可落地改法】可保留；若精简，去掉 onDestroy 与相应注释即可。

### 5.6 【合规】SourceWorkspace 新增渐变引用的 token 均存在
【文件:行号】SourceWorkspace.kt:23-36（DesignTokens.pageGradient.libraryStart/End、ShellInsets.NavReservedHeight）；DesignPalette.kt:95-96、ShellInsets.kt:13 均有定义。
【现象】改动只加背景渐变与统一底部预留高度，无悬空引用。
【为什么是问题】不是问题。
【可落地改法】无。

### 5.7 【P2】本地影片页 3 列 8dp 对照项
【文件:行号】本批未触及 PrivateActivity.kt 网格规格（注释仍为 2:3 卡 3 列，PrivateActivity.kt:45-47）；8dp 间距口径不在本批新增文件内。
【现象】未发现本批引入该回归。
【为什么是问题】team-lead 点名的对照项，本批代码无对应改动，无法从静态审查确认真机观感。
【可落地改法】真机对照 P0 记录复核 3 列布局与 8dp 间距。

## 最该修的 5 件事

1. 【P0】给字幕翻译后端接线或在文档中明确其为阶段半成品：SubtitleTranslationEngine 无实现、两个静态库未链接、无 JNI（subtitle_translation_engine.hpp:45-83、cpp/CMakeLists.txt:100-143）。当前桌面单测全绿会误导交付判断。
2. 【P1】移除 CMake 硬编码本机路径：cpp/CMakeLists.txt:122 的 C:/re-tools/... 改为相对路径/环境变量，并把找不到产物从静默跳过改为显式告警。
3. 【P1】把技术信息能力接入主详情体系：TechBadges/DetailTechRows 目前只在 DetailActivity，主用 MovieDetailActivity + DetailPage（MainActivityShell.kt:216、EmbyActivity.kt:126）缺位；明确产品口径或下沉公共组件。
4. 【P1】关闭 hero 白字压浅底：本批未触及 hero（DetailActivity.kt:143-162、DetailPage.kt:122），需浅色剧照真机验证并按需加 scrim/阴影。
5. 【P1/P2】收敛 TMDB 冷启请求并保留错误语义：TmdbStudios.kt:50-75 等造成约 26 次首屏请求且无 in-flight 去重（MainActivityShell.kt:171-198）；新拉取器丢弃 errorKind（TmdbGenres.kt:39 等）。加去重/限流并把错误分类透传到空态或日志。
