package com.kivo.cinema.subtitle

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

// ============================================================================
// 状态机 reducer 的 JVM 单测（评审 F18：决策逻辑全部桌面可测）。
//
// 锁定的规则（UX v3 §3 + 评审 G32/C9/F4）：
//   - 十态的全部合法/非法转换；
//   - 暂停原因分别维护：任一存在即暂停、全部解除才恢复、手动暂停永不自动恢复；
//   - journal 事件随合法转换产出、非法转换不产事件；
//   - ProcessInterrupted 不自动续译（评审 F4）。
// ============================================================================

class TranslationStateMachineTest {

    private fun reduce(
        state: TranslationStateMachineState,
        event: TranslationEvent,
    ): TranslationReducerOutput = reduceTranslation(state, event)

    @Test
    fun `start from not started produces JOB_CREATED`() {
        val out = reduce(TranslationStateMachineState(), TranslationEvent.StartRequested)
        assertEquals(TranslationTaskState.PREPARING, out.state.taskState)
        assertEquals(TranslationJournalEvents.JOB_CREATED, out.journalEvent)
    }

    @Test
    fun `start from completed is illegal and silent`() {
        val state = TranslationStateMachineState(taskState = TranslationTaskState.COMPLETED)
        val out = reduce(state, TranslationEvent.StartRequested)
        assertEquals(TranslationTaskState.COMPLETED, out.state.taskState)
        assertNull(out.journalEvent)
    }

    @Test
    fun `start from stopped continues previous task`() {
        val state = TranslationStateMachineState(taskState = TranslationTaskState.STOPPED)
        val out = reduce(state, TranslationEvent.StartRequested)
        assertEquals(TranslationTaskState.PREPARING, out.state.taskState)
    }

    @Test
    fun `block commit updates counters and journals`() {
        val state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING)
        val out = reduce(state, TranslationEvent.BlockCommitted(doneCues = 3, totalCues = 10))
        assertEquals(3, out.state.processedCues)
        assertEquals(10, out.state.totalCues)
        assertEquals(TranslationJournalEvents.BLOCK_COMMITTED, out.journalEvent)
    }

    @Test
    fun `pause manual from translating`() {
        val state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING)
        val out = reduce(state, TranslationEvent.PauseRequested)
        assertEquals(TranslationTaskState.PAUSED_MANUAL, out.state.taskState)
        assertEquals(TranslationJournalEvents.PAUSE_REQUESTED, out.journalEvent)
    }

    @Test
    fun `manual pause never auto resumes on system events`() {
        val manual = TranslationStateMachineState(taskState = TranslationTaskState.PAUSED_MANUAL)
        // 系统原因清除对手动暂停无效。
        val out = reduce(manual, TranslationEvent.SystemReasonCleared(TranslationPauseReason.THERMAL))
        assertEquals(TranslationTaskState.PAUSED_MANUAL, out.state.taskState)
        assertNull(out.journalEvent)
    }

    @Test
    fun `system pause stacks reasons and resumes only when all cleared`() {
        var state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING)

        // 两个原因叠加：任一存在即暂停。
        state = reduce(state, TranslationEvent.SystemPaused(TranslationPauseReason.THERMAL)).state
        state = reduce(state, TranslationEvent.SystemPaused(TranslationPauseReason.LOW_BATTERY)).state
        assertEquals(TranslationTaskState.PAUSED_SYSTEM, state.taskState)
        assertEquals(2, state.pauseReasons.size)

        // 清一个还剩一个：仍暂停。
        state = reduce(state, TranslationEvent.SystemReasonCleared(TranslationPauseReason.THERMAL)).state
        assertEquals(TranslationTaskState.PAUSED_SYSTEM, state.taskState)

        // 全清才恢复。
        val out = reduce(state, TranslationEvent.SystemReasonCleared(TranslationPauseReason.LOW_BATTERY))
        assertEquals(TranslationTaskState.TRANSLATING, out.state.taskState)
        assertEquals(TranslationJournalEvents.RESUME_REQUESTED, out.journalEvent)
    }

    @Test
    fun `resume requested ignores system pause`() {
        var state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING)
        state = reduce(state, TranslationEvent.SystemPaused(TranslationPauseReason.THERMAL)).state
        // 手动恢复不解除系统暂停（系统恢复走 SystemReasonCleared）。
        val out = reduce(state, TranslationEvent.ResumeRequested)
        assertEquals(TranslationTaskState.PAUSED_SYSTEM, out.state.taskState)
        assertNull(out.journalEvent)
    }

    @Test
    fun `completed with fallbacks becomes partially done`() {
        var state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING, totalCues = 10)
        state = reduce(state, TranslationEvent.BlockCommitted(10, 10)).state
        val out = reduce(state, TranslationEvent.Completed(failedFallbacks = 3))
        assertEquals(TranslationTaskState.PARTIALLY_DONE, out.state.taskState)
        assertEquals(3, out.state.failedFallbacks)
    }

    @Test
    fun `completed clean becomes completed`() {
        var state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING)
        state = reduce(state, TranslationEvent.Completed(failedFallbacks = 0)).state
        assertEquals(TranslationTaskState.COMPLETED, state.taskState)
    }

    @Test
    fun `source invalidated fails with reason`() {
        var state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING)
        val out = reduce(state, TranslationEvent.SourceInvalidated)
        assertEquals(TranslationTaskState.FAILED, out.state.taskState)
        assertEquals("source subtitle changed", out.state.lastError)
        assertEquals(TranslationJournalEvents.SOURCE_INVALIDATED, out.journalEvent)
    }

    @Test
    fun `process interruption does not auto resume`() {
        var state = TranslationStateMachineState(taskState = TranslationTaskState.TRANSLATING)
        state = reduce(state, TranslationEvent.ProcessInterrupted).state
        assertEquals(TranslationTaskState.INTERRUPTED, state.taskState)

        // 已中断态不响应手动恢复（评审 F4：不悄悄自动续译）。
        val out = reduce(state, TranslationEvent.ResumeRequested)
        assertEquals(TranslationTaskState.INTERRUPTED, out.state.taskState)
    }

    @Test
    fun `stop from any active state`() {
        for (active in listOf(
            TranslationTaskState.TRANSLATING,
            TranslationTaskState.PAUSED_MANUAL,
            TranslationTaskState.PAUSED_SYSTEM,
            TranslationTaskState.INTERRUPTED,
        )) {
            val state = TranslationStateMachineState(taskState = active)
            val out = reduce(state, TranslationEvent.StopRequested)
            assertEquals(TranslationTaskState.STOPPED, out.state.taskState)
        }
        // 终态不响应停止。
        val done = TranslationStateMachineState(taskState = TranslationTaskState.COMPLETED)
        val out = reduce(done, TranslationEvent.StopRequested)
        assertEquals(TranslationTaskState.COMPLETED, out.state.taskState)
        assertNull(out.journalEvent)
    }

    @Test
    fun `failed from preparing is allowed`() {
        val state = TranslationStateMachineState(taskState = TranslationTaskState.PREPARING)
        val out = reduce(state, TranslationEvent.Failed("model missing"))
        assertEquals(TranslationTaskState.FAILED, out.state.taskState)
        assertEquals("model missing", out.state.lastError)
        assertTrue(out.journalEvent == TranslationJournalEvents.JOB_FAILED)
    }
}