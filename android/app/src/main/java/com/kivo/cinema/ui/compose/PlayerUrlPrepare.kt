package com.kivo.cinema.ui.compose

import com.kivo.cinema.net.HttpOriginSource
import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.net.OriginPrewarm
import com.kivo.cinema.playback.PlaybackLoadGate
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 273 v15：URL（Emby）起播的**预备** —— 源站连接在 Activity 起来时就建，不等 Surface。
 *
 * ## 为什么预备要提前
 * 真机实测（273 v15 诊断轮，emby.bangumi.ca）：
 * · 源站**新建连接**要 1000–1600 ms（TLS 握手）；
 * · 源站**复用**连接只要 270–360 ms —— 差 3–5 倍。
 *
 * 【旧做法的问题】连接是在"Surface 到了之后"才建的，而起播那一刻内核
 * 连着要三个跳转（片头 → cues → 回跳片头）。片头那一跳恰好撞上
 * **握手还没做完** ⇒ 真机表头 1039–1586 ms，比复用贵一大截。
 * 诊断轮还量到：4 条连接**同时**握手时每条都要 1.2–1.6 s（互相抢），
 * 而握手一旦完成，之后每一跳都是 270–360 ms。
 *
 * 【新做法】预备在 `onCreate` 就开始：握手与**应用自身启动**（≈1.4 s）重叠。
 * 等 Surface 到了、连接早就在池里 ⇒ 内核第一跳直接复用（≈300 ms）。
 *
 * ## 预备分两组，**故意错开**
 * · 组一（**并发两条**）：总长探测 + 片尾预取。
 *   【为什么只放两条】并发的握手会互相抢：4 条并发时每条 1.2–1.6 s，
 *   2 条 ~1.1 s，单独一条 ~0.8 s。起播要的是"**已经有**一条建好的连接"，
 *   不是"很多条" ⇒ 宁可少而准。
 * · 组二（组一之后）：保温池。它服务的是**播放中**的跳读，
 *   起播阶段用不上，早建只会跟组一抢握手。
 *
 * ## 预备是"加速器"，不是"依赖"
 * 等不到（超时）也照样起播：读侧自己会撞上源站的错误并报错，
 * 卡死起播比慢 0.3 s 更糟。
 */
internal class PlayerUrlPrep(
    /** 主播放源（预备完成即可用；**由 Activity 负责关**）。 */
    val source: HttpOriginSource,
    /** 预备就绪信号（外部预热时用**它**的）。 */
    private val ready: CountDownLatch = CountDownLatch(1),
    /** 判决的来源：预热接手时读对方的字段，否则读自己的。 */
    private val verdictOf: () -> Throwable? = { null },
) : PlaybackLoadGate {

    /** 源站**明确拒绝**时的理由（null = 没判决）。瞬时错误不算判决。 */
    @Volatile private var ownVerdict: Throwable? = null

    override val verdict: Throwable? get() = verdictOf() ?: ownVerdict

    /** 记下判决（预备线程用）。 */
    fun setVerdict(reason: Throwable?) {
        ownVerdict = reason
    }

    /** 预备做完了（连接就绪 / 访问校验有结论）。 */
    fun markReady() = ready.countDown()

    /**
     * 等预备完成。
     *
     * @return true = 预备完成；false = 超时（按"没准备好"处理，但**不**当失败）。
     */
    override fun awaitReady(timeoutMs: Long): Boolean = ready.await(timeoutMs, TimeUnit.MILLISECONDS)
}

/**
 * 建起播预备（273 v15）：**在 Activity 起来时就做**。
 *
 * 【273 v16：先看详情页有没有预热好的源】有 ⇒ 直接接手（连接已在池里、
 * 片尾窗口已在内存里），起播的第一跳就是**复用**。
 *
 * @return 预备句柄；null = 这个 Activity 不是 URL 起播（本地 / SMB）。
 */
internal fun PlayerComposeActivity.prepareUrlSource(): PlayerUrlPrep? {
    val url = intent.getStringExtra("kivo.playurl") ?: return null
    val token = intent.getStringExtra("kivo.embyToken")
    // Emby 详情里就有片源总长（零额外请求）⇒ 片尾预取能在 t=0 发出去。
    val sizeHint = intent.getLongExtra("kivo.playSize", -1L)
    // 273 v16：详情页预热好的源 —— 接手它，别再握手一次。
    val prewarmed = OriginPrewarm.take(url)
    if (prewarmed != null) {
        NetDiag.log("[prep] 接管详情页预热的源（连接与片尾窗口已就绪）")
        return PlayerUrlPrep(prewarmed.source, prewarmed.ready) { prewarmed.verdict }
    }
    val source = HttpOriginSource(
        url, token, "play",
        tailWindowBytes = tailWindowBytes(),
        seekWindowBytes = HttpOriginSource.DEFAULT_SEEK_WINDOW_BYTES,
        sizeHint = sizeHint,
    )
    val prep = PlayerUrlPrep(source)
    Thread({
        val t0 = System.currentTimeMillis()
        NetDiag.log("[prep] URL 预备开始")
        // 组一：**并发两条** —— 总长探测 + 片尾预取。
        // 探测走主 client（顺带把总长记进源），片尾预取走尾段 client，
        // 两条各自建一条连接；之后内核的第一跳就有连接可借。
        val probe = Thread({ prep.setVerdict(source.accessVerdict()) }, "KivoUrlVerdict")
        probe.start()
        if (sizeHint > 0) {
            // 有总长 ⇒ 片尾预取**立刻**发出去（它要的就是尾段那 64 KiB）。
            // 预取是异步的：读侧命中"正在预取的窗口"时会等它落地。
            source.prefetchTail()
        }
        probe.join()
        if (sizeHint <= 0) {
            // 没有总长提示 ⇒ 先探出总长再预取（预取的窗口起点要总长）。
            source.prefetchTail()
        }
        // 组二：**不在这里**启动保温池 —— 它要等首帧（见 [startUrlWarmup]）。
        prep.markReady()
        NetDiag.log(
            "[prep] URL 预备完成 ms=${System.currentTimeMillis() - t0} " +
                "verdict=${prep.verdict?.message ?: "无"}",
        )
    }, "KivoUrlPrepare").start()
    return prep
}

/**
 * 启动保温池 —— **首帧之后**才调（273 v15）。
 *
 * 【为什么不能提前】保温连接要的是"池里有一条空着的连接"，
 * 而起播那几跳（片头 → cues → 回跳片头）要的**也是**空连接。
 * 真机实测：保温池的握手与片头那一跳撞在一起时，
 * 片头表头从 277 ms 涨到 957–2112 ms（起播从 1.13 s 变成 2.9 s）。
 * 起播关键路径只有一次机会，保温随时可以补 ⇒ 保温让路。
 *
 * 【推迟会不会伤到拖动】保温服务的是**播放中**的跳读（拖到未缓存位置）。
 * 首帧之后 1–2 s 才有用户能拖，那时保温的连接已经建好 ⇒ 不伤。
 */
internal fun PlayerComposeActivity.startUrlWarmup() {
    urlPrep?.source?.startWarmup()
    // 273 v26：**同时**把 cues 索引读好（后台线程，两次小请求）。
    // 【为什么趁现在】落点预取要"时间 → 字节偏移"这张表；
    // 表没备好时拖动只能照常走网络（多付一次冷连接 + 一整段下载）。
    // 首帧之后正是源站连接都建好、带宽空闲的时候 ⇒ 读表不抢起播。
    urlPrep?.source?.warmCueIndex()
}

/**
 * 取证钩子：`--ei kivo.tailWindowKiB N` 改片尾预取的窗口大小。
 *
 * · 不传 ⇒ 默认（[HttpOriginSource.DEFAULT_TAIL_WINDOW_BYTES]）。
 * · `N ≤ 0` ⇒ **关掉**片尾预取（回到没有缓存的旧行为）。
 * · `N > 0` ⇒ 窗口 = N KiB。
 *
 * 【为什么要有】起播提速要归因到**某一个**改动，
 * 而"预取开/关、窗口多大"必须能在**同一台设备、同一份断点状态**下对比 ——
 * 改代码重编装机一轮要好几分钟，运行时开关一次装机就能跑完所有组合。
 *
 * 【默认开着】不传这个 extra 时行为就是正式行为。
 */
internal fun PlayerComposeActivity.tailWindowBytes(): Long {
    val kib = intent.getIntExtra("kivo.tailWindowKiB", -1)
    if (kib < 0) return HttpOriginSource.DEFAULT_TAIL_WINDOW_BYTES
    if (kib == 0) return 0L
    return kib.toLong() * 1024L
}

/**
 * 等预备就绪的上限（毫秒）。
 *
 * 【为什么要有上限】预备是加速器不是依赖 —— 等不到也照样起播。
 * 真机实测握手 1.0–1.6 s ⇒ 2.5 s 足够，等它没有意义。
 */
internal const val PREP_READY_WAIT_MS = 2_500L
