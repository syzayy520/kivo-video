package com.kivo.cinema.subtitle

// ============================================================================
// EntryStateResolver —— 字幕面板 AI 入口的展示态解析（纯函数，表驱动单测）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/EntryStateResolver.kt
//
// 设计依据: UX 规范 v3 §3（评审 F2 修正版优先级 + C7 十态总表）。
//   优先级（自上而下，多条件叠加取第一个命中）：
//     功能关闭 → 任务态 → 字幕条件 → 模型条件 → 可用
//   （字幕条件属于「这一部影片」，模型条件属于「全局一次性配置」——
//     没字幕的片子反复提示装模型是噪音，评审 F2 修正版顺序。）
//
// 纯函数：输入全部值类型，输出枚举——表驱动单测锁定（评审 F18）。
// ============================================================================

/// 功能层：设置总开关「启用 AI 字幕翻译」（关闭 = 入口灰显 + 任务暂停保留，
/// 不删数据 —— 评审 C9/G9；灰显保可发现性，评审 F2/G9）。
enum class FeatureSwitch { ENABLED, DISABLED }

/// 字幕条件（cue_assembler 语言检测 + 格式判定产出）。
enum class SubtitleIssue {
    NONE,               ///< 有英文 SRT 外挂
    NO_SUBTITLE,        ///< 无任何字幕
    ASS_ONLY,           ///< 仅 ASS/SSA（v1 拒收）
    EMBEDDED_ONLY,      ///< 仅内嵌字幕（v1 不做提取）
    DUPLICATE_CHINESE,  ///< 已含中文（默认拒收；逃生口「仍要翻译」列二期）
    NON_ENGLISH,        ///< 非英语（假名/其他拉丁语系按保守口径）
}

/// 模型条件（AiSubtitleModelStore 校验产出）。
enum class ModelIssue { NONE, MISSING, CHECK_FAILED, STORAGE }

/// AI 入口的展示态（UX v3 §3 总表的投影）。
enum class AiEntryState {
    FEATURE_DISABLED,     ///< 总开关关闭（灰显，点按引导去设置）
    PREPARING,            ///< 正在准备中文字幕（首块之前）
    TRANSLATING,          ///< 翻译中（已就绪至 mm:ss 主指标）
    PAUSED_MANUAL,        ///< 翻译已暂停（手动）
    PAUSED_SYSTEM,        ///< 已暂停（系统原因入副行）
    INTERRUPTED,          ///< 翻译已中断（可继续）
    STOPPED,              ///< 已停止（可继续）
    PARTIALLY_DONE,       ///< 部分完成（N 条保留原文 + 重试入口）
    COMPLETED,            ///< 已完成
    FAILED,               ///< 翻译失败（原因）
    SUBTITLE_NO_FILE,     ///< 无字幕
    SUBTITLE_ASS,         ///< 仅 ASS
    SUBTITLE_EMBEDDED,    ///< 仅内嵌
    SUBTITLE_DUPLICATE,   ///< 已含中文
    SUBTITLE_NON_ENGLISH, ///< 非英语
    MODEL_MISSING,        ///< 缺模型（路径指引）
    MODEL_CHECK_FAILED,   ///< 模型校验失败
    AVAILABLE,            ///< 可用（happy path：开启后 AI 翻译为中文）
}

/// 入口解析的输入条件（全部值类型）。
data class AiEntryConditions(
    val featureEnabled: Boolean,
    /// 非空 = 任务已存在（taskState 生效）；null = 该片还没有翻译任务。
    val taskState: TranslationTaskState?,
    /// 缓存命中（已完成/部分完成的译文可用）——覆盖「待开始」的展示。
    val cacheHit: Boolean = false,
    val subtitleIssue: SubtitleIssue = SubtitleIssue.NONE,
    val modelIssue: ModelIssue = ModelIssue.NONE,
)

/// 解析入口态。纯函数，无副作用。
fun resolveAiEntryState(conditions: AiEntryConditions): AiEntryState {
    // 1. 功能层。
    if (!conditions.featureEnabled) { return AiEntryState.FEATURE_DISABLED }

    // 2. 任务层（任务存在即以任务态投影；终态/暂停优先于字幕条件——
    //    已有的翻译结果不该被「缺模型」之类盖掉，评审 G12 四层模型的意图）。
    //    NOT_STARTED 的缓存命中兜底在下方统一处理（评审 E24/G11）。
    val task = conditions.taskState
    if (task != null && task != TranslationTaskState.NOT_STARTED) {
        return when (task) {
            TranslationTaskState.PREPARING -> AiEntryState.PREPARING
            TranslationTaskState.TRANSLATING -> AiEntryState.TRANSLATING
            TranslationTaskState.PAUSED_MANUAL -> AiEntryState.PAUSED_MANUAL
            TranslationTaskState.PAUSED_SYSTEM -> AiEntryState.PAUSED_SYSTEM
            TranslationTaskState.INTERRUPTED -> AiEntryState.INTERRUPTED
            TranslationTaskState.STOPPED -> AiEntryState.STOPPED
            TranslationTaskState.PARTIALLY_DONE -> AiEntryState.PARTIALLY_DONE
            TranslationTaskState.COMPLETED -> AiEntryState.COMPLETED
            TranslationTaskState.FAILED -> AiEntryState.FAILED
            // NOT_STARTED 已在外层过滤，这里仅为 when 穷尽性（返回值不可达）。
            TranslationTaskState.NOT_STARTED -> AiEntryState.AVAILABLE
        }
    }

    // 3. 缓存命中兜底（任务为 null 但译文可用——极端：manifest 丢失但 SRT 在）。
    if (conditions.cacheHit) { return AiEntryState.COMPLETED }

    // 4. 字幕条件（影片级，优先于全局模型条件——评审 F2 修正版）。
    when (conditions.subtitleIssue) {
        SubtitleIssue.NO_SUBTITLE -> return AiEntryState.SUBTITLE_NO_FILE
        SubtitleIssue.ASS_ONLY -> return AiEntryState.SUBTITLE_ASS
        SubtitleIssue.EMBEDDED_ONLY -> return AiEntryState.SUBTITLE_EMBEDDED
        SubtitleIssue.DUPLICATE_CHINESE -> return AiEntryState.SUBTITLE_DUPLICATE
        SubtitleIssue.NON_ENGLISH -> return AiEntryState.SUBTITLE_NON_ENGLISH
        SubtitleIssue.NONE -> {}
    }

    // 5. 模型条件（全局一次性配置）。
    when (conditions.modelIssue) {
        ModelIssue.MISSING -> return AiEntryState.MODEL_MISSING
        ModelIssue.CHECK_FAILED -> return AiEntryState.MODEL_CHECK_FAILED
        ModelIssue.STORAGE -> return AiEntryState.MODEL_MISSING
        ModelIssue.NONE -> {}
    }

    // 6. 可用（happy path）。
    return AiEntryState.AVAILABLE
}