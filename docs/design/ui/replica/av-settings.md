# 263 · 音画设置子页 · 家族树设计（先设计后实现）

参考：用户「音画」页 2 帧（内核 radio 卡 / 画面 / 音频 / 画质增强）。
能力盘点（Explore 真查，file:line 见台账）：大部分为未实现能力 ⇒ 置灰+非空原因；真实可接 2 项。

## 真实 vs 置灰

| 行 | 判定 | 接线/原因 |
| --- | --- | --- |
| 内核 radio 卡 | 置灰 | 引擎机制在 PlaybackEngine.select 但无持久化偏好（注释明言待产品化）⇒ 三行 disabled，✓ 标当前 mpv |
| 视频起播方向 | 置灰 | manifest 固定 landscape，无 requestedOrientation ⇒ 自适应未接入 |
| 画面适配方式 | 置灰 | keepaspect/pan-scan/捏合均无；比例 override 已在播放面板 ⇒ 不重复 |
| 单独调整播放亮度 | **真实** | 窗口级 screenBrightness 已有（PlayerGestureRouter:130）；新增 independentBrightness 门控手势亮度，默认 true 保持现状 |
| HDR 动态映射/增强层 | 置灰 | P12-E4 映射本体未实现（hdr.tonemap.implemented=false） |
| 音频引擎 | 置灰 | 双路径随引擎走，无独立用户开关 |
| 音频首选语言 | **真实** | 镜像 slang：新增 audioLanguage+audioAlang()，mpv 起播设 alang |
| SDR转HDR/HDR转SDR | 置灰 | P12-E4 三路径实测不可行 |

## 家族树（落位）

- `ui/design/settings/SettingsAvSub.kt`（新，子页，只组合行原语）≤300
- `ui/design/settings/SettingsRows.kt`（+RadioRow：标题区+右✓，内核 radio 形态）
- `ui/design/settings/SettingsNav.kt`（+AV 路由）
- `ui/design/settings/SettingsPage.kt`（+音画 ChevronRow + 路由映射）
- `playback/PlaybackPrefs.kt`（+audioLanguage/audioAlang()/independentBrightness）
- `playback/MpvPlaybackSession.kt`（起播设 alang，loadFile 前）
- `ui/compose/PlayerGestureRouter.kt`（onVerticalDragStart 亮度门控）

依赖单向：settings UI → playback 域；播放器消费 prefs。不新建包、不平铺。