package com.kivo.cinema.playback

import android.content.Context
import android.view.Surface
import com.kivo.cinema.mpv.LocalRangeServer
import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.ByteSource
import com.kivo.cinema.net.NetDiag
import java.io.File

/**
 * 273 v15/v16：**起播编排** —— 从"什么都没有"到"画面上屏"的整条流水线。
 *
 * ## 为什么单独立一个文件
 * 起播是一串**有顺序**的步骤（起代理 → 起内核 → 设策略 → 过闸门 → 发第一跳 →
 * 等画面），每一步都有自己的理由（代理是传输、内核是解码、闸门是源站握手、
 * 首帧是用户可见的时刻）。把这段流程从会话里搬出来，会话就只剩
 * "状态 + 控制 + 释放"，而这才是会话本身的职责。
 *
 * ## 顺序不可换的地方
 * · **内核创建与源站握手并行**：闸门挡的是 `loadFile`（发第一跳），
 *   不是"内核建好"—— 真机内核创建只要 39 ms，让它等握手是白等 1.3 s。
 * · **闸门在 loadFile 之前**：源站**新建**连接要 0.8–1.6 s（TLS 握手），
 *   复用只要 0.3 s。第一跳撞上没建好的连接就要付一次握手（真机表头 1155 ms）。
 * · **判决在 loadFile 之前查**：源站明确拒绝（401/403/404）时，
 *   原因要**原样**透给用户（不许静默降级，也不许包成"内核坏了"）。
 */
internal class MpvStartSequence(
    private val app: Context,
    private val input: ByteSource,
    private val prefs: PlaybackPrefs,
) {

    /**
     * 起播成功的产物。
     *
     * @param native 活着的内核会话。
     * @param proxy 本地 HTTP 代理（mpv 通过它读源）。
     * @param registeredId 代理里的源 id（释放时要还给代理）。
     * @param positionSeconds 起播时刻的播放位置（秒）。
     * @param durationSeconds 片源总时长（秒）；0 = 内核还没报上来。
     */
    class Prepared(
        val native: MpvSession,
        val proxy: LocalRangeServer,
        val registeredId: String,
        val positionSeconds: Double,
        val durationSeconds: Double,
    )

    /**
     * 跑完整条起播流水线。
     *
     * @param surface 视频输出目标。
     * @param startSeconds > 0 = 续播位（秒）；<= 0 = 从头播。
     * @param loadGate 起播放行闸门（null = 不设闸门）。
     * @param isCancelled 判断是否已被取消。
     * @throws PlaybackRejected 源站**明确拒绝**（原因要原样透出）。
     * @throws IllegalStateException 内核起不来（时钟没起跳）。
     */
    fun prepare(
        surface: Surface,
        startSeconds: Double,
        loadGate: PlaybackLoadGate?,
        isCancelled: () -> Boolean,
    ): Prepared {
        check(!isCancelled()) { "播放已取消" }
        val t0 = System.currentTimeMillis()

        // 1) 起本地代理，把源变成 mpv 能读的 HTTP URL。
        //    【为什么不需要 open()】ByteSource 的语义是"打开即已就绪"
        //    （SmbByteSource.open() 里已经建好连接并拿到了大小），
        //    而 RangeSource 那种"分阶段 open"的形态是给 fd 路径用的。
        val proxy = LocalRangeServer.start()
        val registeredId = proxy.register(input)
        val url = proxy.urlFor(registeredId)

        // 2) 起内核。
        //    【字体目录要先备好】内核初始化时就要设 sub-fonts-dir，
        //    而 libass 会把该目录里的每个文件装进进程（真机：/system/fonts
        //    ⇒ 242 个文件、125.6 MB 常驻原生堆）⇒ 只给它"最小字体集"。
        val fonts = SubtitleFontSet.prepare(app)
        val native = MpvSession.create(app)
        native.initialize(
            vo = "libmpv",
            ao = "audiotrack",
            logFile = File(app.filesDir, "mpv-kernel.log").absolutePath,
            subFontsDir = fonts.dir,
            subFontFamily = fonts.family,
        )
        applyPlaybackPolicy(native)
        native.attachSurface(surface)

        // 3) 过闸门：等源站连接（273 v15/v16）。
        //    【超时也放行】闸门是加速器不是依赖 —— 卡死起播比慢 0.3 s 更糟。
        NetDiag.log("[prep] 内核创建完成 ms=${System.currentTimeMillis() - t0}")
        loadGate?.awaitReady(GATE_WAIT_MS)
        NetDiag.log("[prep] 闸门放行 ms=${System.currentTimeMillis() - t0}")
        // 源站**明确拒绝**（401/403/404）才中止 —— 瞬时错误不判决。
        loadGate?.verdict?.let { throw PlaybackRejected(it.message ?: "源站拒绝了这个视频") }

        // 4) 发第一跳。
        native.loadFile(url, startSeconds)

        // 5) 挂缓存归因探针（内存/超买两个缺口的唯一证据来源）。
        //    【为什么在这挂、而不是等画面上屏】缓存从第一跳就开始涨 ——
        //    等首帧再挂会漏掉"起播期灌了多少"，而那正是超买最凶的一段。
        MpvCacheDiag.attach(native)

        // 6) 等**画面真的上屏**（口径见 [MpvFirstFrameWait]）。
        val first = MpvFirstFrameWait.await(native, isCancelled)

        // 时长：mpv 的 `duration` 属性在媒体加载完成后才有值。
        // 不回填的话进度条会一直显示 0:00:00 / 0:00:00（用户没法拖进度）。
        return Prepared(
            native = native,
            proxy = proxy,
            registeredId = registeredId,
            positionSeconds = first.positionSeconds,
            durationSeconds = native.durationSeconds(),
        )
    }

    /**
     * 把播放策略（262/263）下发到内核。
     *
     * 【为什么必须在 loadFile 之前】字幕/音轨的"首选语言"要**开文件前**设，
     * 否则内核已经选完轨了，设了也不生效。
     */
    private fun applyPlaybackPolicy(native: MpvSession) {
        // ASS 特效：忽略=剥成统一样式，否则保留源特效（面板仍可临时改写）。
        native.setProperty("sub-ass-override", if (prefs.ignoreAssStyle) "yes" else "no")
        // 字幕首选语言：须在 loadFile 前设，自动选轨才匹配。
        val slang = prefs.subtitleSlang()
        if (slang.isNotEmpty()) native.setProperty("slang", slang)
        // 音频首选语言（263）：同样在 loadFile 前设，自动选音轨才匹配。
        val alang = prefs.audioAlang()
        if (alang.isNotEmpty()) native.setProperty("alang", alang)
        // 缓冲上限：按当前网络（Wi-Fi/蜂窝）取用户设的 demuxer 上限。
        native.setProperty("demuxer-max-bytes", prefs.demuxerMaxBytes())
    }

    companion object {
        /**
         * 闸门等待的上限（毫秒）。
         *
         * 【为什么有上限】闸门是加速器不是依赖 —— 超时照样放行。
         * 真机实测源站握手 1.0–1.6 s ⇒ 2.5 s 足够，再久就是源站出问题了。
         */
        const val GATE_WAIT_MS = 2_500L
    }
}
