package com.kivo.cinema.subtitle

import android.content.Context
import java.io.File
import java.io.RandomAccessFile

// ============================================================================
// AiSubtitleModelStore —— 模型文件的发现与校验（DEC-16，评审 E2/G2）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/AiSubtitleModelStore.kt
//
// 【口径（v1 开发者模式，DEC-02）】
//   - 目录：getExternalFilesDir(null)/models/（adb push 可写；**随卸载删除**，
//     Android 11+ 第三方文件管理器进不去 —— 非开发者装模型需应用内安装流程，
//     那是发布门槛不是 v1 范围，UX 规范 §8 北极星已立）；
//   - 校验：文件头 4 字节 = "GGUF"（魔数）+ 大小 ≥ 500MB（1.7B Q4 的下限）。
//     **只认 GGUF** —— GGJT 是弃用旧格式，放行 = 校验过、加载必败
//     （评审 E2 指出的 v3.1 笔误）；
//   - 【DEC-16 口径】SHA256 全量摘要属「安装期一次性校验 + 可信发布清单比对」
//     —— v1 先以魔数+大小做**身份初筛**，可信清单（含 SHA256/tokenizer 版本/
//     设备档位）随发布门槛落地（UX §8），届时缓存键用可信清单摘要。
//
// 【发现规则】目录下**首个**通过校验的 .gguf（多模型并存的选择 UI 列二期，
//   UX §6 语言范围表；当前唯一授权模型 = Qwen3-1.7B-Q4_K_M）。
// ============================================================================

object AiSubtitleModelStore {

    private const val GGUF_MAGIC = "GGUF"
    private const val MIN_MODEL_BYTES = 500L * 1024 * 1024   // 1.7B Q4 ≈ 1.1GB 的下限
    private const val EXPECTED_MODEL_BYTES = 1_282_439_584L  // 设备上 1.7B 的实测大小

    /// 模型目录（外部私有 —— adb push 可写；卸载随删，如实交底见 UX §8）。
    fun modelDir(context: Context): File {
        val base = context.getExternalFilesDir(null)
            ?: File(context.filesDir, "models")   // 外部存储不可用时的兜底（罕见）
        return File(base, "models")
    }

    /// 发现并校验模型。返回通过校验的文件；null = 缺模型/校验失败。
    fun findModel(context: Context): File? {
        val dir = modelDir(context)
        if (!dir.isDirectory) { return null }
        val candidates = dir.listFiles { file -> file.isFile && file.name.endsWith(".gguf") }
            ?: return null
        return candidates.firstOrNull { passesIdentityCheck(it) }
    }

    /// 身份初筛：GGUF 魔数 + 大小下限。失败原因见 [identityFailureReason]。
    fun passesIdentityCheck(file: File): Boolean = identityFailureReason(file) == null

    /// 校验失败的具体原因（入口态/设置页提示的依据，G9 非空 reason）。
    fun identityFailureReason(file: File): String? {
        if (!file.isFile) { return "not a file" }
        if (file.length() < MIN_MODEL_BYTES) { return "file too small for a known model" }
        RandomAccessFile(file, "r").use { handle ->
            val magic = ByteArray(4)
            if (handle.read(magic) != 4) { return "file shorter than GGUF magic" }
            val actual = String(magic, Charsets.US_ASCII)
            // 只认 GGUF：GGJT 是弃用旧格式，放行 = 校验过、加载必败（评审 E2）。
            if (actual != GGUF_MAGIC) { return "magic is $actual, expected GGUF" }
        }
        if (file.length() > EXPECTED_MODEL_BYTES * 2) { return "file larger than known model bound" }
        return null
    }

    /// 设置页/入口的提示文案要素（未找到模型时给路径指引，评审 G「入口四态」）。
    fun guidance(context: Context): String {
        return "put model gguf into ${modelDir(context).absolutePath}"
    }
}