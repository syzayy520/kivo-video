package com.kivo.cinema.bridge

import android.view.Surface

/**
 * JNI 唯一入口。
 *
 * 职责边界（见 docs/design/backend-core/08-p12-family-tree.md 第四节）：
 * 只负责加载 native 库、声明方法签名、把 Java 侧平台数据转交给 native。
 * 不做任何解码、渲染或状态管理。
 */
object NativePlayer {

    init {
        System.loadLibrary("kivo_player")
    }

    /** 返回后端版本标识，用于确认 .so 装载成功。 */
    external fun nativeVersion(): String

    /**
     * 能力探测的 native 实现。
     *
     * @param outputJsonPath 证据落盘路径
     * @param inventory MediaCodecInventory 摊平的记录（TAB 分隔）
     * @return 0 成功；非 0 为错误码
     */
    private external fun nativeProbeCapabilities(
        outputJsonPath: String,
        inventory: Array<String>
    ): Int

    /**
     * 枚举设备视频解码能力，写出 JSON 证据。
     *
     * 之所以需要在 Java 侧先收集：NDK 的 AMediaCodecStore 要到 API 36 才有，
     * minSdk 26 用不了，见 MediaCodecInventory 注释。
     */
    fun probeCapabilities(outputJsonPath: String): Int {
        val inventory = MediaCodecInventory.collectVideoCodecs()
        return nativeProbeCapabilities(outputJsonPath, inventory.toTypedArray())
    }

    /**
     * 帧率能力的 native 实现（P13-A）。
     *
     * @param records `MediaFramerateInventory` 摊平的记录（TAB 分隔，8 字段）
     */
    private external fun nativeProbeFramerates(
        outputJsonPath: String,
        records: Array<String>
    ): Int

    /**
     * 枚举「codec × 分辨率」下的帧率上限，写出 JSON 证据。
     *
     * 与 [probeCapabilities] 的区别：那个回答"有哪些 codec"，这个回答
     * "这些 codec 在 4K 上还能跑多少 fps"。后者才是选片源规格的依据 ——
     * 能力矩阵里的 `max_width = 8192` 是尺寸上限，与帧率无关。
     */
    fun probeFramerates(outputJsonPath: String): Int {
        val records = MediaFramerateInventory.collect()
        return nativeProbeFramerates(outputJsonPath, records.toTypedArray())
    }

    // ---- 解码会话 ----------------------------------------------------------
    // 句柄是不透明指针。必须成对使用 create/destroy，否则会泄漏 Surface 引用。

    /** 创建解码会话，返回不透明句柄。 */
    external fun nativeSessionCreate(): Long

    /** 释放会话。会先停止解码线程，再释放 Surface 引用，顺序由 native 侧保证。 */
    external fun nativeSessionDestroy(handle: Long)

    /**
     * 打开媒体并把解码输出绑定到 surface。返回 0 表示成功。
     *
     * 传 fd 而不是路径：本机 Android 13 上 `AMediaExtractor_setDataSource` 收纯路径
     * 会返回 MALFORMED，而 fd 方式在所有版本可靠，也适配分区存储。
     * 调用方必须保证 fd 在整个会话期间有效。
     */
    external fun nativeSessionOpen(
        handle: Long,
        label: String,
        fd: Int,
        length: Long,
        surface: Surface
    ): Int

    /**
     * 换输出 Surface（P12-D1）。可以在播放过程中调用。
     *
     * @param surface 新 Surface；传 `null` 表示"面没了，先摘掉"。
     *
     * 传 null 之后 codec 切到 ByteBuffer 模式，解出的帧直接丢弃 —— 这段时间没有面可显示，
     * 帧不该堆在驱动里。**必须尽快补一个新的**，否则画面一直不回来。
     *
     * 换面不动会话状态（帧计数、节拍锚点、时钟全部连续），
     * 这是"恢复"与"从头重播"的分界。
     */
    external fun nativeSessionRebindSurface(handle: Long, label: String, surface: Surface?): Int

    /** 启动后台解码线程，立即返回。返回 0 表示成功。 */
    external fun nativeSessionStart(handle: Long): Int

    /**
     * 定位到指定时刻并重新起跑（P12-F1）。**必须在 [nativeSessionStart] 之后调用。**
     *
     * 走 `PREVIOUS_SYNC`，实际落点会 ≤ 目标；落点由证据 `seek.landing_us` 记录。
     * 会停掉并重建解码线程，**帧计数归零** —— 一轮里做过 seek 时，counters 描述的是最后一段。
     *
     * @return 0 成功；9 被拒绝（前置条件不满足）；10 执行失败。原因走 [nativeSessionLastError]
     */
    external fun nativeSessionSeek(handle: Long, targetUs: Long): Int

    /**
     * 开启按节拍呈现（P12-C5）。必须在 [nativeSessionStart] 之前调用。
     *
     * @param targetFps 期望帧率，仅作首帧兜底；真实帧时长由 pts 差值观测。
     * @param allowDrop 是否允许按节拍丢帧。false 用于"只睡不丢"的对照组。
     */
    external fun nativeSessionSetPacing(handle: Long, targetFps: Int, allowDrop: Boolean): Int

    /**
     * 选择流水线模式（P12-C6）与呈现排程方式（P12-C7）。必须在 [nativeSessionStart] 之前调用。
     *
     * @param split true = 送包与收帧各一个线程（实验路径）；false = 两段同线程交替（默认）。
     *        默认 merged 是 P12-C6 判据的回退结果，见 decode_session.hpp 的 PipelineMode 注释。
     * @param present 是否把帧交给 Surface。false 是"只解码不上屏"的诊断轮：
     *        它把解码吞吐与呈现链路（BufferQueue/合成器）的限速分开量。
     * @param atTime true = 不睡，把到期时刻交给 `releaseOutputBufferAtTime` 由合成器定时
     *        （**默认**）；false = CPU 睡到到期时刻再交（C5 结构，回退路径）。
     *        只在 present=true 且有节拍时有意义。默认 true 的依据见 23-p12c7-status.md §五。
     */
    external fun nativeSessionSetPipeline(
        handle: Long,
        split: Boolean,
        present: Boolean,
        atTime: Boolean,
    ): Int

    /**
     * 打开音轨并起流（P12-C8）。必须在 [nativeSessionSetPacing] 与 [nativeSessionStart] 之前调用。
     *
     * 成功之后节拍器改由**音频主时钟**驱动，证据里的 `clock_source` 才会是音频档；
     * 不调用它 = 纯视频播放（`clock_source = system`），行为与 C7 完全一致。
     *
     * @param fd 与视频**同一个文件另开的一个描述符**，调用方保证会话期间有效
     * @return 0 成功；非 0 表示失败，原因走 [nativeSessionLastError]
     */
    external fun nativeSessionOpenAudio(
        handle: Long,
        label: String,
        fd: Int,
        length: Long
    ): Int

    /**
     * 写 A/V 合并证据（`kivo.p12c.sync/1`）。**必须在 [nativeSessionStop] 之后调用** ——
     * 它要读节拍器的完整直方图，那个接口只在工作线程 join 之后才安全。
     */
    external fun nativeSessionWriteSyncStats(handle: Long, path: String): Int

    /** 请求停止并等待解码线程退出。 */
    external fun nativeSessionStop(handle: Long)

    /** 把当前统计写成 JSON 证据。返回 0 表示成功。 */
    external fun nativeSessionWriteStats(handle: Long, path: String): Int

    /** 一行可读摘要，用于日志与屏幕核对。 */
    external fun nativeSessionSummary(handle: Long): String

    /** 最近一次失败原因。本机 logcat 不可用，失败原因只能走这条通道回传。 */
    external fun nativeSessionLastError(handle: Long): String
}
