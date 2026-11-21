package com.kivo.cinema.subtitle

import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

// ============================================================================
// AiEngineClient —— 引擎访问的**唯一入口**，接口画成 IPC 形状（DEC-13/F16）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/AiEngineClient.kt
//
// 【为什么现在就把接口画成 IPC 形状（评审 F16）】
//   DEC-04/13：正式发布 = :ai 独立进程（发布门槛，v1 不实现）。如果 v1 让
//   Kotlin 直接调 JNI 同步方法，迁移到 :ai 进程时就是重写。本接口的约束：
//   - **全异步**（callback，不阻塞调用线程）；
//   - **请求/响应可序列化**（data class，未来走 Messenger 的 Bundle 不变形）；
//   - **cancel 是独立消息**（不与请求耦合）；
//   - **不暴露 native 句柄**（引擎实例只在实现层内部）。
//
// 【v1 实现 = LocalAiEngineClient】进程内直调 JNI，单翻译线程 executor
//   （线程模型契约：所有 native 调用都走这一个线程 —— C++ 单例 mutex 的
//   调用方约定）。发布态换 Messenger 实现时只换这一个类。
// ============================================================================

/// 块翻译请求（可序列化 —— 发布态经 :ai 进程传输时数据面不变）。
data class AiTranslateRequest(
    val sourceLines: List<String>,
    val useFewshot: Boolean,
    val contextLines: List<String> = emptyList(),
)

/// 块翻译响应。
///
/// 槽位口径（README §5.1）：slots.size() == request.sourceLines.size()；
/// 失败槽位 = 空串（原样回传 C++ 侧约定）。error 非空 = 引擎未就绪/加载失败
/// 的整批失败（slots 为空列表）。
data class AiTranslateResponse(
    val slots: List<String>,
    val error: String?,
)

/// 引擎访问接口。**上层（Controller/Job/UI）只允许从这里进**，
/// 不许直接摸 AiTranslationNative（不暴露 native 句柄，DEC-13）。
interface AiEngineClient {

    /// 加载模型（重复调用 = 替换模型文件并重建引擎）。
    /// 返回 false 时 lastError() 给出原因。
    fun loadModel(modelPath: String): Boolean

    /// 块翻译（异步；结果经 [onResult] 回到调用线程上下文之外的回调线程）。
    fun translateBlock(
        request: AiTranslateRequest,
        onResult: (Result<AiTranslateResponse>) -> Unit,
    )

    /// 任务级取消（v1 偏差：不中断进行中的推理，见 AiTranslationNative 注）。
    fun cancel()

    fun isReady(): Boolean
    fun lastError(): String?
    fun consumedTokens(): Long

    companion object {
        /// 可用性快捷判定（.so 未加载时上层入口直接灰显）。
        fun isNativeAvailable(): Boolean = AiTranslationNative.available
    }
}

/// v1 进程内实现：JNI 直调 + 单翻译线程 executor。
///
/// 线程契约：全部 native 调用都在**同一个单线程 executor** 上执行
/// （C++ 侧单实例 mutex 的调用方约定；回调线程 = 该 executor 线程，
/// UI 更新由调用方自行切主线程）。
class LocalAiEngineClient : AiEngineClient {

    private val executor: ExecutorService = Executors.newSingleThreadExecutor { runnable ->
        Thread(runnable, "kivo-ai-translate").apply {
            // 背景优先级候选之一 —— T-0 矩阵确认 n_threads=4 的核簇行为后定（评审 A2）。
            priority = Thread.NORM_PRIORITY
        }
    }
    private val cancelRequested = AtomicBoolean(false)

    override fun loadModel(modelPath: String): Boolean {
        if (!AiTranslationNative.available) { return false }
        return executor.submit<Boolean> { AiTranslationNative.nativeLoadModel(modelPath) }.get()
    }

    override fun translateBlock(
        request: AiTranslateRequest,
        onResult: (Result<AiTranslateResponse>) -> Unit,
    ) {
        cancelRequested.set(false)
        executor.execute {
            val response: AiTranslateResponse = try {
                val slots = AiTranslationNative.nativeTranslateBlock(
                    request.sourceLines.toTypedArray(),
                    request.useFewshot,
                    request.contextLines.toTypedArray(),
                )
                if (slots != null) {
                    AiTranslateResponse(slots.toList(), null)
                } else {
                    AiTranslateResponse(emptyList(), AiTranslationNative.nativeLastError())
                }
            } catch (throwable: Throwable) {
                AiTranslateResponse(emptyList(), throwable.message ?: "native call failed")
            }
            onResult(Result.success(response))
        }
    }

    override fun cancel() {
        cancelRequested.set(true)
        if (AiTranslationNative.available) {
            executor.execute { AiTranslationNative.nativeCancel() }
        }
    }

    override fun isReady(): Boolean {
        if (!AiTranslationNative.available) { return false }
        return executor.submit<Boolean> { AiTranslationNative.nativeReady() }.get()
    }

    override fun lastError(): String? {
        if (!AiTranslationNative.available) { return "libkivo_ai.so not available" }
        return executor.submit<String?> { AiTranslationNative.nativeLastError() }.get()
    }

    override fun consumedTokens(): Long {
        if (!AiTranslationNative.available) { return 0 }
        return executor.submit<Long> { AiTranslationNative.nativeConsumedTokens() }.get()
    }
}