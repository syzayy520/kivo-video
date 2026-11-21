package com.kivo.cinema.subtitle

// ============================================================================
// 翻译任务状态机 —— 纯 reducer（(state, event) → (state, journalEvents)）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/TranslationStateMachine.kt
//
// 设计依据: docs/design/ai-subtitle-translation-family-tree.md (v3.2 §2/§3)
//   - **四层状态模型**（评审 E12/G12）：功能层（FeatureEnabled，不在本文件）·
//     任务层（本文件十态）· 环境层（PauseReason 集合）· 展示层（EntryStateResolver）；
//   - **暂停原因分别维护**（评审 G32）：任一存在即暂停、全部解除才恢复、
//     手动暂停永不自动恢复（三条规则都在 reducer 里）；
//   - **journal 事件产出**（评审 C5 十类事件）：reducer 只产出**事件名**，
//     落盘由 TranslationJournal 执行 —— journal 是进度真源（DEC-08 v2），
//     manifest 是意图真源（评审 F4 分工）。
//
// 纯函数：不读时钟、不做 IO —— 全部可 JVM 单测（评审 F18：最易出 bug 的
// 逻辑不许留在无桌面测试的一侧）。
// ============================================================================

/// 任务层十态（UX 规范 v3 §3 总表）。
enum class TranslationTaskState {
    NOT_STARTED,      ///< 待开始（可用未开启）
    PREPARING,        ///< 准备中（校验字幕/模型/存储 + 引擎初始化）
    TRANSLATING,      ///< 翻译中
    PAUSED_MANUAL,    ///< 已暂停（手动，永不自动恢复）
    PAUSED_SYSTEM,    ///< 已暂停（系统：热控/低电量/存储/内存——原因集合）
    INTERRUPTED,      ///< 已中断（进程被杀后重进；评审 F4：不自动续译）
    STOPPED,          ///< 已停止（用户关闭中文字幕；可继续）
    PARTIALLY_DONE,   ///< 部分完成（存在失败回退行，可重试）
    COMPLETED,        ///< 已完成（无 PENDING；领先窗口口径 = 已覆盖全部已播放区域）
    FAILED            ///< 翻译失败（重试穷尽）
}

/// 系统暂停原因（评审 G32：分别维护，不是一个共享 Boolean）。
enum class TranslationPauseReason { THERMAL, LOW_BATTERY, STORAGE_FULL, MEMORY_PRESSURE }

/// 状态机事件（调用方发起或系统产生）。
sealed class TranslationEvent {
    object StartRequested : TranslationEvent()
    data class PreparationDone(val totalCues: Int) : TranslationEvent()
    data class BlockCommitted(val doneCues: Int, val totalCues: Int) : TranslationEvent()
    object PauseRequested : TranslationEvent()
    object ResumeRequested : TranslationEvent()
    data class SystemPaused(val reason: TranslationPauseReason) : TranslationEvent()
    data class SystemReasonCleared(val reason: TranslationPauseReason) : TranslationEvent()
    object StopRequested : TranslationEvent()
    data class Completed(val failedFallbacks: Int) : TranslationEvent()
    data class Failed(val reason: String) : TranslationEvent()
    object SourceInvalidated : TranslationEvent()
    object ProcessInterrupted : TranslationEvent()
}

/// 任务状态（manifest 意图真源的内存映像；journal 不重建意图 —— 评审 F4 分工）。
data class TranslationStateMachineState(
    val taskState: TranslationTaskState = TranslationTaskState.NOT_STARTED,
    val pauseReasons: Set<TranslationPauseReason> = emptySet(),
    val processedCues: Int = 0,
    val totalCues: Int = 0,
    val failedFallbacks: Int = 0,
    val lastError: String? = null,
)

/// reducer 产出：新状态 + 应写入 journal 的事件（空 = 无事件）。
data class TranslationReducerOutput(
    val state: TranslationStateMachineState,
    val journalEvent: String?,
)

/// 十类 journal 事件名（评审 C5；缺失的两类 BLOCK_STARTED/PreparationDone
/// 不产出 —— 进度真源只记「提交与意图」，进行中状态由 manifest 持有）。
object TranslationJournalEvents {
    const val JOB_CREATED = "JOB_CREATED"
    const val BLOCK_COMMITTED = "BLOCK_COMMITTED"
    const val PAUSE_REQUESTED = "PAUSE_REQUESTED"
    const val SYSTEM_PAUSED = "SYSTEM_PAUSED"
    const val RESUME_REQUESTED = "RESUME_REQUESTED"
    const val JOB_STOPPED = "JOB_STOPPED"
    const val JOB_COMPLETED = "JOB_COMPLETED"
    const val JOB_FAILED = "JOB_FAILED"
    const val SOURCE_INVALIDATED = "SOURCE_INVALIDATED"
}

/// 状态机 reducer。**非法转换返回原状态且不产事件**（状态机的自洽性由单测锁定）。
fun reduceTranslation(
    state: TranslationStateMachineState,
    event: TranslationEvent,
): TranslationReducerOutput {
    return when (event) {
        is TranslationEvent.StartRequested -> {
            if (state.taskState != TranslationTaskState.NOT_STARTED &&
                state.taskState != TranslationTaskState.STOPPED &&
                state.taskState != TranslationTaskState.FAILED
            ) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(taskState = TranslationTaskState.PREPARING, lastError = null),
                TranslationJournalEvents.JOB_CREATED,
            )
        }

        is TranslationEvent.PreparationDone -> {
            if (state.taskState != TranslationTaskState.PREPARING) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(
                    taskState = TranslationTaskState.TRANSLATING,
                    totalCues = event.totalCues,
                ),
                null,
            )
        }

        is TranslationEvent.BlockCommitted -> {
            if (state.taskState != TranslationTaskState.TRANSLATING) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(processedCues = event.doneCues, totalCues = event.totalCues),
                TranslationJournalEvents.BLOCK_COMMITTED,
            )
        }

        is TranslationEvent.PauseRequested -> {
            if (state.taskState != TranslationTaskState.TRANSLATING) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(taskState = TranslationTaskState.PAUSED_MANUAL),
                TranslationJournalEvents.PAUSE_REQUESTED,
            )
        }

        is TranslationEvent.ResumeRequested -> {
            // 手动暂停 → 手动恢复；系统暂停不响应手动恢复（评审 G32/C9）。
            if (state.taskState != TranslationTaskState.PAUSED_MANUAL) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(taskState = TranslationTaskState.TRANSLATING),
                TranslationJournalEvents.RESUME_REQUESTED,
            )
        }

        is TranslationEvent.SystemPaused -> {
            // 允许 TRANSLATING 进入暂停，也允许 **PAUSED_SYSTEM 态叠加新原因**
            //（评审 G32：原因集合分别维护，任一存在即暂停——只允许从
            // TRANSLATING 进入会把多原因叠加拒成非法转换，实测抓出）。
            if (state.taskState != TranslationTaskState.TRANSLATING &&
                state.taskState != TranslationTaskState.PAUSED_SYSTEM
            ) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(
                    taskState = TranslationTaskState.PAUSED_SYSTEM,
                    pauseReasons = state.pauseReasons + event.reason,
                ),
                TranslationJournalEvents.SYSTEM_PAUSED,
            )
        }

        is TranslationEvent.SystemReasonCleared -> {
            // 只在系统暂停态清原因；全部解除才恢复（评审 G32：任一存在即暂停）。
            if (state.taskState != TranslationTaskState.PAUSED_SYSTEM ||
                event.reason !in state.pauseReasons
            ) {
                return TranslationReducerOutput(state, null)
            }
            val remaining = state.pauseReasons - event.reason
            return if (remaining.isEmpty()) {
                TranslationReducerOutput(
                    state.copy(
                        taskState = TranslationTaskState.TRANSLATING,
                        pauseReasons = remaining,
                    ),
                    TranslationJournalEvents.RESUME_REQUESTED,
                )
            } else {
                TranslationReducerOutput(state.copy(pauseReasons = remaining), null)
            }
        }

        is TranslationEvent.StopRequested -> {
            // 活动态（翻译中/暂停/中断）都可停止；完成/失败/停止/待开始是终态不响应。
            val active = setOf(
                TranslationTaskState.TRANSLATING,
                TranslationTaskState.PAUSED_MANUAL,
                TranslationTaskState.PAUSED_SYSTEM,
                TranslationTaskState.INTERRUPTED,
            )
            if (state.taskState !in active) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(taskState = TranslationTaskState.STOPPED),
                TranslationJournalEvents.JOB_STOPPED,
            )
        }

        is TranslationEvent.Completed -> {
            if (state.taskState != TranslationTaskState.TRANSLATING) {
                return TranslationReducerOutput(state, null)
            }
            val target = if (event.failedFallbacks > 0) {
                TranslationTaskState.PARTIALLY_DONE
            } else {
                TranslationTaskState.COMPLETED
            }
            TranslationReducerOutput(
                state.copy(
                    taskState = target,
                    failedFallbacks = event.failedFallbacks,
                    processedCues = state.totalCues,
                ),
                TranslationJournalEvents.JOB_COMPLETED,
            )
        }

        is TranslationEvent.Failed -> {
            if (state.taskState != TranslationTaskState.TRANSLATING &&
                state.taskState != TranslationTaskState.PREPARING
            ) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(
                    taskState = TranslationTaskState.FAILED,
                    lastError = event.reason,
                ),
                TranslationJournalEvents.JOB_FAILED,
            )
        }

        is TranslationEvent.SourceInvalidated -> {
            // 源字幕 hash 不匹配 → 旧任务作废（评审 C8/G24 场景表）。
            TranslationReducerOutput(
                state.copy(
                    taskState = TranslationTaskState.FAILED,
                    lastError = "source subtitle changed",
                ),
                TranslationJournalEvents.SOURCE_INVALIDATED,
            )
        }

        is TranslationEvent.ProcessInterrupted -> {
            // 进程被杀后重进：运行/暂停态 → 已中断（**不自动续译**，评审 F4）。
            val recoverable = setOf(
                TranslationTaskState.TRANSLATING,
                TranslationTaskState.PAUSED_MANUAL,
                TranslationTaskState.PAUSED_SYSTEM,
            )
            if (state.taskState !in recoverable) {
                return TranslationReducerOutput(state, null)
            }
            TranslationReducerOutput(
                state.copy(taskState = TranslationTaskState.INTERRUPTED),
                null,
            )
        }
    }
}