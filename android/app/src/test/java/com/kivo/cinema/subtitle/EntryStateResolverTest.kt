package com.kivo.cinema.subtitle

import org.junit.Assert.assertEquals
import org.junit.Test

// ============================================================================
// 入口态解析器的表驱动单测（评审 F2 修正版优先级 + G12 四层模型）。
//
// 优先级：功能关闭 > 任务态 > 字幕条件 > 模型条件 > 可用。
// ============================================================================

class EntryStateResolverTest {

    private fun resolve(
        featureEnabled: Boolean = true,
        taskState: TranslationTaskState? = null,
        cacheHit: Boolean = false,
        subtitleIssue: SubtitleIssue = SubtitleIssue.NONE,
        modelIssue: ModelIssue = ModelIssue.NONE,
    ): AiEntryState = resolveAiEntryState(
        AiEntryConditions(
            featureEnabled = featureEnabled,
            taskState = taskState,
            cacheHit = cacheHit,
            subtitleIssue = subtitleIssue,
            modelIssue = modelIssue,
        )
    )

    @Test
    fun `feature disabled wins over everything`() {
        assertEquals(
            AiEntryState.FEATURE_DISABLED,
            resolve(featureEnabled = false, taskState = TranslationTaskState.TRANSLATING,
                    modelIssue = ModelIssue.MISSING),
        )
    }

    @Test
    fun `happy path is available`() {
        assertEquals(AiEntryState.AVAILABLE, resolve())
    }

    @Test
    fun `task states project to their entry states`() {
        assertEquals(AiEntryState.PREPARING, resolve(taskState = TranslationTaskState.PREPARING))
        assertEquals(AiEntryState.TRANSLATING, resolve(taskState = TranslationTaskState.TRANSLATING))
        assertEquals(AiEntryState.PAUSED_MANUAL, resolve(taskState = TranslationTaskState.PAUSED_MANUAL))
        assertEquals(AiEntryState.PAUSED_SYSTEM, resolve(taskState = TranslationTaskState.PAUSED_SYSTEM))
        assertEquals(AiEntryState.INTERRUPTED, resolve(taskState = TranslationTaskState.INTERRUPTED))
        assertEquals(AiEntryState.STOPPED, resolve(taskState = TranslationTaskState.STOPPED))
        assertEquals(AiEntryState.PARTIALLY_DONE, resolve(taskState = TranslationTaskState.PARTIALLY_DONE))
        assertEquals(AiEntryState.COMPLETED, resolve(taskState = TranslationTaskState.COMPLETED))
        assertEquals(AiEntryState.FAILED, resolve(taskState = TranslationTaskState.FAILED))
    }

    @Test
    fun `subtitle issues beat model issues`() {
        // 无字幕 + 缺模型 → 提示字幕（影片级优先于全局配置，评审 F2 修正版）。
        assertEquals(
            AiEntryState.SUBTITLE_NO_FILE,
            resolve(subtitleIssue = SubtitleIssue.NO_SUBTITLE, modelIssue = ModelIssue.MISSING),
        )
        // 仅 ASS + 缺模型 → 提示 ASS。
        assertEquals(
            AiEntryState.SUBTITLE_ASS,
            resolve(subtitleIssue = SubtitleIssue.ASS_ONLY, modelIssue = ModelIssue.MISSING),
        )
    }

    @Test
    fun `embedded and duplicate and non-english have distinct states`() {
        assertEquals(AiEntryState.SUBTITLE_EMBEDDED,
                     resolve(subtitleIssue = SubtitleIssue.EMBEDDED_ONLY))
        assertEquals(AiEntryState.SUBTITLE_DUPLICATE,
                     resolve(subtitleIssue = SubtitleIssue.DUPLICATE_CHINESE))
        assertEquals(AiEntryState.SUBTITLE_NON_ENGLISH,
                     resolve(subtitleIssue = SubtitleIssue.NON_ENGLISH))
    }

    @Test
    fun `model issues surface when subtitle is fine`() {
        assertEquals(AiEntryState.MODEL_MISSING, resolve(modelIssue = ModelIssue.MISSING))
        assertEquals(AiEntryState.MODEL_CHECK_FAILED, resolve(modelIssue = ModelIssue.CHECK_FAILED))
    }

    @Test
    fun `cache hit overrides not started`() {
        assertEquals(
            AiEntryState.COMPLETED,
            resolve(taskState = TranslationTaskState.NOT_STARTED, cacheHit = true),
        )
    }

    @Test
    fun `cache hit alone is completed`() {
        // manifest 丢失但译文 SRT 在：缓存兜底（评审 E24 场景表）。
        assertEquals(AiEntryState.COMPLETED, resolve(cacheHit = true))
    }

    @Test
    fun `task state beats cache hit`() {
        // 翻译中且缓存部分命中 → 显示翻译中（不是已完成）。
        assertEquals(
            AiEntryState.TRANSLATING,
            resolve(taskState = TranslationTaskState.TRANSLATING, cacheHit = true),
        )
    }
}