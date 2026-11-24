package com.kivo.cinema.subtitle

import android.content.Context
import com.kivo.cinema.subtitle.remote.RemoteAiEngineClient
import com.kivo.cinema.subtitle.remote.RemoteEnginePrefs
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

    private val client: AiEngineClient = localClient
    private val runningJob = AtomicReference<TranslationJob?>(null)

    companion object {
        /// 本地引擎 client（进程级共享：所有 native 调用走同一个单线程 executor，
        /// C++ 单例 mutex 的调用方约定；第二个 client 的 loadModel 会 g_engine.reset()
        /// 销毁正在用的引擎——2026-11-21 冒烟实测）。
        private val localClient: AiEngineClient by lazy { LocalAiEngineClient() }

        /// 远程引擎 client（同样进程级单例——单线程 executor 的线程契约）。
        @Volatile
        private var remoteClient: RemoteAiEngineClient? = null

        /// 引擎选择（design.md ai-online-2026-11-25 §四）：按配置返回本地/远程单例。
        /// 切换即时生效（下一次翻译起）；两实现共享同一 AiEngineClient 形状。
        @Synchronized
        fun engineClient(context: Context): AiEngineClient {
            val prefs = RemoteEnginePrefs(context)
            return if (prefs.useRemote) {
                remoteClient ?: RemoteAiEngineClient(context.applicationContext).also { remoteClient = it }
            } else {
                localClient
            }
        }

        /// 共享装载：远程引擎 = 配置完整即就绪；本地 = isReady → ModelStore → loadModel。
        /// 返回 null = 就绪；非空 = 用户可见中文原因（G9）。
        /// 本地未就绪时才 loadModel —— 此时没有「正在用的引擎」，重建无害。
        internal fun ensureModelLoadedReason(context: Context): String? {
            val remote = RemoteEnginePrefs(context)
            if (remote.useRemote) { return remote.configError() }
            if (!AiTranslationNative.available) { return "AI 翻译库不可用" }
            if (localClient.isReady()) { return null }
            val model = AiSubtitleModelStore.findModel(context)
                ?: return "未找到翻译模型：${AiSubtitleModelStore.guidance(context)}"
            return if (localClient.loadModel(model.absolutePath)) {
                null
            } else {
                "模型加载失败：${localClient.lastError() ?: "未知原因"}"
            }
        }
    }

    /// 面板行投影用：运行中任务的进度快照（TRANSLATING 态副行数据源，UX v3 §3）。
    /// onProgress 更新、worker finally 清空——终态由 lastTranslationPath（缓存命中）表达。
    @Volatile var currentProgress: TranslationJobProgress? = null
        private set
    private val stateMachine = TranslationStateMachineState()

    /// libkivo_ai.so 是否可用（不可用时入口灰显，reason 固定）。
    fun isNativeAvailable(): Boolean = AiTranslationNative.available

    /// 引擎是否就绪（本地 = 模型已加载；远程 = 配置完整）。
    fun isEngineReady(): Boolean {
        val remote = RemoteEnginePrefs(context)
        if (remote.useRemote) { return remote.configError() == null }
        return AiTranslationNative.available && client.isReady()
    }

    /// 入口态解析（UX v3 §3：功能开关由调用方传入——设置页总开关）。
    fun entryState(
        featureEnabled: Boolean,
        hasEnglishSrt: Boolean,
        subtitleIssue: SubtitleIssue,
    ): AiEntryState {
        val remotePrefs = RemoteEnginePrefs(context)
        if (remotePrefs.useRemote) {
            // 远程引擎不依赖 libkivo_ai.so 与本地模型——入口态只看配置完整性。
            return if (remotePrefs.configError() == null) {
                resolveAiEntryState(
                    AiEntryConditions(
                        featureEnabled = featureEnabled,
                        taskState = if (runningJob.get() != null) TranslationTaskState.TRANSLATING else null,
                        cacheHit = lastTranslationPath != null,
                        subtitleIssue = subtitleIssue,
                        modelIssue = ModelIssue.NONE,
                    )
                )
            } else {
                resolveAiEntryState(
                    AiEntryConditions(
                        featureEnabled = featureEnabled,
                        taskState = null,
                        cacheHit = false,
                        subtitleIssue = subtitleIssue,
                        modelIssue = ModelIssue.MISSING,
                    )
                )
            }
        }
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
    fun ensureModelLoaded(): Boolean = ensureModelLoadedReason(context) == null

    /// 诊断双写：logcat + 外部 ai-smoke.log。此前「ai-smoke.log 无 done 行」的
    /// 盲区源于面板路径不写外部日志（只有探针写）；logcat 主缓冲在 mpv 日志
    /// 冲刷下不可靠（AI-DEVICE §7 前科），诊断以文件为准。
    private fun diag(msg: String) {
        android.util.Log.i("kivo-ai", msg)
        runCatching {
            java.io.File(context.getExternalFilesDir(null), "ai-smoke.log")
                .appendText("${System.currentTimeMillis()} $msg\n")
        }
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

        val job = TranslationJob(engineClient(context), context) { progress ->
            currentProgress = progress
            onProgress?.invoke(progress)
        }
        runningJob.set(job)
        Thread {
            diag("ai-ctrl worker-started")
            val result = try {
                val loadReason = ensureModelLoadedReason(context)
                diag("ai-ctrl ensure-loaded=${loadReason == null}")
                if (loadReason != null) {
                    // 模型加载失败必须明确终止——此前返回值被丢弃，job.run 会在
                    // 引擎未就绪下继续跑，真正的失败原因被推迟且掩埋。
                    TranslationJobResult(null, 0, 0, 0, 0, 0, cancelled = false, reason = loadReason)
                } else {
                    diag("ai-ctrl before-job-run")
                    job.run(sourceSrt, playheadMs).also {
                        diag("ai-ctrl after-job-run path=${it.translationPath ?: "null"}")
                    }
                }
            } catch (throwable: Throwable) {
                // 完整堆栈双写——此前 catch 静默吞掉（G9 注释空转），worker 内
                // 任何异常都只剩 2 秒 toast，根因无从取证。
                diag("ai-ctrl worker-catch ${throwable.javaClass.name}: ${throwable.message}\n${android.util.Log.getStackTraceString(throwable)}")
                TranslationJobResult(
                    null, 0, 0, 0, 0, 0, cancelled = false,
                    reason = "翻译任务异常：${throwable.message ?: throwable.javaClass.simpleName}",
                )
            } finally {
                runningJob.set(null)
                currentProgress = null
            }
            lastTranslationPath = result.translationPath ?: lastTranslationPath
            diag("ai-ctrl onDone-enter path=${result.translationPath ?: "null"}")
            onDone?.invoke(result)
        }.start()
        return true
    }

    /// 停止当前任务（关闭中文字幕的底层动作；已译内容保留可续）。
    fun stopTranslation() {
        runningJob.get()?.cancel()
    }
}