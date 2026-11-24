package com.kivo.cinema.subtitle.online

import android.content.Context
import com.kivo.cinema.subtitle.LocalAiEngineClient
import com.kivo.cinema.subtitle.TranslationJob
import com.kivo.cinema.subtitle.TranslationJobProgress
import com.kivo.cinema.subtitle.TranslationJobResult

/**
 * 外语字幕 → 中文（design.md §八-7）：复用既有 kivo_core_subtitle_translation 的
 * [TranslationJob.run]（阻塞，IO 线程调用；模型/缓存/取消都由原工具链负责）。
 * [onProgress] 回调分钟级进度（块提交粒度）——在线字幕场景下给面板状态行，避免「无响应」。
 * 能力不可用时由原返回/异常给出原因，不伪造翻译结果。
 */
class OnlineSubtitleTranslator(private val context: Context) {
    fun translateToChinese(
        sourceSrt: String,
        playheadMs: Long = 0L,
        onProgress: ((TranslationJobProgress) -> Unit)? = null,
    ): TranslationJobResult =
        TranslationJob(LocalAiEngineClient(), context, onProgress).run(sourceSrt, playheadMs)
}