package com.kivo.cinema.subtitle.remote

import android.content.Context

/**
 * 在线 AI 翻译引擎的配置层（design.md ai-online-2026-11-25 §四）。
 *
 * 覆盖面 = SenPlayer 预置 5 端点（逆向实证）+ OpenRouter + 自定义：
 * 国内 SiliconFlow/DeepSeek、海外 OpenAI/Gemini/OpenRouter/Anthropic。
 * 预置档位只提供 base_url 与默认模型——API Key 一律用户自填（SenPlayer 同款，不内置）。
 */
enum class RemoteProtocol { OPENAI_COMPATIBLE, ANTHROPIC }

enum class RemoteProvider(
    val label: String,
    val baseUrl: String,
    val defaultModel: String,
    val protocol: RemoteProtocol,
) {
    // 预置模型 2026-11-25 裸 API 实测三轮（PC 端同 key 计时）：
    // ① Hunyuan-MT-7B 不遵 JSON（纯译文直出）② Qwen3.5-9B 思考反噬（75.8s/块）+
    // chat_template_kwargs 关思考失败 ③ GLM-5.3-Flash 6s+ 思考关不掉
    // ④ **DeepSeek-V4.1-Flash 3.4s/块 JSON ✓** → 默认取它（¥2/¥8，2003 条约 ¥0.5）。
    SILICONFLOW("SiliconFlow", "https://api.siliconflow.cn/v1", "deepseek-ai/DeepSeek-V4.1-Flash", RemoteProtocol.OPENAI_COMPATIBLE),
    DEEPSEEK("DeepSeek", "https://api.deepseek.com/v1", "deepseek-flash", RemoteProtocol.OPENAI_COMPATIBLE),
    OPENAI("OpenAI", "https://api.openai.com/v1", "gpt-4o-mini", RemoteProtocol.OPENAI_COMPATIBLE),
    GEMINI("Gemini", "https://generativelanguage.googleapis.com/v1beta/openai", "gemini-3.5-flash-lite", RemoteProtocol.OPENAI_COMPATIBLE),
    OPENROUTER("OpenRouter", "https://openrouter.ai/api/v1", "", RemoteProtocol.OPENAI_COMPATIBLE),
    ANTHROPIC("Anthropic", "https://api.anthropic.com", "claude-3-5-haiku-latest", RemoteProtocol.ANTHROPIC),
    CUSTOM("自定义", "", "", RemoteProtocol.OPENAI_COMPATIBLE),
}

/** 在线引擎配置（SharedPreferences「remote_ai_engine」；key 不回显明文）。 */
class RemoteEnginePrefs(context: Context) {
    private val prefs = context.getSharedPreferences("remote_ai_engine", Context.MODE_PRIVATE)

    /// true = 翻译走在线接口；false = 本地模型（默认——不花钱、无隐私外发）。
    var useRemote: Boolean
        get() = prefs.getBoolean("use_remote", false)
        set(v) = prefs.edit().putBoolean("use_remote", v).apply()

    var provider: RemoteProvider
        get() = runCatching {
            RemoteProvider.valueOf(prefs.getString("provider", RemoteProvider.SILICONFLOW.name) ?: "")
        }.getOrDefault(RemoteProvider.SILICONFLOW)
        set(v) = prefs.edit().putString("provider", v.name).apply()

    var apiKey: String
        get() = prefs.getString("api_key", "") ?: ""
        set(v) = prefs.edit().putString("api_key", v).apply()

    /// 自定义档位的接口地址；预置档位忽略此值。
    var customBaseUrl: String
        get() = prefs.getString("custom_base_url", "") ?: ""
        set(v) = prefs.edit().putString("custom_base_url", v).apply()

    /// 模型名；空 = 档位默认模型。
    var model: String
        get() = prefs.getString("model", "") ?: ""
        set(v) = prefs.edit().putString("model", v).apply()

    fun effectiveBaseUrl(): String? = when (provider) {
        RemoteProvider.CUSTOM -> customBaseUrl.trim().takeIf { it.isNotBlank() }
        else -> provider.baseUrl
    }

    fun effectiveModel(): String? = when (provider) {
        RemoteProvider.CUSTOM -> model.trim().takeIf { it.isNotBlank() }
        else -> model.trim().takeIf { it.isNotBlank() } ?: provider.defaultModel.takeIf { it.isNotBlank() }
    }

    /// 配置完整性校验（G9）：null = 就绪；非空 = 用户可见中文原因。
    fun configError(): String? = when {
        effectiveBaseUrl() == null -> "自定义档位需要填写接口地址"
        effectiveModel() == null -> "请填写模型名"
        apiKey.isBlank() -> "请填写 API Key"
        else -> null
    }
}