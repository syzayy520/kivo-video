package com.kivo.cinema.mpv

import android.app.Activity
import android.app.Instrumentation
import android.graphics.SurfaceTexture
import android.os.Bundle
import android.os.SystemClock
import android.view.Surface
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbByteSource
import com.kivo.cinema.smb.SmbCredential
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * 253-P6-2 验收：**字幕真的画在画面上了吗**。
 *
 * ## 为什么用 mpv 自己的截图，而不是手机截图
 * 内核测试用的 Surface 是一张**虚拟纹理**（`SurfaceTexture(0)`），
 * 它从来没有上过屏 —— 拿手机截图只会拍到桌面。
 * 而 mpv 有 `screenshot-to-file` 命令，能把**它自己合成好的那一帧**
 * （视频 + OSD + 字幕）写成 PNG。
 * 这比截屏更硬：截屏还要经过系统的合成器，而这张图直接来自内核。
 *
 * ## 【关键】三种截图的区别
 * mpv 的截图有三种模式，含义完全不同：
 *  · `video`     —— 只有视频层，**没有字幕**；
 *  · `subtitles` —— 只有字幕层，**没有视频**；
 *  · `window`    —— 合成后的完整画面（视频 + 字幕 + OSD）。
 * 拿 `video` 模式去证明"字幕显示了"是自欺欺人 —— 它本来就不含字幕。
 * 本测试固定用 `window`。
 *
 * ## 判据
 *  1. 能列出片源自带的字幕轨（Shelter 有 5 条中文 PGS）；
 *  2. 选中其中一条后，`window` 截图与 `video` 截图**内容不同**
 *     —— 不同就说明字幕层被合成进画面了；
 *  3. 外挂 SRT 同样能被 mpv 渲染（走 libass 文本通路，与 PGS 位图通路不同）。
 *
 * ## 谁来看像素
 * 本测试只负责产出 PNG 与客观差异（md5），**不做像素判读** ——
 * 判读由人看图完成（把 PNG 拉回宿主机）。测试只断言"两张图不一样"，
 * 因为那一条不依赖任何主观标准。
 */
class MpvSubtitleEvidenceInstrumentation : Instrumentation() {

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
        super.onStart()
        val log = StringBuilder()
        var texture: SurfaceTexture? = null
        var surface: Surface? = null
        var session: MpvPlaybackSession? = null
        try {
            case("字幕平面：内置 PGS + 外挂文本字幕，都进画面") {
                val host = args?.getString("smbHost") ?: "192.168.5.1"
                val share = args?.getString("smbShare") ?: "XiaoMi"
                val path = args?.getString("smbPath")
                checkNotNull(path) { "缺少 -e smbPath" }

                val source = SmbByteSource.open(
                    address = parseAddress(host, share),
                    credential = SmbCredential(
                        username = args?.getString("smbUser") ?: "",
                        password = args?.getString("smbPass") ?: "",
                    ),
                    filePath = path,
                )
                texture = SurfaceTexture(0).apply {
                    // 【为什么必须给尺寸】SurfaceTexture(0) 是一张**没有消费者的虚拟纹理**，
                    // 它的默认 buffer size 是 0×0。mpv 从窗口尺寸推算输出分辨率，
                    // 拿到 0 就退化成 1×1 —— 截出来的 PNG 只有 166 字节。
                    // 【那个 166 字节还差点骗过判据】所有图都是 1×1，
                    // 但 PNG 里带了不同的元数据，md5 不一样 ⇒ "图与图不同"成立，
                    // 而图其实都是空的。所以判据必须同时看**文件大小**。
                    setDefaultBufferSize(1920, 1080)
                }
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

                val outDir = File(targetContext.filesDir, "p253-subtitle").apply { mkdirs() }
                val catalog = session.tracks
                val subTracks = catalog.subtitleTracks()
                log.append(
                    " SUBTRACKS " + subTracks.joinToString(";") {
                        "id=${it.id} lang=${it.lang} codec=${it.codec} title=${it.title.take(24)}"
                    },
                )
                check(subTracks.isNotEmpty()) { "片源没有字幕轨" }

                // ---- 取证点：几个大概率有字幕/对白的位置 ----
                val probeSeconds = listOf(120L, 600L, 1500L, 2400L)

                // 基准：不开字幕时的 `window` 截图。
                catalog.disableTrack("sub")
                val baseline = captureAll(session, outDir, "nosub", probeSeconds)
                log.append(" BASELINE " + baseline.joinToString(",") { it.name })

                // ---- 内置字幕（Shelter 的中文 PGS）----
                val builtin = subTracks.firstOrNull { !it.external }
                    ?: error("没有内置字幕轨")
                check(session.selectSubtitle(builtin.id)) {
                    "选内置字幕失败（id=${builtin.id}）"
                }
                Thread.sleep(1_200)
                val withBuiltin = captureAll(session, outDir, "builtin-${builtin.id}", probeSeconds)
                log.append(" BUILTIN " + withBuiltin.joinToString(",") { it.name })
                // 与基准不同 = 字幕层真的进了合成画面。
                val builtinDiffers = withBuiltin.zip(baseline).any { (a, b) ->
                    a.lengthBytes() != b.lengthBytes() || a.md5() != b.md5()
                }
                check(builtinDiffers) {
                    "选了内置字幕后画面与不开字幕时完全相同 —— 字幕没进画面"
                }

                // ---- 外挂文本字幕（走 libass，与 PGS 是两条不同的通路）----
                val subFile = writeTestSubtitle()
                check(session.loadExternalSubtitle(subFile)) {
                    "外挂字幕挂载失败：${session.lastError}"
                }
                Thread.sleep(1_000)
                val externalIds = catalog.externalSubtitleIds()
                check(externalIds.isNotEmpty()) { "外挂字幕没出现在轨道列表里" }
                val extId = externalIds.first()
                check(session.selectSubtitle(extId)) { "选外挂字幕失败（id=$extId）" }
                Thread.sleep(1_200)
                val withExternal = captureAll(session, outDir, "external-$extId", probeSeconds)
                log.append(" EXTERNAL " + withExternal.joinToString(",") { it.name })
                val externalDiffers = withExternal.zip(baseline).any { (a, b) ->
                    a.lengthBytes() != b.lengthBytes() || a.md5() != b.md5()
                }
                check(externalDiffers) {
                    "选了外挂文本字幕后画面与不开字幕时完全相同 —— 文本字幕没进画面"
                }

                log.append(" OUTDIR ${outDir.absolutePath}")
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

    /**
     * 在每个取证点截一张 `window` 模式的图。
     *
     * 【为什么要等】seek 之后 mpv 要解码目标帧、
     * 再把字幕层与视频层合成 —— 立刻截图会拍到还没合成完的帧。
     *
     * 【为什么要卡大小】1×1 的空 PNG 只有 166 字节，而它与另一张空 PNG
     * 的 md5 也可能不同（PNG 元数据）。只比 md5 会得到"图不一样"的假阳性。
     * 一张 1920×1080 的真实画面至少几十 KB，所以大小是更硬的判据。
     */
    private fun captureAll(
        session: MpvPlaybackSession,
        outDir: File,
        tag: String,
        seconds: List<Long>,
    ): List<File> {
        val files = mutableListOf<File>()
        for (at in seconds) {
            session.seekTo(at * 1_000_000L)
            // 等落位 + 解码 + 字幕合成。2 秒足够（实测落位误差 < 1 秒）。
            Thread.sleep(2_500)
            val file = File(outDir, "$tag-$at.png")
            file.delete()
            val ok = session.command(
                "screenshot-to-file",
                "${file.absolutePath} window",
            )
            if (ok) {
                // mpv 写文件是异步的，给它一点落盘时间。
                Thread.sleep(800)
            }
            check(file.isFile && file.length() > MIN_PNG_BYTES) {
                "截图无效（${file.name}，${if (file.isFile) file.length() else -1} 字节，" +
                    "下限 $MIN_PNG_BYTES）—— mpv 没画出真实画面"
            }
            files += file
        }
        return files
    }

    companion object {
        /**
         * 一张真实画面的 PNG 最小字节数。
         *
         * 1×1 的空 PNG 是 166 字节；1920×1080 的真实画面压缩后至少几十 KB。
         * 取 10 KB 做门槛：低于它的图一定是空的。
         */
        private const val MIN_PNG_BYTES = 10_000L
    }

    private fun File.lengthBytes(): Long = if (isFile) length() else -1L

    /** 文件内容的 md5（用来客观判断"两张图不一样"）。 */
    private fun File.md5(): String = runCatching {
        val digest = java.security.MessageDigest.getInstance("MD5")
        inputStream().use { stream ->
            val buffer = ByteArray(64 * 1024)
            while (true) {
                val read = stream.read(buffer)
                if (read <= 0) break
                digest.update(buffer, 0, read)
            }
        }
        digest.digest().joinToString("") { "%02x".format(it) }
    }.getOrDefault("")

    /**
     * 写一条测试外挂字幕。
     *
     * 【内容为什么用 ASCII】位图字幕与文本字幕是两条通路，
     * 这里测的是**文本**通路。ASCII 内容足以证明"文本字幕被渲染了"，
     * 而不引入"中文字形缺失"这个与本测试无关的变量。
     */
    private fun writeTestSubtitle(): File {
        val file = File(targetContext.filesDir, "kivo-test-sub.srt")
        file.writeText(
            buildString {
                append("1\n")
                append("00:02:00,000 --> 00:02:20,000\n")
                append("KIVO-MPV-SUBTITLE-PROBE-A\n")
                append("\n")
                append("2\n")
                append("00:10:00,000 --> 00:10:20,000\n")
                append("KIVO-MPV-SUBTITLE-PROBE-B\n")
                append("\n")
                append("3\n")
                append("00:25:00,000 --> 00:25:20,000\n")
                append("KIVO-MPV-SUBTITLE-PROBE-C\n")
                append("\n")
                append("4\n")
                append("00:40:00,000 --> 00:40:20,000\n")
                append("KIVO-MPV-SUBTITLE-PROBE-D\n")
                append("\n")
            },
        )
        return file
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }

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
