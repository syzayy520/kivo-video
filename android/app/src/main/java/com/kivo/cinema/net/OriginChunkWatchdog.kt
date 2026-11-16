package com.kivo.cinema.net

/**
 * 291：**单块读取的吞吐看门狗** —— 识别「连接退化」，让上层换一条连接重开。
 *
 * ## 它解决什么（真机 p290-live，2026-11-15 夜间公网源）
 *
 * 播放到 236 s 时主链路冻结 **127.8 s**：那一块 4 MiB 的表头等了 10.1 s，
 * 之后整个窗口内**零** `open fail` / `read fail` / `eof unexpected`，
 * 但 demux cache 从 `fw=3.9M / dur=13.2s` 一路掉到 `fw=0.0M / dur=0.0s / underrun=true`。
 * 同一窗口只有保温 ping 在跑（52 次），主 chunk 一条都没有。
 *
 * 根因不是「带宽略低」：片源平均码率 501 KB/s，源站当时仍以约 60 KB/s 在吐字节。
 * [HttpOriginClient.OriginStream.readInto] 只受 `soTimeout = 30 s` 约束，
 * 而那是**单次 read() 拿不到数据**的超时 —— 只要源站每 <30 s 吐一点字节，
 * `read()` 就一直返回正数，`OriginConnectionLease` 便永不 abort、永不重开，
 * 也**永不留下一行日志**。一条退化连接就这样把整条播放链路拖死两分钟
 * （B 臂 keepalive 的 124.1 s 冻结是同一机制：复用连接用久后吞吐塌了）。
 *
 * 这正是 275 §5.2「限制每请求超时」缺的那一半：
 * 已有的是**无数据超时**，缺的是**整块进度/吞吐下限**。
 *
 * ## 判据为什么这样定（保守，不误伤整体慢源站）
 *
 * 连续 [STALL_WINDOW_MS] 内平均吞吐低于 [MIN_RATE_BPS] 才判退化。
 * 下限 80 KB/s 远低于实测的正常慢源站（A 臂 566 / 当前会话 417 KB/s），
 * 只覆盖「连接退化到几十 KB/s」这一档。
 *
 * 【为什么不按绝对耗时判】源站**整体**慢时，换连接只会反复付 TLS 握手
 * （真机新建表头 989–5680 ms），比留在原连接更慢。看门狗只抓
 * 「单条连接明显比同源站其它连接慢」的情形，按**实际读到的字节**判定，
 * 与单次 read 阻塞多久解耦。
 *
 * 触发后有 [REARM_COOLDOWN_MS] 冷却：换到新连接先给它一段跑道，
 * 不拿旧窗口的字节数把新连接也判死。
 *
 * ## 开关
 *
 * `adb shell setprop debug.kivo.stallWatchdog off` ⇒ 完全关闭（A/B 对照臂）。
 * 关闭时 [note] 恒返回 false，既有取数行为一字不变。
 */
internal class OriginChunkWatchdog(private val label: String = "play") {

    private val enabled: Boolean = readSystemProperty(PROPERTY)?.trim()?.lowercase() != "off"

    /** 当前统计窗口的起点（System.currentTimeMillis）。 */
    private var windowStartMs = 0L

    /** 当前窗口内累计读到的字节。 */
    private var windowBytes = 0L

    /** 上次判定退化的时刻，用于冷却。 */
    private var lastTriggerMs = 0L

    /** 累计触发次数（只进日志，用来复算这一轮被救了几次）。 */
    private var triggers = 0

    init {
        if (!enabled) NetDiag.log("[$label] origin 吞吐看门狗已关闭（A/B 对照臂）")
    }

    /** 换块 / 开新连接时重置窗口；累计触发次数保留。 */
    @Synchronized
    fun reset(nowMs: Long = System.currentTimeMillis()) {
        windowStartMs = nowMs
        windowBytes = 0L
    }

    /**
     * 喂一次成功读取的字节数。
     *
     * @return true = 这条连接已退化，调用方应 `abort()` 并从当前写入位置重开一块。
     *         该判定**不计入连续失败**（换连接是自救，不是故障）。
     */
    @Synchronized
    fun note(bytes: Int, nowMs: Long = System.currentTimeMillis()): Boolean {
        if (!enabled || bytes <= 0) return false

        // 冷却期：只把窗口推进，不判定 —— 新连接需要先拿到一条跑道。
        if (nowMs - lastTriggerMs < REARM_COOLDOWN_MS) {
            windowStartMs = nowMs
            windowBytes = bytes.toLong()
            return false
        }

        windowBytes += bytes
        val elapsed = nowMs - windowStartMs
        if (elapsed < STALL_WINDOW_MS) return false

        val observed = windowBytes
        val rateBps = observed * 1000L / elapsed
        windowStartMs = nowMs
        windowBytes = 0L
        if (rateBps >= MIN_RATE_BPS) return false

        triggers++
        lastTriggerMs = nowMs
        NetDiag.log(
            "[$label] origin 连接退化：${elapsed}ms 仅读到 ${observed / 1024}KiB " +
                "(${rateBps / 1024}KB/s < ${MIN_RATE_BPS / 1024}KB/s)，第 $triggers 次换连接重开",
        )
        return true
    }

    private fun readSystemProperty(name: String): String? = try {
        val clazz = Class.forName("android.os.SystemProperties")
        clazz.getMethod("get", String::class.java).invoke(null, name) as? String
    } catch (e: Exception) {
        null
    }

    companion object {
        private const val PROPERTY = "debug.kivo.stallWatchdog"

        /** 统计窗口：连续这么久的低速才判退化（给正常抖动留余量）。 */
        private const val STALL_WINDOW_MS = 6_000L

        /**
         * 吞吐下限 80 KB/s。
         *
         * 【为什么是 80】片源 501 KB/s；正常慢源站实测 417–566 KB/s。
         * 80 KB/s 只有它们的 1/5，只有「连接明显退化」才会掉到这一档，
         * 不会把源站整体慢误判成单连接故障。
         */
        private const val MIN_RATE_BPS = 80L * 1024L

        /** 触发后的冷却：新连接至少先跑这么久再重新判定。 */
        private const val REARM_COOLDOWN_MS = 12_000L
    }
}
