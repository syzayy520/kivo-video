package com.kivo.cinema.playback

import android.content.Context
import android.content.Intent

/**
 * 354 · **播放输入通道的唯一决策点**。
 *
 * ## 它解决什么
 *
 * 在此之前，"这场播放怎么把字节交给 mpv"只有一种答案：全部塞进 loopback HTTP 代理
 * （`LocalRangeServer` → `http://127.0.0.1:<port>/v/<id>`）。统一抽象换来的是
 * **每类源都被削平到最低公共分母**，对本地文件纯属损失：
 *
 * | 损失 | 机制 |
 * | --- | --- |
 * | 同名字幕失效 | mpv 的 `sub-auto` 只看 URL 所在目录，代理 URL 是 `/v/<id>` ⇒ 看不到兄弟文件 |
 * | 格式识别受响应头支配 | 代理必须回 `Content-Type`，回错 demuxer 直接死（D12I：写死 matroska ⇒ 全部 `.ts` 播不了） |
 * | 多一次 socket + 一次全量拷贝 | 文件 → byte[] → socket → mpv → demuxer |
 *
 * 而 mpv 自己对本地文件有更好的通道：`loadfile <绝对路径>`，走 ffmpeg 的 file 协议，
 * `sub-auto` / 内嵌字体附件 / 原生 seek 全部可用。
 *
 * ## 为什么单独立文件（而不是在起播处写 if）
 *
 * 项目已有三个同构决策点（[PlaybackEngine] / [PlaybackRenderMode] / [HdrOutputSwitch]），
 * 由 [PlaybackStack] 聚合。输入通道是同一层的第四个维度 ⇒ 必须同构，
 * 否则就回到「两个地方各算一遍」——280 已经为这个错误付过代价
 * （HDR Surface 8bit/10bit 打架，见 [PlaybackStack] 文件头）。
 *
 * ## 职责边界
 *
 * 本文件**只决定走哪条通道**，不做预检、不碰文件系统。
 * 路径能否真的读出来由 [DirectFileMedia] 判 —— 决策与事实分开，
 * 这样"决策说直连但预检失败"能被如实记录（而不是被当成决策错误）。
 */
internal object PlaybackInputMode {

    /** 本地文件直连（mpv 直接读路径，不经代理）。 */
    const val DIRECT_FILE = "direct"

    /**
     * 经 loopback HTTP 代理（SMB / HTTP / 预览内核，以及本地文件的回退）。
     *
     * 【取值就是 `proxy`】开关与日志都用这个词 —— 与 354 文档一致。
     * （首版写成 `loopback` 但开关文档写 `proxy`，`when` 不匹配 ⇒ 强制回退**静默失效**，
     * 真机 `setprop debug.kivo.inputMode proxy` 后仍走直连，才发现这个错。）
     */
    const val LOOPBACK = "proxy"

    /** 兼容旧写法/手误：`loopback` 等价于 [LOOPBACK]。 */
    private const val LOOPBACK_ALIAS = "loopback"

    /** 自动：有本地真实路径就直连，其余走代理。 */
    const val AUTO = "auto"

    /**
     * 一场播放的通道决策。
     *
     * @param mode [DIRECT_FILE] 或 [LOOPBACK]
     * @param reason 为什么是这个结果（**必须能写进日志**，禁止静默切换）
     */
    class Decision(val mode: String, val reason: String) {
        val isDirect: Boolean get() = mode == DIRECT_FILE
        fun describe(): String = "mode=$mode（$reason）"
    }

    /**
     * 读系统属性（与 [PlaybackRenderMode] 同一套取证/回退惯例）。
     *
     * 【为什么反射】`android.os.SystemProperties` 是隐藏 API。拿不到就当没设 ——
     * 拿不到不会出错，拿到就能在出问题时即时回退，不用重装。
     */
    private fun sysProp(name: String): String? = try {
        val c = Class.forName("android.os.SystemProperties")
        val m = c.getMethod("get", String::class.java, String::class.java)
        (m.invoke(null, name, "") as? String)?.trim()?.takeIf { it.isNotEmpty() }
    } catch (_: Throwable) {
        null
    }

    /**
     * 决策。
     *
     * 优先级：系统属性 `debug.kivo.inputMode` > intent `kivo.inputMode` > 默认 auto。
     * 这是**取证与回退**顺序（与 renderMode 一致）：现场能一行属性退回旧行为。
     *
     * @param hasLocalPath 调用方是否提供了本地文件绝对路径（SMB/HTTP 没有 ⇒ 恒 LOOPBACK）
     * @param intentOverride intent 里的 `kivo.inputMode`（脚本化对照实验用）
     */
    fun resolve(
        app: Context,
        hasLocalPath: Boolean,
        intentOverride: String? = null,
    ): Decision {
        val forced = sysProp("debug.kivo.inputMode")?.lowercase()
            ?: intentOverride?.trim()?.lowercase()?.takeIf { it.isNotEmpty() }
        when (forced) {
            LOOPBACK, LOOPBACK_ALIAS -> return Decision(LOOPBACK, "开关强制 proxy")
            DIRECT_FILE -> return Decision(
                if (hasLocalPath) DIRECT_FILE else LOOPBACK,
                if (hasLocalPath) "开关强制 direct" else "开关强制 direct，但没有本地路径 ⇒ 无法直连",
            )
            // `auto` 是文档里的合法值 ⇒ 必须与「没设」同义，不能报成非法。
            AUTO, null -> return defaultDecision(hasLocalPath)
            // 【不许静默忽略】写错值时必须能看出来，否则"设了开关没反应"会被当成产品 bug。
            else -> return Decision(
                defaultDecision(hasLocalPath).mode,
                "开关值不可识别（$forced）⇒ 按默认",
            )
        }
    }

    /** 默认策略：有本地路径就直连，其余走代理。 */
    private fun defaultDecision(hasLocalPath: Boolean): Decision =
        if (!hasLocalPath) Decision(LOOPBACK, "无本地路径（SMB/HTTP 源）")
        else Decision(DIRECT_FILE, "本地文件默认直连")

    /** intent 里的覆盖值（`--es kivo.inputMode proxy`）。 */
    fun intentOverride(intent: Intent?): String? =
        intent?.getStringExtra("kivo.inputMode")?.trim()?.takeIf { it.isNotEmpty() }
}
