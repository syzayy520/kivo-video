package com.kivo.cinema.mpv

import android.app.Activity
import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.view.Surface
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbByteSource
import com.kivo.cinema.smb.SmbCredential
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 253-P6-2 诊断：**字幕到底有没有内容**。
 *
 * ## 它回答什么
 * "字幕不上屏"有三种完全不同的原因，**必须用数据分开**：
 *  1. 该时刻**根本没有字幕**（选轨/位置都对，只是内容为空）—— 最常见，也最容易被误判成"渲染失败"；
 *  2. 字幕**解码失败**（有内容但解不出位图）；
 *  3. 字幕**解出来了但没画到画面上**（渲染层问题）。
 *
 * 旧交接文档把这三种混成了一句"字幕没显示"，然后去查渲染层 —— 方向就错了。
 *
 * ## 判据（全部来自 mpv 自己的属性，不猜）
 * - `sub-text`        —— 当前时刻的字幕文本（文本字幕才有内容；PGS 位图为空属正常）
 * - `sub-start` / `sub-end` —— **字幕包的覆盖区间**（秒）。这是硬证据：
 *   若 `[start, end]` 不含目标时刻 ⇒ 该时刻本来就没有字幕。
 * - `sid`             —— 当前选中的字幕轨 id
 * - `sub-visibility`  —— 字幕是否被隐藏
 * - `osd-dimensions/w` `/h` —— OSD 的目标尺寸（位图字幕按它缩放）
 * - `track-list/<i>/type` / `/external` —— 轨道分类
 *
 * ## 为什么用 mpv 的属性而不是截图
 * 截图只能回答"画面上有没有字"，回答不了"本来有没有内容"。
 * 而这正是本卡卡住的地方 —— 必须先把这两件事分开。
 */
class MpvSubtitleDiagnosticInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var passed = 0
    private var total = 0
    private var args: Bundle? = null

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        args = arguments
        start()
    }

    override fun onStart() {
        val log = StringBuilder()
        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: MpvPlaybackSession? = null
        try {
            case("字幕内容诊断：包区间 / 文本 / OSD 尺寸") {
                val host = args?.getString("smbHost") ?: "192.168.5.1"
                val share = args?.getString("smbShare") ?: "XiaoMi"
                val path = args?.getString("smbPath")
                checkNotNull(path) { "缺少 -e smbPath" }
                // 目标时刻（秒）。默认 14 —— Shelter 的片头对白起点附近。
                val at = (args?.getString("atSec") ?: "14").toDouble()
                val subtitleId = (args?.getString("subId") ?: "2").toInt()
                // 截图文件名（给了就在目标时刻截一张 mpv 自己的合成帧）
                val shotName = args?.getString("shot")

                val source = SmbByteSource.open(
                    address = parseAddress(host, share),
                    credential = SmbCredential(
                        username = args?.getString("smbUser") ?: "",
                        password = args?.getString("smbPass") ?: "",
                    ),
                    filePath = path,
                )
                texture = SurfaceTexture(0)
                surface = Surface(texture)

                session = MpvPlaybackSession(targetContext, source)
                val ready = CountDownLatch(1)
                var result = -1
                session.start(surface) { r ->
                    result = r
                    ready.countDown()
                }
                check(ready.await(60, TimeUnit.SECONDS)) { "起播超时（60 s）" }
                check(result == 0) { "起播失败：${session.lastError}" }
                log.append(" START ok")

                // ---- 选轨（**先于 seek**）----
                //
                // 【为什么顺序不能反】mpv 切字幕轨时会做一次轨道刷新，
                // 把播放位置打回该轨自己的起点（真机日志：refresh seek to 3.000000）。
                // 先 seek 后选轨 ⇒ 位置被打回 ⇒ 取证落在没有字幕的时刻 ⇒ 误判。
                check(session.selectSubtitle(subtitleId)) {
                    "选字幕轨失败（id=$subtitleId）：${session.lastError}"
                }
                Thread.sleep(1_500)
                log.append(" SID $subtitleId")

                // ---- seek 到目标时刻 ----
                session.seekTo((at * 1_000_000L).toLong())
                Thread.sleep(3_000)
                log.append(" AT ${at}s")

                // ---- 读 mpv 的属性（全部硬证据）----
                val native = session.native()
                fun prop(name: String): String? = native.property(name)

                val sid = prop("sid")
                val subText = prop("sub-text")
                val subStart = prop("sub-start")
                val subEnd = prop("sub-end")
                val visibility = prop("sub-visibility")
                val osdW = prop("osd-dimensions/w")
                val osdH = prop("osd-dimensions/h")
                val osdMt = prop("osd-dimensions/mt")
                val osdMb = prop("osd-dimensions/mb")
                val osdAspect = prop("osd-dimensions/aspect")

                log.append(" sid=$sid vis=$visibility")
                log.append(" subText=${subText?.take(40)?.replace("\n", "\\n") ?: "<null>"}")
                log.append(" subStart=$subStart subEnd=$subEnd")
                log.append(" osd=${osdW}x$osdH aspect=$osdAspect mt=$osdMt mb=$osdMb")

                // ---- 轨道目录：分类 + external ----
                val catalog = session.tracks
                val subs = catalog.subtitleTracks()
                log.append(
                    " SUBTRACKS " + subs.joinToString(";") {
                        "id=${it.id} lang=${it.lang} codec=${it.codec} ext=${it.external}"
                    },
                )
                check(subs.isNotEmpty()) { "没有字幕轨" }

                // ---- 判据 1：必须选中了字幕轨 ----
                check(sid?.toIntOrNull() == subtitleId) {
                    "sid 不是 $subtitleId（读到 $sid）"
                }

                // ---- 判据 2：字幕可见 ----
                check(visibility != "no") { "sub-visibility=no，字幕被隐藏" }

                // ---- 判据 3：OSD 尺寸（**只作记录，不作判据**）----
                //
                // 【为什么不能拿它当判据】本测试用的是 SurfaceTexture(0) 虚拟纹理，
                // 它**没有消费者** ⇒ 尺寸退化成 1x1 ⇒ mpv 的 VO 窗口也是 1x1。
                // 而位图字幕正是按 OSD 尺寸缩放的，所以在这个测试里
                // 字幕必然缩成一个点 —— **这是测试环境造成的，不是生产路径的问题**。
                // 真实 Surface（播放页）上的 OSD 尺寸见 status.md 的真机日志：
                //   Window size: 2320x956 / OSD borders: l=19 t=0 r=19 b=0
                //
                // 要在真 Surface 上判字幕，必须走播放页 + adb screencap。
                val w = osdW?.toIntOrNull() ?: 0
                val h = osdH?.toIntOrNull() ?: 0
                log.append(" osdSize=${w}x$h（1x1 = 虚拟纹理退化，非生产路径）")

                // ---- 判据 4：字幕包区间 —— **这一条才是"有没有内容"的硬证据** ----
                //
                // sub-start/sub-end 给的是**当前选中字幕轨**的包覆盖区间（秒）。
                // 若目标时刻落在区间外 ⇒ **该时刻本来就没有字幕**，
                // 此时"画面上没有字幕"是**正确行为**，不是 bug。
                val start = subStart?.toDoubleOrNull()
                val end = subEnd?.toDoubleOrNull()
                if (start != null && end != null) {
                    val covers = at >= start && at <= end
                    log.append(" COVERS_AT $at in [$start,$end] = $covers")
                    log.append(" PACKET_WIDTH_MS ${((end - start) * 1000).toInt()}")
                    if (!covers) {
                        // 不覆盖 ⇒ 目标时刻无字幕。**这不是失败**，是事实。
                        // 但必须显式说出来，否则下一个人又会当成"渲染问题"去查渲染层。
                        report += "NOTE 目标时刻 $at s 不在字幕包区间 [$start,$end] 内" +
                            " ⇒ 该时刻本就没有字幕（不是渲染失败）"
                    }
                } else {
                    log.append(" COVERS_AT unknown（读不到 sub-start/sub-end）")
                }

                // ---- 扫描：把整片里"有字幕"的窗口找出来 ----
                //
                // 【为什么要扫】PGS 是**位图字幕**，一包一张图。
                // 只测一个时刻得到的是"那几十毫秒有没有"，回答不了"字幕功能通不通"。
                //
                // 【为什么用"连续播放采样"而不是"逐点 seek"】
                // 逐点 seek 有两个问题：① seek 是异步的，落定需要时间；
                // ② seek 会打断字幕解码器的状态（实测：连续 seek 扫描 0~120 s **零命中**，
                // 而同一时刻单独测却有 sub-start/sub-end）。
                // 连续播放不打断解码器状态，采样到的就是**真实播放时**的字幕存在性。
                val scan = (args?.getString("scanFrom") ?: "0").toDouble()
                val scanTo = (args?.getString("scanTo") ?: "0").toDouble()
                if (scanTo > scan) {
                    // 回到扫描起点，然后**连续播放**过去，边播边采样。
                    session.seekTo((scan * 1_000_000L).toLong())
                    Thread.sleep(1_500)
                    val native2 = session.native()
                    var hits = 0
                    var samples = 0
                    var firstStart = -1.0
                    var lastEnd = -1.0
                    val windows = StringBuilder()
                    var t = scan
                    while (t <= scanTo) {
                        val s = native2.property("sub-start")?.toDoubleOrNull()
                        val e = native2.property("sub-end")?.toDoubleOrNull()
                        samples++
                        if (s != null && e != null && e > s) {
                            hits++
                            if (firstStart < 0) firstStart = s
                            if (e > lastEnd) lastEnd = e
                            if (hits <= 10) {
                                windows.append(" [${fmt(s)}..${fmt(e)}]")
                            }
                        }
                        Thread.sleep(200)
                        t += 0.2
                    }
                    log.append(" SCAN $scan..$scanTo samples=$samples hits=$hits")
                    log.append(" FIRST=${fmt(firstStart)} LAST=${fmt(lastEnd)}")
                    log.append(" WINDOWS$windows")
                    check(hits > 0) {
                        "连续播放 $scan..$scanTo s（采样 $samples 次）**没采到任何字幕包** —— " +
                        "字幕轨存在但整段没有内容"
                    }
                    report += "SCAN hits=$hits/$samples first=${fmt(firstStart)} last=${fmt(lastEnd)}"
                }

                // ---- 关键判据：让 **mpv 自己**把字幕画进一张 PNG ----
                //
                // 【为什么这是决定性的】
                // mpv 的 `screenshot-to-file <file> window` 走的是
                // `player/screenshot.c:330` 的 **MODE_FULL_WINDOW** 分支：
                //   struct mp_osd_res res = osd_get_vo_res(mpctx->video_out->osd);
                //   osd_draw_on_image(mpctx->osd, res, mpctx->playback_pts,
                //                     OSD_DRAW_SUB_ONLY, image);
                // 它**独立地把字幕画进图片**，**不依赖 cur_frame**。
                //
                // 于是这张图能回答一个截图回答不了的问题：
                //   · 图里有字幕 ⇒ mpv 内部渲染正常 ⇒ 问题在"送到 Surface"这一步；
                //   · 图里没字幕 ⇒ mpv 内部就没生成字幕 ⇒ 问题在解码/选轨/时刻。
                //
                // 【旧交接文档的判断错在哪】它说 render API 下 screenshot-to-file 截不到画面
                // （cur_frame 是空的）—— 那是 **MODE_VIDEO** 的行为；
                // 而 **window 模式不依赖 cur_frame**，旧文档把两种模式混为一谈了。
                if (shotName != null) {
                    val outDir = File(targetContext.filesDir, "p253-sub-diag").apply { mkdirs() }
                    val file = File(outDir, shotName)
                    file.delete()
                    val ok = session.command("screenshot-to-file", "${file.absolutePath} window")
                    Thread.sleep(1_500)
                    log.append(" SHOT cmd=$ok exists=${file.isFile} bytes=${if (file.isFile) file.length() else -1}")
                    check(ok) { "screenshot-to-file 失败" }
                    check(file.isFile && file.length() > 1_000) {
                        "截图无效（${file.length()} 字节）"
                    }
                    log.append(" SHOTPATH ${file.absolutePath}")
                }

                log.append(" OUTDIR ${File(targetContext.filesDir, "p253-sub-diag").absolutePath}")
            }
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString(
                    "stream",
                    report.joinToString("\n") + "\n" + log + "\nFAIL ${e.stackTraceToString()}",
                )
            })
            return
        } finally {
            runCatching { session?.stop() }
            runCatching { session?.awaitClosed(5_000) }
            runCatching { surface?.release() }
            runCatching { texture?.release() }
        }
        finish(Activity.RESULT_OK, Bundle().apply {
            putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n") + "\n" + log)
        })
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }

    /** 秒数格式化（保留 3 位小数）。 */
    private fun fmt(v: Double): String = String.format("%.3f", v)

    /** 解析 `host` 或 `host:port` 形式的地址（平板经 PC 转发时端口不是 445）。 */
    private fun parseAddress(host: String, share: String): SmbAddress {
        val colon = host.lastIndexOf(':')
        if (colon > 0 && host.indexOf(':') == colon) {
            val h = host.substring(0, colon)
            val p = host.substring(colon + 1).toIntOrNull()
            if (p != null && p in 1..65535) {
                return SmbAddress(h, p, share)
            }
        }
        return SmbAddress(host, SmbAddress.DEFAULT_PORT, share)
    }
}
