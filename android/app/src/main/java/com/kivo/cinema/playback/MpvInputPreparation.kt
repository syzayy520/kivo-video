package com.kivo.cinema.playback

import android.content.Intent
import com.kivo.cinema.mpv.LocalRangeServer
import com.kivo.cinema.net.ByteSource
import com.kivo.cinema.net.NetDiag

/**
 * 354 · **把"输入从哪来"准备成 mpv 能读的一个 URL**（起播流水线的第 1 步）。
 *
 * ## 为什么从 [MpvStartSequence] 拆出来
 *
 * 原来这一段（起代理 → register → urlFor）混在起播编排里，而「内核/策略/闸门/首帧」
 * 是另一类职责。把输入通道单独拎出来后：
 * · 通道决策（[PlaybackInputMode]）、能否真的读（[DirectFileMedia]）、
 *   资源创建（代理/源）三件事集中在**一个地方**，不会散落；
 * · [MpvStartSequence] 只剩"内核怎么起、什么时候放行、什么时候算上屏"。
 *
 * ## 顺序不可换
 *
 * 决策 → 预检 → 起代理（仅在需要时）→ 取 URL。
 * 预检在起代理之前：直连成立时**一个代理对象都不该创建**（那是白开销）。
 *
 * ## 不许静默降级
 *
 * 直连被否决（开关强制 proxy / 预检失败）时**必须**落一行日志说明原因，
 * 并且要么回退到代理，要么抛出可读错误 —— 不允许静默变成黑屏。
 */
internal class MpvInputPreparation(
    private val app: android.content.Context,
    /** 需要代理的源；直连时可为 null。 */
    private val source: ByteSource?,
    /** 本地文件绝对路径；非空即有资格直连。 */
    private val directPath: String?,
    private val intent: Intent? = null,
) {

    /**
     * 准备结果。
     *
     * @param url 交给 mpv `loadfile` 的字符串（本地路径 或 `http://127.0.0.1:.../v/<id>`）
     * @param proxy 只在走代理时非空（直连时**不创建**，释放时也就不需要它）
     * @param registeredId 代理里的源 id（释放时还给代理）
     * @param directUri 直连路径（null = 走代理）；仅用于取证与日志
     */
    class Ready(
        val url: String,
        val proxy: LocalRangeServer?,
        val registeredId: String?,
        val directUri: String?,
    )

    fun prepare(maxRangeBytes: Long): Ready {
        val decision = PlaybackInputMode.resolve(
            app,
            hasLocalPath = directPath != null,
            intentOverride = PlaybackInputMode.intentOverride(intent),
        )
        NetDiag.log("[input] ${decision.describe()}")

        val directUri = if (decision.isDirect) directUriOrFallback() else null
        if (directUri != null) {
            // 直连：不建代理、不 register —— 这条路 mpv 自己读文件。
            return Ready(url = directUri, proxy = null, registeredId = null, directUri = directUri)
        }

        val src = source
            ?: throw IllegalStateException("既无本地可读路径也无 ByteSource，无法播放")
        val proxy = LocalRangeServer.start(maxRangeBytes = maxRangeBytes)
        val id = proxy.register(src)
        return Ready(url = proxy.urlFor(id), proxy = proxy, registeredId = id, directUri = null)
    }

    /** 预检；失败回退代理并留痕（返回 null 表示"改用代理"）。 */
    private fun directUriOrFallback(): String? {
        val path = directPath ?: return null
        return when (val plan = DirectFileMedia.plan(path)) {
            is DirectFileMedia.Plan.Ok -> {
                NetDiag.log("[input] 直连可用 uri=${plan.uri} size=${plan.sizeBytes}")
                plan.uri
            }
            is DirectFileMedia.Plan.Failed -> {
                NetDiag.log("[input] 直连预检失败（${plan.reason}）⇒ 回退 loopback 代理")
                null
            }
        }
    }
}
