# AI 字幕设置子页（AI Subtitles）· 家族树设计（实现前置门闸）

> 来源：用户 2026-11-13 提供的「AI 字幕」9 帧截图（根页 OFF/ON、服务商 picker、模型 picker、
> 语音识别模型页两帧、系统支持的源语言页）。延续 261→262→263→264 设置家族节奏。
> 本稿先定家族树与逐行真实能力，**未完成本设计不得实现**。措辞遵守 D6：不写参考产品名，统一称「参考」。

## 〇、能力审计（2026-11-13 实测）

| 审计项 | 证据 | 结论 |
| --- | --- | --- |
| 语音识别（ASR）后端 | grep `whisper\|SpeechRecognizer\|speech_recognition\|语音识别` android/app/src/main = 0 命中；backend 仅 translation-unit 编译术语注释 | **无** |
| 翻译/LLM 客户端 | grep `openai\|deepseek\|gemini\|AI 字幕\|AiSub` = 0 命中 | **无** |
| 现有 API Key | TmdbClient.kt:44 `apiKey()`；SettingsStore.kt:28 `tmdbKeyConfigured` | **TMDB 专用**，非 LLM key，不复用不混存 |
| 系统翻译语言包 API | Android 无 iOS 式系统翻译公开 API | **无** |
| 字幕偏好语言 | PlaybackPrefs.kt:66-68 `subtitleLanguage` + :228 `languageLabel` | **真**，可作目标语言读值 |
| 行原语 | SettingsRows.kt：SectionCard:45 / BetaBadge:66 / ChevronRow:88(enabled) / ValueRow:108 / InfoRow:120 / SwitchRow:130(enabled) | 齐备，小升级即可 |
| 路由 | SettingsNav.kt:20 单层六路由；SettingsPage.kt:62 sub 分派 | +AI 路由；钻入页用子页内部状态，不升级导航栈 |

**总判定**：本页几乎全组无后端 ⇒ 灰行+非空原因为主；真实部分 = 钻入导航、目标语言读值、
模型目录实测、用量空态。**不放假开关、不假「可下载」、不放 ON 态死 UI**。

## 一、页面结构（3 页）

### 根页「AI 字幕」
| 组 | 行 | 参考形态 | 本版本判定 |
| --- | --- | --- | --- |
| （无节头卡） | 启用 AI 字幕 | Switch ON | **灰 Switch OFF**+原因「语音识别与翻译内核尚未接入」 |
| 语音转字幕 [Beta] | 识别音轨中的语音，生成字幕。 | ›+橙「缺少模型」 | **真钻入**→语音识别模型页；橙「缺少模型」为真（无模型无下载器） |
| 字幕翻译 | 使用 AI 翻译 | Switch | **灰 Switch OFF**+原因「大模型翻译客户端尚未接入」 |
| 字幕翻译 | 系统支持的源语言 | ›「0 种 /10」 | **真钻入**→源语言页；「0 种 /10」为真（未接入⇒可用 0） |
| 翻译用量 | 还没有翻译用量记录 | 灰信息行 | **真**空态（dim ValueRow） |

脚注 3 处：识别脚注/翻译脚注/用量脚注，形态对齐、内容改写为 Android 现实（见 §四）。
**参考 ON 态条件行不放置**：服务商(13 项 picker)/API Key/模型 picker/自定义翻译规则/测试翻译服务
——灰开关不可达，放即死 UI；将来内核接入时按家族补（披露 D1）。

### 钻入页「语音识别模型」
| 行 | 判定 |
| --- | --- |
| 模型数量 / 空间占用 | **真**实测 `filesDir/asr-models`（约定模型目录，空⇒0 / 0 KB） |
| 刷新模型列表 | 蓝 LinkRow **灰**+原因「模型源尚未接入」 |
| 13 语言行（英日韩西德法白俄波俄克罗泰乌意） | **灰** ChevronRow，值「未下载」为真+原因「识别内核尚未接入」 |
| 脚注 | 改写：下载后按大小/内存自动选模的参考语义保留为「接入后」口径 |

### 钻入页「系统支持的源语言」
| 行 | 判定 |
| --- | --- |
| 目标语言 | **真**读 `PlaybackPrefs.subtitleLanguage` 标签（zh→中文） |
| 10 语言行（俄德意日法英葡西阿韩） | dim ValueRow 值「未接入」（参考「可下载」为假承诺，不放假值） |
| 刷新已下载语言 | 蓝 LinkRow **灰**+原因「Android 无系统翻译语言包 API」 |
| 脚注 | 改写为 Android 现实 |

## 二、家族树（新增 3 文件 + 既有家族小升级，不新建包、不平铺）

| 文件 | 职责 | 动作 |
| --- | --- | --- |
| `ui/design/settings/SettingsAiSub.kt` | AI 根页 + 内部三屏状态(ROOT/ASR/SYSLANGS)+BackHandler | **新增** ≤300 |
| `ui/design/settings/SettingsAsrModels.kt` | 语音识别模型页（纯页面 composable，收 back） | **新增** ≤300 |
| `ui/design/settings/SettingsSysLangs.kt` | 系统支持的源语言页（纯页面 composable，收 back） | **新增** ≤300 |
| `ui/design/settings/SettingsRows.kt` | SectionCard+beta；ChevronRow+value/valueColor；ValueRow+dim；+LinkRow | 改（小升级） |
| `ui/design/settings/SettingsStore.kt` | +`asrModelStats()` 实测模型目录（读模型门面） | 改（小） |
| `ui/design/settings/SettingsNav.kt` | 路由 +AI | 改 |
| `ui/design/settings/SettingsPage.kt` | 根页 +AI 字幕入口 + sub 分派 | 改 |

依赖方向不变：settings UI → PlaybackPrefs（只读 subtitleLanguage）/ SettingsStore（实测）。
**PlaybackPrefs 不新增键**：灰开关不持久化（无假状态），目标语言复用既有键。
内部钻入不升级 SettingsNav 栈：子页内 `rememberSaveable` 屏枚举 + BackHandler（内层先消费）。

## 三、默认值与披露

- 灰开关恒 OFF 不可切换；无新增持久化键 ⇒ 无默认值债。
- D1：参考 ON 态条件行（服务商/API Key/模型/规则/测试）不放置，灰开关不可达；接入时补家族。
- D2：「缺少模型」「0 种 /10」「未下载」「还没有翻译用量记录」均为真实空态，非假值。
- D3：目标语言显示既有字幕偏好标签（中文），与参考「中文 (简体)」措辞异，真实值优先。
- D4：`filesDir/asr-models` 为约定模型目录，实测 0/0 KB；目录不存在按 0 计（runCatching）。
- D5：iOS 系统翻译脚注改写为 Android 现实（无系统翻译语言包 API）。

## 四、脚注改写（形态对齐、内容诚实）

- 识别：「识别在本机运行，需下载对应语言模型。当前版本尚未接入识别内核，模型列表为空。」
- 翻译：「Android 无 iOS 式系统翻译语言包接口；当前版本尚未接入翻译内核，源语言可用数为 0。」
- 用量：「记录这台设备用于字幕翻译的累计 token 用量，仅供参考，实际费用以服务商账单为准。当前未接入翻译，无记录。」
- 模型页：「接入后，播放器会按模型大小和当前可用内存自动选择可用模型；内存不足时跳过大模型，尝试同语言小模型。」
- 源语言页：「这里列出可翻译成当前目标语言的原文语言。Android 无系统翻译语言包接口，当前版本尚未接入翻译内核。」

## 五、闸门

1. 视觉：3 页节头/卡/行形态/脚注/Beta/橙状态与参考逐行对位（灰化适配披露）。
2. 无假状态：灰行均带非空原因；无假开关/假「可下载」；ON 态死 UI 不放置。
3. 真实复算：目标语言随字幕偏好联动（播放子页改 zh→en 后本页复算）；模型数量/空间占用实测；钻入/返回栈真机验证。
4. 行数：3 新文件各 ≤300；SettingsRows 升级后 ≤300。
5. 真机：K40 b6e950ee 截图 + 导航复算 + 设置其余页不回归。
6. 证据：docs/evidence/p265/ai-subtitles/。