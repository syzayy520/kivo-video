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
    /**
     * 需要代理的源（SMB / WebDAV / HTTP）。本地文件直连时为 null ——
     * 见 [directPath] 与 [PlaybackInputMode]。
     */
    private val input: ByteSource?,
    private val prefs: PlaybackPrefs,
    /**
     * **宿主 Activity 的 intent**（可为空）。
     *
     * 【为什么必须显式传】`app` 是 `applicationContext`，**不是 Activity**
     * ⇒ `(app as? Activity)` 恒为 null ⇒ intent 里的实验开关
     * （`kivo.renderMode` / `kivo.gpuNext` / `kivo.demuxerMb` /
     * `kivo.maxRangeMb` / `kivo.hdrProbe` / `kivo.inputMode` …）**从来就读不到**。
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
    /**
     * 354 · 本地文件绝对路径。**非空 = 有资格直连**（是否真的直连由 [PlaybackInputMode] 决定，
     * 是否真的能读由 [DirectFileMedia] 预检决定）。
     *
     * 三者互斥关系：`directPath != null` 时 [input] 可以为 null；
     * `directPath == null` 时 [input] 必须非空（否则无处取数据）。
     */
    private val directPath: String? = null,
) {

    /**
     * 起播成功的产物。
     *
     * @param native 活着的内核会话。
     * @param proxy 本地 HTTP 代理；**本地文件直连时为 null**（没有代理可释放）。
     * @param registeredId 代理里的源 id（释放时要还给代理）；直连时同样为 null。
     * @param directUri 直连时交给 mpv 的路径；走代理时为 null。
     * @param positionSeconds 起播时刻的播放位置（秒）。
     * @param durationSeconds 片源总时长（秒）；0 = 内核还没报上来。
     */
    class Prepared(
        val native: MpvSession,
        val proxy: LocalRangeServer?,
        val registeredId: String?,
        val directUri: String?,
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

        // 1) 准备输入通道（354）：本地文件直连 `loadfile <path>`，其余起 loopback 代理。
        //
        //    【为什么本地不再走代理】代理是给"需要协议转换"的源准备的；本地文件没有
        //    协议要转，套一层换来的是三个损失：`sub-auto` 看不到兄弟文件（同名字幕失效）、
        //    格式识别受响应头支配（D12I：写死 matroska ⇒ 全部 .ts 播不了）、
        //    多一次 socket + 全量拷贝。
        //
        //    【决策在哪】决策（[PlaybackInputMode]）、预检（[DirectFileMedia]）、
        //    资源创建（代理/源）全在 [MpvInputPreparation] —— 这里只取结果，
        //    不判断走哪条路（否则又回到"两个地方各算一遍"）。
        //
        //    【274 · 单次响应上限】只对代理路径有意义（本地文件不经过它）。
        val inputPlan = MpvInputPreparation(app, input, directPath, intent)
            .prepare(maxRangeBytes = intentMaxRangeBytes())
        val proxy = inputPlan.proxy
        val registeredId = inputPlan.registeredId
        val url = inputPlan.url

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
        // 345：initialize 成功即可读 property ⇒ 在这里注册，避免早快照全 not_measured。
        PlaybackMetrics.attachNative(native)
        native.attachSurface(surface)

        // 301 · 字幕字体诊断：**读回内核实际生效的字体选项**。
        //
        // 【为什么必须读回】`sub-fonts-dir` 走的是 option 通道（initialize 之前 set），
        // 它的失败**不会**出现在 property 日志里（"Set property" 只记录运行期改动）。
        // 而 libass 是否扫描字体目录直接决定**文本字幕能不能显示**：
        // 301 实测同一部片 PGS 正常、SRT 全空，内核日志只有
        // `can't find selected font provider`，**一条 `Loading font file` 都没有**
        // —— 说明 libass 根本没拿到目录（libass 的 embedded provider 会无条件扫描
        // `fonts_dir` 并逐条打 "Loading font file"，与 fontconfig 无关）。
        if (fonts.dir.isNotBlank()) {
            val actual = runCatching { native.property("options/sub-fonts-dir") }.getOrNull()
            val ok = actual == fonts.dir
            NetDiag.log(
                "[subfonts] 内核 sub-fonts-dir=$actual 期望=${fonts.dir} " +
                    if (ok) "匹配" else "不匹配 ⇒ 文本字幕会显示不出来",
            )
        }

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
        MpvCacheDiag.attach(native, app)

        // 6) 等**画面真的上屏**（口径见 [MpvFirstFrameWait]）。
        //    292 · 方案 C：这里**不**恢复起播预缓冲选项。方案 B 已证伪 ——
        //    mpv 的 initial buffering 恰在首帧**之后**才进入，绑首帧恢复会当场掐断它。
        //    恢复改由 [MpvCacheDiag] 每 2s 喂 runway、达标或超时才写回默认
        //    （见 [RebufferPolicy.note]）。
        val first = MpvFirstFrameWait.await(native, isCancelled)

        // 317 · 取证：首帧后到底能不能读到视频属性。
        // 【为什么必须打这条】317 修复用 `video-codec` 区分"纯音频"与"vo 失败"，
        // 但如果这个属性在本实现里根本读不到，那个判据就是盲的 —— 先证明可读性。
        NetDiag.log(
            "[vo] 首帧后属性 vid=${native.property("vid")} " +
                "codec=${native.property("video-codec")} " +
                "conf=${first.confirmed} " +
                "params=${native.property("video-params")?.take(80)}",
        )

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
            directUri = inputPlan.directUri,
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
        // 298 · 诊断开关：`debug.kivo.hwdec` 覆盖 hwdec（取证用，缺省不写）。
        //
        // 【为什么需要它】HDR10+ 的动态元数据（SMPTE ST 2094-40）**硬解路径拿不到**：
        // MediaCodec 只把它塞进 `hdr10-plus-info` 这个 Java ByteBuffer，
        // ffmpeg 的 hevc_mediacodec 不转成 `AV_FRAME_DATA_DYNAMIC_HDR_PLUS` side data
        // ⇒ mpv/libplacebo 看不到 ⇒ tone-map 只能靠 `hdr-compute-peak`（内容分析）。
        // 要确认「软解能否用上动态元数据」，就得临时强制软解（`no`），本开关为此存在。
        // 真机 298 取证：硬解 4K HDR10+ 60.51fps / 软解性能见 279 §十六。
        val hwdecOverride = systemProperty(HWDEC_PROPERTY)
        if (!hwdecOverride.isNullOrBlank()) {
            val ok = native.setProperty("hwdec", hwdecOverride)
            NetDiag.log("[hwdec] 诊断覆盖 hwdec=$hwdecOverride 写入=$ok")
        }
        // 292 · 自适应 rebuffer 闸门：必须在 loadFile 之前预置（首帧零缓冲窗口
        // 早于 MpvCacheDiag 第一次采样）。开关 off 时一行属性都不写。
        RebufferPolicy.beforeLoad(native)
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

    /**
     * 298 · 读系统属性（诊断开关用；取不到返回 null，不抛）。
     *
     * 与 [RebufferPolicy] 里那份同源 —— 反射读 `android.os.SystemProperties.get`。
     */
    private fun systemProperty(name: String): String? = try {
        Class.forName("android.os.SystemProperties")
            .getMethod("get", String::class.java).invoke(null, name) as? String
    } catch (e: Exception) {
        null
    }

    // 280：出画通路与 HDR 的决策已全部上收到 [PlaybackStack]。
    // 这里原来有 useGpuNext() 与 intentRenderMode() 两个私有函数，
    // 与 PlayerComposeActivity 各自算一遍、且输入不等价（一个读 kivo.renderMode，
    // 一个还兼容 kivo.gpuNext）⇒ 已删除，只保留 PlaybackStack 一处。

    companion object {
        /** 298 · 诊断：覆盖 hwdec（`mediacodec` / `no` / …）。缺省不写。 */
        private const val HWDEC_PROPERTY = "debug.kivo.hwdec"

        /**
         * 闸门等待的上限（毫秒）。
         *
         * 【为什么有上限】闸门是加速器不是依赖 —— 超时照样放行。
         * 真机实测源站握手 1.0–1.6 s ⇒ 2.5 s 足够，再久就是源站出问题了。
         */
        const val GATE_WAIT_MS = 2_500L
    }
}
