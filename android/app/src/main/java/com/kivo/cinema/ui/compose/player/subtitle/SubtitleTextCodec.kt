package com.kivo.cinema.ui.compose.player.subtitle

// ============================================================================
// SubtitleTextCodec —— 字幕轨的文本/位图判定（AI 翻译内封支持，2026-11-22）。
//
// 与 C++ 提取器（jni_embedded_subtitle_bridge.cxx）同口径：只数可翻译的
// 文本轨，两侧的字幕轨序号（ordinal）才能一一对应。
//
// 边界：位图轨（PGS/VobSub）是图片不是文本——需要 OCR，不在当前范围，
// 拒绝时必须给非空 reason（G9）。
// ============================================================================

/// 内嵌轨是否可翻译的文本字幕。codec 来自 mpv track-list（真值，不猜标题）。
/// 与 C++ 提取器严格同口径（只 SUBRIP/MOV_TEXT）：ASS/WebVTT 的 Dialogue 转换
/// 未实现（v1 偏差登记）——放行会让 ordinal 映射错位或报不准确 reason，
/// 故归入「该字幕格式暂不支持翻译」，宁缺勿错。
internal fun isTextSubtitleCodec(codec: String): Boolean = when (codec.lowercase()) {
    "subrip", "srt", "mov_text" -> true
    else -> false
}

/// 位图字幕（PGS/VobSub/DVB）——图片，不可翻译。
internal fun isBitmapSubtitleCodec(codec: String): Boolean = when (codec.lowercase()) {
    "hdmv_pgs_subtitle", "dvd_subtitle", "dvb_subtitle", "xsub" -> true
    else -> false
}