package com.kivo.cinema.subtitle.remote

import com.kivo.cinema.subtitle.AiEngineClient
import com.kivo.cinema.subtitle.AiTranslateRequest
import com.kivo.cinema.subtitle.AiTranslateResponse
import com.kivo.cinema.tmdb.TmdbClient
import org.json.JSONArray
import java.net.HttpURLConnection
import java.net.InetSocketAddress
import java.net.Proxy
import java.net.URL
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

/**
 * 在线 AI 翻译引擎（design.md ai-online-2026-11-25 §四）。
 *
 * 实现 [AiEngineClient]——接口当初按 IPC 形状设计（全异步、可序列化、不暴露 native
 * 句柄），远程实现原样插上；TranslationJob 的切块/缺号重试/缓存/落盘零改动复用。
 *
 * 线程契约：与本地引擎同款——全部网络调用走自己的单线程 executor（串行块）。
 * isReady = 配置完整（key/model/baseUrl）；loadModel = no-op（远程无模型加载）。
 * 失败不静默回退：整块失败经 error 字段透传中文 reason（G9）。
 */
class RemoteAiEngineClient(private val context: android.content.Context) : AiEngineClient {

    private val executor: ExecutorService =
        Executors.newSingleThreadExecutor { r -> Thread(r, "kivo-ai-remote") }
    private val cancelled = AtomicBoolean(false)

    private val prefs = RemoteEnginePrefs(context)

    override fun loadModel(modelPath: String): Boolean = true

    /// HTTP 引擎无 native 单线程约束——TranslationJob 按 4 路预取窗口流水线化
    ///（2026-11-25 裸 API 实测 3.4s/块，串行下 234 条仍要 ~5 分钟，并发后 ~1 分钟）。
    override fun supportsParallelBlocks(): Boolean = true

    override fun translateBlock(
        request: AiTranslateRequest,
        onResult: (Result<AiTranslateResponse>) -> Unit,
    ) {
        cancelled.set(false)
        executor.execute {
            val configError = prefs.configError()
            if (configError != null) {
                onResult(Result.success(AiTranslateResponse(emptyList(), configError)))
                return@execute
            }
            val block = RemoteEngineProtocols.Block(
                model = prefs.effectiveModel() ?: "",
                lines = request.sourceLines,
                contextLines = request.contextLines,
            )
            val outcome = executeBlock(block)
            onResult(Result.success(outcome))
        }
    }

    private fun executeBlock(block: RemoteEngineProtocols.Block): AiTranslateResponse {
        val baseUrl = prefs.effectiveBaseUrl() ?: return AiTranslateResponse(emptyList(), "接口地址未配置")
        val anthropic = prefs.provider.protocol == RemoteProtocol.ANTHROPIC
        val body = if (anthropic) {
            RemoteEngineProtocols.buildAnthropicBody(block)
        } else {
            RemoteEngineProtocols.buildOpenAiBody(block)
        }
        val endpoint = if (anthropic) {
            "${baseUrl.trimEnd('/')}/v1/messages"
        } else {
            "${baseUrl.trimEnd('/')}/chat/completions"
        }
        return try {
            val connection = openConnection(endpoint)
            try {
                connection.requestMethod = "POST"
                connection.connectTimeout = CONNECT_TIMEOUT_MS
                connection.readTimeout = READ_TIMEOUT_MS
                connection.doOutput = true
                connection.setRequestProperty("Content-Type", "application/json")
                if (anthropic) {
                    connection.setRequestProperty("x-api-key", prefs.apiKey)
                    connection.setRequestProperty("anthropic-version", "2023-06-01")
                } else {
                    connection.setRequestProperty("Authorization", "Bearer ${prefs.apiKey}")
                }
                if (prefs.provider == RemoteProvider.OPENROUTER) {
                    connection.setRequestProperty("X-Title", "KivoCinema")
                }
                connection.outputStream.use { it.write(body.toByteArray(Charsets.UTF_8)) }
                val code = connection.responseCode
                val responseBody = if (code in 200..299) {
                    connection.inputStream.readBytes().decodeToString()
                } else {
                    val err = connection.errorStream?.readBytes()?.decodeToString().orEmpty()
                    return AiTranslateResponse(emptyList(), failureReason(code, err))
                }
                if (cancelled.get()) { return AiTranslateResponse(emptyList(), "翻译已停止") }
                val content = if (anthropic) {
                    RemoteEngineProtocols.parseAnthropicContent(responseBody)
                } else {
                    RemoteEngineProtocols.parseOpenAiContent(responseBody)
                }
                val parsed = RemoteEngineProtocols.parseSlots(content, block.lines.size)
                if (parsed.error != null) {
                    AiTranslateResponse(emptyList(), "服务商返回内容无法解析（${parsed.error}），请重试或更换模型")
                } else {
                    AiTranslateResponse(parsed.slots.map { it.orEmpty() }, null)
                }
            } finally {
                connection.disconnect()
            }
        } catch (e: Exception) {
            AiTranslateResponse(emptyList(), networkReason(e))
        }
    }

    /// 测试连接：GET /models（Anthropic /v1/models）→ 成功返回模型条数（设置页用）。
    fun testConnection(): Result<Int> {
        val baseUrl = prefs.effectiveBaseUrl()
            ?: return Result.failure(IllegalStateException("自定义档位需要填写接口地址"))
        val anthropic = prefs.provider.protocol == RemoteProtocol.ANTHROPIC
        val endpoint = if (anthropic) "${baseUrl.trimEnd('/')}/v1/models" else "${baseUrl.trimEnd('/')}/models"
        return try {
            val connection = openConnection(endpoint)
            try {
                if (anthropic) {
                    connection.setRequestProperty("x-api-key", prefs.apiKey)
                    connection.setRequestProperty("anthropic-version", "2023-06-01")
                } else {
                    connection.setRequestProperty("Authorization", "Bearer ${prefs.apiKey}")
                }
                connection.connectTimeout = CONNECT_TIMEOUT_MS
                connection.readTimeout = READ_TIMEOUT_MS
                val code = connection.responseCode
                if (code !in 200..299) {
                    val err = connection.errorStream?.readBytes()?.decodeToString().orEmpty()
                    return Result.failure(IllegalStateException(failureReason(code, err)))
                }
                val data = JSONArray(
                    org.json.JSONObject(connection.inputStream.readBytes().decodeToString()).getString("data"),
                )
                Result.success(data.length())
            } finally {
                connection.disconnect()
            }
        } catch (e: Exception) {
            Result.failure(IllegalStateException(networkReason(e)))
        }
    }

    override fun cancel() { cancelled.set(true) }

    override fun isReady(): Boolean = prefs.configError() == null

    override fun lastError(): String? = prefs.configError()

    override fun consumedTokens(): Long = 0

    /** 统一出网口：复用 tmdb_proxy.txt 的 SOCKS 配置（用户配一次代理全 App 生效）。 */
    private fun openConnection(urlStr: String): HttpURLConnection {
        val url = URL(urlStr)
        val proxied = TmdbClient(context).proxyAddr()?.let { addr ->
            val host = addr.substringBeforeLast(':')
            val port = addr.substringAfterLast(':').toIntOrNull() ?: 10808
            Proxy(Proxy.Type.SOCKS, InetSocketAddress(host, port))
        }
        return (proxied?.let { url.openConnection(it) } ?: url.openConnection()) as HttpURLConnection
    }

    private fun failureReason(code: Int, body: String): String {
        val lower = body.lowercase()
        return when {
            code == 401 || code == 403 -> "接口密钥无效或未授权，请在设置中检查 API Key"
            lower.contains("insufficient_quota") || lower.contains("quota") ||
                lower.contains("balance") -> "服务商额度不足，请检查账户余额或稍后重试"
            code == 429 -> "服务商限流中，请稍后重试"
            lower.contains("model") -> "模型名不可用：${prefs.effectiveModel().orEmpty()}，请在设置中修正"
            else -> "服务商返回 HTTP $code"
        }
    }

    private fun networkReason(e: Exception): String {
        val detail = e.message?.takeIf { it.isNotBlank() }?.take(60) ?: e.javaClass.simpleName
        return "无法连接服务商（网络不可达或超时：$detail）"
    }

    private companion object {
        const val CONNECT_TIMEOUT_MS = 10_000
        const val READ_TIMEOUT_MS = 120_000
    }
}