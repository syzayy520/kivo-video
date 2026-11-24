package com.kivo.cinema.subtitle.online

import android.content.Context
import com.kivo.cinema.subtitle.AiSubtitleController
import com.kivo.cinema.subtitle.TranslationJob
import com.kivo.cinema.subtitle.TranslationJobProgress
import com.kivo.cinema.subtitle.TranslationJobResult

/**
 * 外语字幕 → 中文（design.md §八-7）：复用既有 kivo_core_subtitle_translation 的
 * [TranslationJob.run]（阻塞，IO 线程调用；模型/缓存/取消都由原工具链负责）。
 * [onProgress] 回调分钟级进度（块提交粒度）——在线字幕场景下给面板状态行，避免「无响应」。
 *
 * 引擎经 [AiSubtitleController.engineClient] 按配置选择本地/远程（进程级共享：
 * 此前每次调用 new LocalAiEngineClient = executor 泄漏 + 与播放面板各自线程进 native）。
 * 翻译前经 ensureModelLoadedReason 校验：本地 = 装载模型；远程 = 配置完整。
 */
class OnlineSubtitleTranslator(private val context: Context) {
    fun translateToChinese(
        sourceSrt: String,
        playheadMs: Long = 0L,
        onProgress: ((TranslationJobProgress) -> Unit)? = null,
    ): TranslationJobResult {
        val loadReason = AiSubtitleController.ensureModelLoadedReason(context)
        if (loadReason != null) {
            onProgress?.invoke(TranslationJobProgress("FAILED", 0, 0, null))
            return TranslationJobResult(null, 0, 0, 0, 0, 0, cancelled = false, reason = loadReason)
        }
        return TranslationJob(AiSubtitleController.engineClient(context), context, onProgress)
            .run(sourceSrt, playheadMs)
    }
}