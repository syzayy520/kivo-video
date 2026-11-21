package com.kivo.cinema.subtitle

import android.content.Context
import java.io.File
import java.security.MessageDigest

// ============================================================================
// TranslationCache —— 译文缓存键与落位（评审 C7/G18/B18）。
//
// 落位路径: android/app/src/main/java/com/kivo/cinema/subtitle/TranslationCache.kt
//
// 【缓存键（评审 C7/G18 的完整口径）】
//   sha256(源字幕内容) + 解析版本 + 目标语言 —— 模型指纹/prompt SHA 属
//   **推理契约指纹**（DEC-16），v1 由 manifest 持有（模型替换 → manifest
//   校验源不匹配 → 提示重翻，评审 G24 场景表）；键的完整矩阵随可信清单
//   落地（UX §8 发布门槛）。
//
// 【译文落位】filesDir/ai_subs/<cacheKey>.zh.srt —— filesDir 不被系统清理
//   （README §5.2 的 cacheDir 口径已被 v3.1 推翻，评审 C5/B15）。
//
// 【活动文件保护】清空缓存时跳过「当前正在使用的译文」（调用方传入活动键）。
// ============================================================================

object TranslationCache {

    /// 解析版本（cue 归一/切块规则升级 → 旧缓存作废，评审 G18）。
    const val PARSER_VERSION = "cue-assembler-v3.2"

    const val TARGET_LANGUAGE = "zh-CN"

    /// 缓存键：sha256(源字幕) + 解析版本 + 目标语言。
    fun cacheKey(sourceSrtUtf8: String): String {
        val digest = MessageDigest.getInstance("SHA-256")
            .digest(sourceSrtUtf8.toByteArray(Charsets.UTF_8))
        val hex = digest.joinToString("") { "%02x".format(it) }
        return "$hex-$PARSER_VERSION-$TARGET_LANGUAGE"
    }

    /// 译文文件（落 filesDir/ai_subs/ —— 不被系统清理，防重翻）。
    fun translationFile(context: Context, cacheKey: String): File {
        val dir = File(context.filesDir, "ai_subs")
        dir.mkdirs()
        return File(dir, "$cacheKey.srt")
    }

    /// 缓存命中：译文文件已存在且非空。
    fun hasTranslation(context: Context, cacheKey: String): Boolean {
        val file = translationFile(context, cacheKey)
        return file.isFile && file.length() > 0
    }

    /// 目录占用（字节）—— 设置页「翻译缓存」行的数据源（UX §7 分层）。
    fun cacheSizeBytes(context: Context): Long {
        val dir = File(context.filesDir, "ai_subs")
        return dir.listFiles()?.sumOf { it.length() } ?: 0L
    }

    /// 清空缓存。`activeKey` = 当前正在使用的译文键 —— **活动文件保护**
    ///（评审 G：清理不影响正在使用的字幕）。
    fun clear(context: Context, activeKey: String?): Int {
        val dir = File(context.filesDir, "ai_subs")
        var removed = 0
        dir.listFiles()?.forEach { file ->
            if (activeKey != null && file.name == "$activeKey.srt") {
                return@forEach   // 活动文件保护
            }
            if (file.delete()) { ++removed }
        }
        return removed
    }

    /// SHA-256 hex（源字幕内容 hash —— 键的根基）。
    fun sha256Hex(content: String): String =
        MessageDigest.getInstance("SHA-256")
            .digest(content.toByteArray(Charsets.UTF_8))
            .joinToString("") { "%02x".format(it) }
}