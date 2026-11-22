package com.kivo.cinema.ui.compose.player.subtitle

import com.kivo.cinema.subtitle.AiEntryState
import com.kivo.cinema.subtitle.TranslationJobProgress

// ============================================================================
// AiRowSubtitle —— 字幕面板「AI 中文字幕」行的副行文案映射（UX v3 §3 四态投影）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/ui/compose/player/subtitle/
//
// 落位原因：T-C 评审登记的偏差（AI 行是静态行，状态投影走点击反馈）——
// 本文件把 AiEntryState（EntryStateResolver 纯函数的输出）映射为副行文案，
// 由 PlayerPanelItems 装配时注入。上屏中文、不带任何开发备注（P0 红线）。
//
// 进度实时跳动未做（偏差登记）：onProgress 每块触发会让面板整列重枚举
// （IO 线程每轮 99 次 JNI 读）——面板打开时快照一次 + onDone 后 panelRevision
// 刷新；细粒度刷新通道列后续。
// ============================================================================

/// TRANSLATING 态的进度段（进度缺失时回退纯文案）。
private fun progressSuffix(progress: TranslationJobProgress?): String =
    progress?.takeIf { it.totalCues > 0 }?.let { "（${it.processedCues}/${it.totalCues} 条）" } ?: ""

/// AiEntryState → 面板副行文案。
internal fun aiRowSubtitle(state: AiEntryState, progress: TranslationJobProgress?): String = when (state) {
    AiEntryState.PREPARING -> "正在准备中文字幕…"
    AiEntryState.TRANSLATING -> "翻译中${progressSuffix(progress)}"
    AiEntryState.PAUSED_MANUAL -> "翻译已暂停"
    AiEntryState.PAUSED_SYSTEM -> "已暂停（系统原因），可继续"
    AiEntryState.INTERRUPTED -> "翻译已中断，可重新开始"
    AiEntryState.STOPPED -> "已停止，可重新开始"
    AiEntryState.PARTIALLY_DONE -> "部分完成，点击重试"
    AiEntryState.COMPLETED -> "译文已就绪，点击挂载"
    AiEntryState.FAILED -> "翻译失败，点击重试"
    AiEntryState.SUBTITLE_NO_FILE -> "需先添加英文字幕"
    AiEntryState.SUBTITLE_ASS -> "仅 ASS 字幕，暂不支持翻译"
    // §19 后内嵌 subrip/mov_text 可译，本态只由「仅内嵌 ASS」触发；v1 装配侧
    // 统一归 ASS_ONLY，分支保留作穷尽防御。
    AiEntryState.SUBTITLE_EMBEDDED -> "仅内嵌 ASS 字幕，暂不支持翻译"
    AiEntryState.SUBTITLE_DUPLICATE -> "已含中文字幕"
    AiEntryState.SUBTITLE_NON_ENGLISH -> "仅非英语字幕，暂不支持翻译"
    AiEntryState.MODEL_MISSING -> "未找到翻译模型"
    AiEntryState.MODEL_CHECK_FAILED -> "翻译模型校验失败"
    // §20 落地为入口行移除（stripAiRow），本态不可达；保留作穷尽防御。
    AiEntryState.FEATURE_DISABLED -> "AI 字幕总开关已关闭"
    AiEntryState.AVAILABLE -> "点击翻译为中文"
}