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
    /**
     * **宿主 Activity 的 intent**（可为空）。
     *
     * 【为什么必须显式传】`app` 是 `applicationContext`，**不是 Activity**
     * ⇒ `(app as? Activity)` 恒为 null ⇒ intent 里的实验开关
     * （`kivo.renderMode` / `kivo.gpuNext` / `kivo.demuxerMb` /
     * `kivo.maxRangeMb` / `kivo.hdrProbe` …）**从来就读不到**。
     * （这个坑之前在 `MpvPlaybackSession` 已经犯过一次：那里加了 `host`，
     * 这里却又用 `app` 重犯了一遍 ⇒ 改成显式参数，从根上断掉这条路。）
     */
    private val intent: android.content.Intent? = null,
    /**
     * 播放栈选型（内核 / 出画通路 / 真 HDR）。
     *
     * 【默认值就是"算一次"】[PlaybackStack.resolve] 只用 [app] 与 [intent]，
     * 两者在构造时都已就绪 ⇒ 默认参数求值即可得到与
     * `PlayerComposeActivity.prepareHdrSurface` **完全相同**的结果
     * （280 之前那两处各算一遍、输入还不等价，会推出矛盾组合）。
     * 调用方如果已经算过，传进来复用即可，避免重复读系统属性。
     */
    private val stack: PlaybackStack.Decision = PlaybackStack.resolve(app, intent),
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
        //
        //    【274 · 单次响应上限（实验开关）】播放主链路默认**不限**（0），
        //    而 273 v12 给预览设了上限（PREVIEW_MAX_RANGE_BYTES）。
        //    播放主链路不限 ⇒ 内核拿到第一帧就 seek 走时，代理还在往一个
        //    没人读的 socket 里灌数据（真机 v18：预览取 1 帧顺序拉了 24 MiB）
        //    ⇒ 这是"超买"的一大半。
        //    ⇒ 实验用 `--ei kivo.maxRangeMb N` 覆盖，0 = 不限（保持旧行为）。
        val proxy = LocalRangeServer.start(
            maxRangeBytes = intentMaxRangeBytes(),
        )
        val registeredId = proxy.register(input)
        val url = proxy.urlFor(registeredId)

        // 2) 起内核。
        //    【字体目录要先备好】内核初始化时就要设 sub-fonts-dir，
        //    而 libass 会把该目录里的每个文件装进进程（真机：/system/fonts
        //    ⇒ 242 个文件、125.6 MB 常驻原生堆）⇒ 只给它"最小字体集"。
        val fonts = SubtitleFontSet.prepare(app)
        val native = MpvSession.create(app)
        // 280 · 指标：内核创建完成（与"源站握手"是并行关系，见文件头时序说明）。
        PlaybackMetrics.mark(PlaybackMetrics.T_KERNEL_CREATED)
        // 【280 · 出画通路】选型来自 [PlaybackStack]（唯一决策点）：
        //   gpu-next = mpv 自己的 VO + 完整 libplacebo 渲染器
        //              （ES3 context ⇒ compute shader ⇒ HDR 峰值计算不再被禁，与 SenPlayer 同架构）；
        //   libmpv   = render API + legacy RA（无 compute shader，回退用）。
        //   Surface 由 native 层通过 `wid` 选项交给 mpv（见 mpv_session.cpp）。
        val vo = stack.vo
        // 真 HDR 输出（10bit Surface + BT.2020 PQ）。**已含设备能力闸门**：
        // 设备 gralloc 不支持 10bit 时这里就是 false（见 [HdrOutputSwitch]），
        // 所以调用方不必再判一次 —— 280 之前这里有第二份判断，会与 Activity 侧不一致。
        val hdrSurface = stack.hdrOutput
        NetDiag.log("[hdr] ${stack.describe()}")
        // 帧插值（MEMC）：必须在 initialize **之前**解析 —— `display-fps` 是
        // initialize 之后不可写的选项（真机 -3 = PROPERTY_UNAVAILABLE），
        // 而它是插值能否真正生效的前提（见 [VideoInterpolation]）。
        val interp = VideoInterpolation.plan(app, VideoInterpolationSwitch.isEnabled(prefs, intent))
        native.initialize(
            vo = vo,
            ao = "audiotrack",
            logFile = File(app.filesDir, "mpv-kernel.log").absolutePath,
            subFontsDir = fonts.dir,
            subFontFamily = fonts.family,
            // 278-P2：真 HDR 输出（10bit Surface + BT.2020 PQ）。
            // 【为什么这里再判一次】Surface 的 10bit 格式与 dataspace 已在
            // Activity 侧配好（PlayerSessionStart.prepareHdrSurface）；
            // 这里只负责让内核用 10bit EGL config + 给 EGL surface 打 PQ 标注。
            hdrSurface = hdrSurface,
            interpolation = interp.enabled,
            displayFps = interp.displayFps,
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
        // 280 · 指标：loadfile 起点。到首帧的差就是"打开+解封装+解码+上屏"的总耗时，
        // 口径比 275 的 open_ms 宽（275 的 open_ms 只到 demux 可用，mpv 没对外暴露那个点）。
        PlaybackMetrics.mark(PlaybackMetrics.T_LOADFILE)
        native.loadFile(url, startSeconds)

        // 5) 挂缓存归因探针（内存/超买两个缺口的唯一证据来源）。
        //    【为什么在这挂、而不是等画面上屏】缓存从第一跳就开始涨 ——
        //    等首帧再挂会漏掉"起播期灌了多少"，而那正是超买最凶的一段。
        MpvCacheDiag.attach(native)

        // 6) 等**画面真的上屏**（口径见 [MpvFirstFrameWait]）。
        val first = MpvFirstFrameWait.await(native, isCancelled)

        // 7) 274-HDR · L2：HDR→SDR 色调映射。
        //    【为什么放在首帧之后】色彩参数（`video-params`）要等解封装报出轨道才有值；
        //    而"是不是 HDR"这个判定必须在**拿到真实参数**之后才能做（不许按扩展名猜）。
        //    【为什么对 SDR 内容零影响】见 [MpvHdrToneMap]：非 HDR 时一个参数都不下发。
        //    【开关】`debug.kivo.hdrToneMap=off` 可关掉（取证量真基线用，见 [HdrToneMapSwitch]）。
        if (HdrToneMapSwitch.isEnabled(app)) {
            MpvHdrToneMap.applyIfHdr(native)
        } else {
            NetDiag.log("[color] L2 已按 debug.kivo.hdrToneMap=off 关闭 ⇒ 不下发色调映射参数")
        }

        // 7b) 278-P2 · 真 HDR 输出：写 Surface 的 PQ dataspace。
        //     【为什么必须在这里（首帧之后）】对 gpu-next 通路，mpv 的 EGL surface
        //     是**打开文件时**才创建的，不是 attachSurface 时 ⇒ 早一步写 dataspace
        //     时 buffer 还是 8bit，内核会以 EINVAL 拒（真机实测 rc=-22）。
        //     首帧之后：EGL 已用 10bit config 建好面，buffer 已是 10bit ⇒ 合法。
        if (hdrSurface) {
            // 取证用：intent --ei kivo.hdrProbe 1 ⇒ 挨个试候选 dataspace。
            val probe = intent?.getIntExtra("kivo.hdrProbe", 0) == 1
            NetDiag.log(
                "[hdr] " + if (probe) HdrSurfaceConfig.probeDataspaces(surface)
                else HdrSurfaceConfig.applyDataspace(surface)
            )
        }

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
        // 280 · 出画通路记录（哪条路出的画 ⇒ 出问题时第一眼能定位）。
        if (stack.renderMode == PlaybackRenderMode.GPU_NEXT) {
            NetDiag.log("[color] 出画模式 = gpu-next（mpv 自己的 VO + 完整 libplacebo）")
        } else {
            NetDiag.log("[color] 出画模式 = legacy（render API + legacy RA）")
        }
        // ASS 特效：忽略=剥成统一样式，否则保留源特效（面板仍可临时改写）。
        native.setProperty("sub-ass-override", if (prefs.ignoreAssStyle) "yes" else "no")
        // 字幕首选语言：须在 loadFile 前设，自动选轨才匹配。
        val slang = prefs.subtitleSlang()
        if (slang.isNotEmpty()) native.setProperty("slang", slang)
        // 音频首选语言（263）：同样在 loadFile 前设，自动选音轨才匹配。
        val alang = prefs.audioAlang()
        if (alang.isNotEmpty()) native.setProperty("alang", alang)
        // 缓冲上限：按当前网络（Wi-Fi/蜂窝）取用户设的 demuxer 上限。
        //
        // 【274 · 实验开关】`--ei kivo.demuxerMb N` 覆盖。
        //   【为什么这个旋钮重要】273 v25 在 C++ 侧把它收成 32 MiB，
        //   但**这一行又覆盖回用户偏好的 64 MiB**（真机内核日志：
        //   `Set property: demuxer-max-bytes="64MiB" -> 1`）⇒ v25 从未生效。
        //   真机 240 s 长稳：内核缓存顶格 `total=71-72 M`（64 前向 + 8 回看），
        //   而 35 s 播放只需 11 MiB ⇒ **67 MiB 是内核吃饱了不读、代理还在灌的浪费**。
        val demuxerMb = intentDemuxerMb()
        native.setProperty(
            "demuxer-max-bytes",
            if (demuxerMb > 0) "${demuxerMb}MiB" else prefs.demuxerMaxBytes(),
        )
    }

    /**
     * 274 · 实验开关：覆盖单次响应上限（字节）。
     *
     * 【来源】Activity 的 intent `--ei kivo.maxRangeMb N`。
     * 【缺省】0 = 不限（= 旧行为）。
     */
    private fun intentMaxRangeBytes(): Long {
        val mb = intent?.getIntExtra("kivo.maxRangeMb", 0) ?: 0
        return if (mb > 0) mb.toLong() * 1024L * 1024L else 0L
    }

    /**
     * 274 · 实验开关：覆盖 demuxer 缓存上限（MiB）。
     *
     * 【来源】Activity 的 intent `--ei kivo.demuxerMb N`。
     * 【缺省】0 = 用用户偏好（= 旧行为）。
     */
    private fun intentDemuxerMb(): Int = intent?.getIntExtra("kivo.demuxerMb", 0) ?: 0

    // 280：出画通路与 HDR 的决策已全部上收到 [PlaybackStack]。
    // 这里原来有 useGpuNext() 与 intentRenderMode() 两个私有函数，
    // 与 PlayerComposeActivity 各自算一遍、且输入不等价（一个读 kivo.renderMode，
    // 一个还兼容 kivo.gpuNext）⇒ 已删除，只保留 PlaybackStack 一处。

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
