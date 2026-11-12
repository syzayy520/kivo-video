# P265 · AI 字幕设置子页 · 真机取证

设备：Redmi K40（`b6e950ee`）， portrait 1080×2400。
构建：`./gradlew :app:assembleDebug --no-configuration-cache` → `BUILD SUCCESSFUL`（compileDebugKotlin 执行）。
安装：`adb install -r`（HyperOS 弹「继续安装」tap 347,2111）→ `Success`。
设计稿：`docs/design/ui/replica/ai-subtitles-settings.md`（家族树先行，审计结论见下）。

## 一、审计结论（为什么全是置灰）

仓库 grep `whisper|openai|deepseek|gemini|SpeechRecognizer|语音识别|AI 字幕` = 0 命中 ⇒ **无 ASR / LLM / 系统翻译后端**。
API Key 仅 TMDB 一家（`TmdbClient.kt`），与翻译无关。Android 无 iOS 式系统翻译语言包接口。
故所有开关 / 服务商 / 模型下载 / 刷新一律 **置灰 + 非空原因**；ON 态条件行（服务商 13 选 / API Key /
模型选择 / 自定义翻译规则 / 测试翻译服务）**不实现** —— 灰开关不可达，不留死 UI。
真实部分仅四：三级钻入导航、目标语言读 `PlaybackPrefs.subtitleLanguage`、模型目录实测、用量空态。

## 二、截图索引

| 文件 | 内容 | 证明 |
| --- | --- | --- |
| c07 | 设置根页，AI 字幕入口卡（音画之后） | 路由接线 §五.1 |
| c08 | AI 根页 OFF：总开关灰+原因、Beta、橙色「缺少模型」、0 种 / 10、用量空态、脚注 | 视觉/无假 §五.1/2 |
| c09 | 语音识别模型页：模型数量 0 / 空间占用 0 B、刷新灰+原因、13 语言「未下载」灰+原因 | 真实实测 §五.3 |
| c10 | 从模型页 Back 回 AI 根页（不退出） | 内部返回栈 §五.1 |
| c11 | 系统源语言页：目标语言=中文（读 zh） | 真实读回 §五.3 |
| c12–c15 | 播放子页滚动到「字幕首选语言」（导航过程） | 联动操作路径 |
| c16 | 语言选择器，中文 ✓ | 联动起点 |
| c17 | 选英语后播放子页值=英语 | 写入生效 |
| c18 | 系统源语言页目标语言=英语 | **联动成立** §五.3 |
| c19 | 选择器英语 ✓（确认持久化） | 写入持久化 |
| c20 | 恢复中文 | 默认恢复 |

## 三、闸门映射（设计 §五）

1. **视觉/结构**：c07/c08/c09/c11 三页 + 根入口，inset 卡 / 固定头 / Beta / 脚注与参考一致。
2. **无假控件**：c08/c09 所有无后端行置灰且副标题非空（"…尚未接入"）；无 ON 态死 UI。
3. **真实读回**：
   - 目标语言联动：c11(中文) → 改 en → c18(英语) → 恢复 c20(中文)。证明读 `subtitleLanguage` 而非硬编码。
   - 模型统计：c09 显示 0 / 0 B；`run-as ls files/asr-models` = `No such file or directory` ⇒ `asrModelStats()` 返回 0/0，实测非造假。
   - 无新增持久化键：`playback_prefs.xml` 仍为 p264 键集（见 prefs-restored.xml），灰开关不写 prefs。
4. **行数**：SettingsAiSub 81 / SettingsAsrModels 48 / SettingsSysLangs 43 / SettingsRows 229 / SettingsStore 69 / SettingsNav 50 / SettingsPage 77，均 ≤300。
5. **真机**：K40 b6e950ee，非模拟器。
6. **证据**：本目录 c07–c20 + prefs-restored.xml。

## 四、默认恢复

- 字幕语言 zh→en→zh，最终 `subtitle_language=zh`（prefs-restored.xml）。
- p264 其余默认未动（double_tap_sides=seek / show_next_button=true / vertical_swipe=true / default_speed=1.0 / seek_forward_seconds=15 等）。
- 测试时锁定竖屏（accelerometer_rotation 0 / user_rotation 0），取证后恢复 accelerometer_rotation 1。

## 五、披露

- D1 无 ASR/LLM/系统翻译后端 ⇒ 全置灰带原因（审计 0 命中）。
- D2 ON 态条件行不实现（灰开关不可达，避免死 UI）。
- D3 Android 无系统翻译语言包接口 ⇒ 源语言可用数 0，清单为参考静态列表。
- D4 模型目录不存在 ⇒ 0 / 0 B 为实测空值，非伪造。
- D5 用量空态（未接入翻译，无记录）。
- D6 启动自动恢复上次测试片源（横屏测试图），需先退出播放器才能进设置；属导航过程，非本页缺陷。
- D7 MIUI 过滤应用 logcat ⇒ 无应用日志；内核侧证据 = 真机占位 + 代码（与 p264 同口径）。