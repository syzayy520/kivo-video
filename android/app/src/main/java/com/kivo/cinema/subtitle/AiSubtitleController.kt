package com.kivo.cinema.subtitle

import android.content.Context
import java.util.concurrent.atomic.AtomicReference

// ============================================================================
// AiSubtitleController —— AI 字幕翻译的门面（UI 的唯一入口，DEC-13）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/AiSubtitleController.kt
//
// 职责：组合 AiEngineClient + AiSubtitleModelStore + TranslationJob，
// 把「入口态解析 / 模型加载 / 任务启动 / 取消」收拢成一个对象；
// 上层（字幕面板/设置页）**不许直接摸** AiEngineClient 或 AiTranslationNative。
//
// 线程模型：startTranslation 内部起独立线程跑 job.run（与 importSubtitleFile
// 的 Thread{} 模式一致）；回调切回主线程由调用方处理（importSubtitleFile 同款）。
//
// 单任务：同一时间只有一个翻译任务（进行中再来 → 拒绝并报进度，
// 评审 G8/B12——不排队不并发）。
// ============================================================================

class AiSubtitleController(private val context: Context) {

    private val client: AiEngineClient = LocalAiEngineClient()
    private val runningJob = AtomicReference<TranslationJob?>(null)
    private val stateMachine = TranslationStateMachineState()

    /// libkivo_ai.so 是否可用（不可用时入口灰显，reason 固定）。
    fun isNativeAvailable(): Boolean = AiTranslationNative.available

    /// 引擎是否就绪（模型已加载）。
    fun isEngineReady(): Boolean = AiTranslationNative.available && client.isReady()

    /// 入口态解析（UX v3 §3：功能开关由调用方传入——设置页总开关）。
    fun entryState(
        featureEnabled: Boolean,
        hasEnglishSrt: Boolean,
        subtitleIssue: SubtitleIssue,
    ): AiEntryState {
        if (!AiTranslationNative.available) { return AiEntryState.FEATURE_DISABLED }
        val modelIssue = when {
            isEngineReady() -> ModelIssue.NONE
            AiSubtitleModelStore.findModel(context) != null -> ModelIssue.NONE
            else -> ModelIssue.MISSING
        }
        val taskState = if (runningJob.get() != null) {
            TranslationTaskState.TRANSLATING
        } else {
            null
        }
        val cacheHit = lastTranslationPath != null
        return resolveAiEntryState(
            AiEntryConditions(
                featureEnabled = featureEnabled,
                taskState = taskState,
                cacheHit = cacheHit,
                subtitleIssue = subtitleIssue,
                modelIssue = modelIssue,
            )
        )
    }

    /// 模型加载：ModelStore 发现 → client.loadModel。返回 false 时
    /// lastError()/guidance() 给出原因与路径指引（G9 非空 reason）。
    fun ensureModelLoaded(): Boolean {
        if (client.isReady()) { return true }
        val model = AiSubtitleModelStore.findModel(context) ?: return false
        return client.loadModel(model.absolutePath)
    }

    /// 模型缺失时的路径指引（入口态副行文案，评审 G 入口四态）。
    fun modelGuidance(): String = AiSubtitleModelStore.guidance(context)

    /// 最近一次译文路径（缓存命中判定 / 二次观看挂载）。
    var lastTranslationPath: String? = null
        private set

    /// 启动翻译任务（单任务；进行中调用返回 false 并报进度）。
    ///
    /// @param sourceSrt 英文 SRT 全文（导入链已归一 UTF-8）。
    /// @param playheadMs 当前播放头（领先窗口 Preview 起点）。
    /// @param onProgress 进度回调（executor 线程——UI 更新自行切主线程）。
    /// @param onDone 完成回调（主流程结束；翻译路径在 result.translationPath）。
    /// @return false = 已有任务在跑。
    fun startTranslation(
        sourceSrt: String,
        playheadMs: Long,
        onProgress: ((TranslationJobProgress) -> Unit)?,
        onDone: ((TranslationJobResult) -> Unit)?,
    ): Boolean {
        if (runningJob.get() != null) { return false }

        val job = TranslationJob(client, context) { progress ->
            onProgress?.invoke(progress)
        }
        runningJob.set(job)
        Thread {
            val result = try {
                ensureModelLoaded()
                job.run(sourceSrt, playheadMs)
            } catch (throwable: Throwable) {
                TranslationJobResult(null, 0, 0, 0, 0, 0, cancelled = false).also {
                    // 异常路径也要给非空 reason（G9），然后回 job 的失败态。
                }
            } finally {
                runningJob.set(null)
            }
            lastTranslationPath = result.translationPath ?: lastTranslationPath
            onDone?.invoke(result)
        }.start()
        return true
    }

    /// 停止当前任务（关闭中文字幕的底层动作；已译内容保留可续）。
    fun stopTranslation() {
        runningJob.get()?.cancel()
    }
}