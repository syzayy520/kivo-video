package com.kivo.cinema.subtitle

// ============================================================================
// AI 翻译引擎的 JNI 声明（libkivo_ai.so）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/AiTranslationNative.kt
//
// 【懒加载 + 降级（DEC-13 加载期隔离）】
//   System.loadLibrary 包 try/catch —— .so 缺失/ABI 不符/符号缺失
//   （UnsatisfiedLinkError）只降级 AI 功能，绝不影响 kivo_player。
//   available=false 时上层入口灰显并给 reason（G9）。
//
// 【槽位口径（README §5.1）】nativeTranslateBlock 返回数组长度必须等于
//   sourceLines.size()；失败槽位 = 空串（C++ 侧已保证不返回 null 元素）。
//   返回 null = 引擎未就绪，原因走 nativeLastError()。
//
// 【线程模型（v1 偏差如实登记）】nativeTranslateBlock 是同步阻塞调用
//   （单翻译线程由 AiEngineClient 的 executor 保证）；nativeCancel 在同步
//   调用期间不中断推理（decode 内中断列 T-B 后续）—— 评审 G32 的
//   「1 秒内响应」在 v1 阶段做不到。
// ============================================================================

object AiTranslationNative {

    /// libkivo_ai.so 是否可用（懒加载；失败只降级 AI）。
    val available: Boolean by lazy {
        try {
            System.loadLibrary("kivo_ai")
            true
        } catch (error: UnsatisfiedLinkError) {
            false
        } catch (error: SecurityException) {
            false
        }
    }

    /// 加载模型（mmap）。返回 false 时原因见 nativeLastError()。
    external fun nativeLoadModel(modelPath: String): Boolean

    /// SRT 文本 → 归一 cue 平铺数组（C++ cue_assembler 权威实现，避免 Kotlin
    /// 重复解析的偏差累积）。每个元素 = "<startMs>\t<endMs>\t<text>"（单行）。
    external fun nativeParseCues(srtUtf8: String): Array<String>?

    /// 块翻译：返回数组长度 == sourceLines.size()；失败槽位 = 空串。
    /// 返回 null = 引擎未就绪。
    external fun nativeTranslateBlock(
        sourceLines: Array<String>,
        useFewshot: Boolean,
        contextLines: Array<String>,
    ): Array<String>?

    /// 任务级取消（v1 偏差：不中断进行中的推理）。
    external fun nativeCancel()

    /// 引擎是否已就绪（模型加载成功）。
    external fun nativeReady(): Boolean

    /// 最近失败原因（ready=false 或槽位失败时的 reason 来源）。
    external fun nativeLastError(): String?

    /// 累计 token 用量（本地推理也有成本：电量、发热 —— SettingsAiSub 数据源）。
    external fun nativeConsumedTokens(): Long
}