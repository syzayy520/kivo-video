package com.kivo.cinema.mpv

import android.content.Context

/**
 * 253 家族：libmpv 内核的 Kotlin 门面。
 *
 * **只做一件事**：把 native 会话包装成 Kotlin 侧好用的对象，并提供错误传播。
 * 不含任何播放策略（那是上层的事），也不含任何 UI。
 *
 * 与旧内核的关系：两条链路**完全独立**，由播放页的引擎开关选择，互不引用。
 * 旧内核见 `com.kivo.cinema.bridge.PlaybackSession`。
 */
object MpvNative {
    init {
        // libmpv 适配层是**独立的 .so**（kivo_mpv），不是 kivo_player 的一部分 ——
        // 家族边界见 docs/design/backend-core/253-mpv-kernel-family-tree.md。
        // 旧内核不加载它；两条链路互不影响。
        System.loadLibrary("kivo_mpv")
    }

    /**
     * 创建内核会话。
     *
     * @param appContext Android Context（applicationContext 即可）。
     *   **必须传**：mpv 的硬解与音频输出要通过它拿到 JavaVM 与 MediaCodec 上下文。
     */
    @JvmStatic external fun nativeCreate(appContext: Any): Long
    @JvmStatic external fun nativeInitialize(handle: Long, vo: String, ao: String, logFile: String): Int
    @JvmStatic external fun nativeAttachSurface(handle: Long, surface: Any): Int
    @JvmStatic external fun nativeAttachCapture(handle: Long, width: Int, height: Int): Int
    @JvmStatic external fun nativeReadCaptureFrame(handle: Long): ByteArray?
    @JvmStatic external fun nativeCaptureFramePts(handle: Long): Double
    @JvmStatic external fun nativeCaptureFrameGen(handle: Long): Long
    @JvmStatic external fun nativeLoadFile(handle: Long, url: String): Int
    @JvmStatic external fun nativeSeek(handle: Long, position: Double, exact: Boolean): Int
    @JvmStatic external fun nativeCommand(handle: Long, name: String, args: String): Int
    @JvmStatic external fun nativePositionSeconds(handle: Long): Double
    @JvmStatic external fun nativeDurationSeconds(handle: Long): Double

    /**
     * 读一个内核属性（字符串形式）。
     *
     * 支持 mpv 的**属性路径**语法：`track-list/count`、`track-list/0/title`。
     * 读不到时返回 null —— 属性在某些状态下本来就不存在
     * （媒体没加载时读 duration），这不是错误。
     */
    @JvmStatic external fun nativeGetProperty(handle: Long, name: String): String?

    /** 写一个内核属性（字符串形式）。典型用途：speed / aid / sid / vid。 */
    @JvmStatic external fun nativeSetProperty(handle: Long, name: String, value: String): Int

    @JvmStatic external fun nativeRelease(handle: Long)
}

/**
 * 一个 libmpv 会话。
 *
 * 线程纪律：所有方法必须在**同一个**线程调用（native 侧用一把全局锁串行化），
 * 但跨线程调用是安全的 —— 不会崩，只是会被排队。
 */
class MpvSession private constructor(private val handle: Long) : AutoCloseable {

    /** 底层句柄，供诊断用。 */
    val nativeHandle: Long get() = handle

    /**
     * 初始化内核。
     *
     * @param vo mpv 的视频输出名。Android 上用 "libmpv"（我们用 mpv_render_context 自己渲染）。
     * @param ao mpv 的音频输出名。**留空 = "null"（不出声）** ——
     *   上游 mpv 的 `ao=audiotrack` 需要 libmpv 注册到 JavaVM 才能用，
     *   我们编的是上游版本，直接用会在 mpv_initialize 内部崩。
     *   音频输出改由我们自己的 AAudio 层承担（P2）。
     * @param logFile mpv 日志落盘路径。留空 = 不落盘。
     *   **崩溃定位的关键**：这台设备上 adb logcat 读不到 app 进程的日志，
     *   而 native 崩溃没有异常可捕获 —— 只能让 mpv 自己写日志出来。
     *
     * @throws IllegalStateException 初始化失败，**消息里带 mpv 给的原始原因**
     *   （例如 "mpv option rejected: vo=xxx (error setting option)"）。
     *
     *   【为什么不吞掉这个异常】旧实现用 runCatching 把它变成 false，
     *   于是上层只看到"起播失败"，看不到"哪个选项被拒" ——
     *   而 mpv 自己**不打印**被拒选项的任何消息。
     *   抛出带原因的异常，是让失败可诊断的唯一办法。
     */
    fun initialize(vo: String = "libmpv", ao: String = "", logFile: String = "") {
        try {
            MpvNative.nativeInitialize(handle, vo, ao, logFile)
        } catch (e: IllegalStateException) {
            throw IllegalStateException("mpv initialize failed: ${e.message}", e)
        }
    }

    /** 把 Surface 接到视频输出上。 */
    fun attachSurface(surface: Any) {
        MpvNative.nativeAttachSurface(handle, surface)
    }

    /** 离屏取帧目标（预览）：渲染进纹理 FBO 读回 CPU，不走 Surface/ImageReader。 */
    fun attachCapture(width: Int, height: Int) {
        MpvNative.nativeAttachCapture(handle, width, height)
    }

    /** 最新离屏帧 RGBA（top-down）；没有比上次更新的帧返回 null。 */
    fun readCaptureFrame(): ByteArray? = MpvNative.nativeReadCaptureFrame(handle)

    /** 最新离屏帧对应的 time-pos（秒），无帧 -1。仅取证日志用。 */
    fun captureFramePts(): Double = MpvNative.nativeCaptureFramePts(handle)

    /** 最新离屏帧的 seek 代数；> 发 seek 前读到的代数 = 本次 seek 的新帧。 */
    fun captureFrameGen(): Long = MpvNative.nativeCaptureFrameGen(handle)

    /** 打开一个媒体（本地路径或 URL，例如 `smb://host/share/path`）。 */
    fun loadFile(url: String) {
        MpvNative.nativeLoadFile(handle, url)
    }

    /**
     * 跳到指定位置（秒）。
     *
     * @param exact true = 精确到帧（mpv 的 `hr-seek=exact`，会解到目标帧）。
     *   false = 落在关键帧，立刻起播，但可能早最多一整个 GOP。
     */
    fun seek(positionSeconds: Double, exact: Boolean = false) {
        MpvNative.nativeSeek(handle, positionSeconds, exact)
    }

    /**
     * 下发一条 mpv 命令（如 `set pause yes`）。
     *
     * @return true = 命令被内核接受。
     */
    fun command(name: String, args: String = ""): Boolean =
        runCatching { MpvNative.nativeCommand(handle, name, args) == 0 }.getOrDefault(false)

    /** 当前播放位置（秒）；读不到返回 -1。 */
    fun positionSeconds(): Double = MpvNative.nativePositionSeconds(handle)

    /** 总时长（秒）；读不到返回 -1。 */
    fun durationSeconds(): Double = MpvNative.nativeDurationSeconds(handle)

    /**
     * 读一个内核属性（字符串形式）。
     *
     * 支持 mpv 的属性路径语法：`track-list/count`、`track-list/0/title`。
     *
     * @return 属性值；读不到返回 null。**读不到不是错误** ——
     *   有些属性在某些状态下本来就不存在（媒体没加载时读 duration）。
     */
    fun property(name: String): String? =
        runCatching { MpvNative.nativeGetProperty(handle, name) }.getOrNull()

    /**
     * 写一个内核属性（字符串形式）。
     *
     * 典型用途：`speed`（倍速）、`aid`/`sid`/`vid`（选轨）。
     *
     * @return true = 写入成功。
     */
    fun setProperty(name: String, value: String): Boolean =
        runCatching { MpvNative.nativeSetProperty(handle, name, value) == 0 }.getOrDefault(false)

    override fun close() {
        MpvNative.nativeRelease(handle)
    }

    companion object {
        /**
         * 创建一个会话。失败时抛 [IllegalStateException]（原因在消息里）。
         *
         * @param appContext Android Context。**必填** —— native 侧要用它注册
         *   JavaVM 给 FFmpeg，否则 mpv 的硬解（mediacodec）与音频输出
         *   （audiotrack）都拿不到 Java 环境。
         */
        fun create(appContext: Context): MpvSession {
            val handle = MpvNative.nativeCreate(appContext.applicationContext)
            if (handle == 0L) {
                throw IllegalStateException("mpv_create failed")
            }
            return MpvSession(handle)
        }
    }
}
