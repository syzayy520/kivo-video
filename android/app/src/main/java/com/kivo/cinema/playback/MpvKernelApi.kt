package com.kivo.cinema.playback

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.mpv.MpvSpeedController
import com.kivo.cinema.mpv.MpvTrackCatalog
import java.io.File

/**
 * 273 v16：**mpv 内核能力门面** —— 把内核的轨道/倍速/字幕/命令/属性
 * 翻译成上层能直接用的东西。
 *
 * ## 为什么单独立一个文件
 * 内核能力与"会话生命周期"是**两种变化原因**：
 * · 轨道/倍速/字幕的**语义**随 mpv 的属性定义走（`aid` / `sid` / `speed` / `track-list`…），
 *   与会话起停无关 —— 换一套内核语义，改的是这一层；
 * · 会话起停、进度、失败释放是**运行期**的事 —— 换一套生命周期，改的是会话那一层。
 * 混在一个文件里，两边互相迁就，文件必然超长（273 v16 之前会话是 385 行）。
 *
 * ## 失败怎么办
 * 内核读不到就报"尚未就绪"，**不编默认值**（铁律：不许伪造状态）；
 * 写不进去就把原因回报给会话（[reportError]），由上层显示给用户。
 */
internal class MpvKernelApi(
    private val sessionProvider: () -> MpvSession?,
    /** 失败原因回报（写进会话的 lastError，供上层显示）。 */
    private val reportError: (String) -> Unit,
) {

    /**
     * 取活着的内核会话。
     *
     * @throws IllegalStateException 会话还没起或已经释放。
     */
    private fun require(): MpvSession =
        sessionProvider() ?: throw IllegalStateException("内核会话尚未启动或已释放")

    /** 轨道目录（音轨/字幕轨枚举与选轨）。 */
    val tracks: MpvTrackCatalog get() = MpvTrackCatalog(require())

    /** 倍速控制。 */
    val speed: MpvSpeedController get() = MpvSpeedController(require())

    /**
     * 直接读 mpv 的原生属性（诊断用）。
     *
     * 【为什么需要这一层】轨道目录与倍速控制器都是**面向 UI 的结构化视图**，
     * 但排查内核问题需要 mpv 的**原始属性**（`sub-start` / `sub-end` / `osd-dimensions` 的子属性）。
     * 没有这一层，就只能靠截图猜"字幕有没有内容"——
     * 而"该时刻本来就没有字幕"和"字幕没渲染出来"在截图上长得一模一样。
     */
    fun native(): MpvSession = require()

    /**
     * 原生会话的**安全**版本：会话还没起时返回 null 而不是抛异常。
     *
     * 【为什么需要】UI 的**轮询**会在会话建立之前就开始跑
     * （比如顶栏网速每 1 秒读一次 `cache-speed`）。
     * 用 [native]（会抛）会让轮询任务崩掉、整个 Activity 挂掉 ——
     * 这正是第一版播放页 UI 实机测试时"Activity 起来又退回桌面"的原因。
     */
    fun nativeOrNull(): MpvSession? = sessionProvider()

    /**
     * 轨道目录的**安全**版本：会话还没起时返回 null 而不是抛异常。
     *
     * 【为什么需要】UI 的面板是**随时可能被打开**的 ——
     * 用户在起播完成前点"音轨"就会走到这里。
     * 用 [tracks]（会抛）会让面板直接崩掉；用这个则只是"暂时没有可选项"。
     */
    fun tracksOrNull(): MpvTrackCatalog? = sessionProvider()?.let { MpvTrackCatalog(it) }

    // ---- 字幕（规则在 [MpvSubtitleControl]，这里只转发） ----

    private fun subtitles(): MpvSubtitleControl = MpvSubtitleControl(require(), reportError)

    /** 字幕是否已启用（mpv 的 `sid` 有值即为启用）。 */
    val subtitlesEnabled: Boolean get() = subtitles().enabled

    /**
     * 挂载一条外挂字幕文件。
     *
     * @param file 本地字幕文件（.srt / .ass / .ssa）。
     * @return true = 挂载成功。
     */
    fun loadExternalSubtitle(file: File): Boolean = subtitles().loadExternalSubtitle(file)

    /**
     * 选中一条字幕轨。
     *
     * @param trackId 轨道 id（来自 [MpvTrackCatalog.tracks]）。
     */
    fun selectSubtitle(trackId: Int): Boolean = subtitles().selectSubtitle(trackId)

    /** 关闭字幕。 */
    fun disableSubtitles(): Boolean = subtitles().disableSubtitles()

    /** 当前字幕轨是否在**显示**（挂了 ≠ 选了 ≠ 可见，三态分开）。 */
    fun isSubtitleVisible(): Boolean = subtitles().isSubtitleVisible()

    // ---- 倍速与内核命令 ----

    /**
     * 下发一条内核命令。
     *
     * @param name 命令名（如 `screenshot-to-file`）。
     * @param args 参数（已按 mpv 命令行格式序列化）。
     * @return true = 命令被内核接受。
     *
     * 【为什么要透出命令】
     * 上层偶尔需要做"没有对应属性"的动作：截图、加载外挂字幕、
     * 切到下一条字幕轨。这些都是 mpv 的命令而不是属性。
     */
    fun command(name: String, args: String = ""): Boolean =
        runCatching { require().command(name, args) }.getOrDefault(false)

    /**
     * 设置倍速。
     *
     * @param ratio 倍速（会被夹到 0.25~4.0）。
     * @return true = 设置成功。
     */
    fun setSpeed(ratio: Double): Boolean {
        val ok = speed.setSpeed(ratio)
        if (!ok) reportError("倍速设置失败（$ratio）")
        return ok
    }

    /** 当前倍速。 */
    fun currentSpeed(): Double = speed.speed()

    /**
     * 当前倍速的**安全**版本：会话还没起时返回 null。
     *
     * 【为什么需要】UI 的面板在会话建立前就可能被打开，
     * 用 [currentSpeed]（会抛）会让界面崩掉。
     */
    fun currentSpeedOrNull(): Double? =
        sessionProvider()?.let { runCatching { it.property("speed")?.toDoubleOrNull() }.getOrNull() }

    /** 内核状态摘要，供诊断用。 */
    fun currentSummary(): String = sessionProvider()?.positionSeconds()?.toString().orEmpty()
}
