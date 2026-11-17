package com.kivo.cinema.playback

import android.content.Context
import com.kivo.cinema.mpv.MpvSession
import java.io.File

/**
 * 345 · 内核健康度的**时间序列**（soak）。
 *
 * 单份 `files/metrics/<label>.json` 只能回答"这场播放最后是什么样"，
 * 回答不了"丢帧是不是在持续累积"——那正是长稳判据要的。
 * 本文件按固定周期把 [MpvHealthProbe] 的快照追加成 NDJSON，一行一个采样点。
 *
 * 边界：只负责「节奏 + 落行」，字段定义与取值全在 [MpvHealthProbe]；
 * 不碰 [PlaybackMetrics] 的单文件格式（那是"最后一份"，两者互补而不是替代）。
 */
internal object MpvHealthSoak {

    /** 5 s 一个点：240 s 的验收窗口有 48 个点，够看趋势；又不至于把文件写大。 */
    private const val PERIOD_MS = 5_000L

    /** 单文件上限：防长时间挂机把存储写满（4 MB ≈ 4 万个采样点）。 */
    private const val MAX_BYTES = 4L * 1024 * 1024

    @Volatile private var out: File? = null
    @Volatile private var startAt = 0L
    @Volatile private var lastAt = 0L
    @Volatile private var truncated = false

    /**
     * 每场播放开始时调（由 [PlaybackMetrics.begin] 转发），清空上一场的序列。
     *
     * @param context 为 null（无上下文）时停采 —— 不静默写到别的目录。
     */
    fun begin(context: Context?, label: String) {
        startAt = System.currentTimeMillis()
        lastAt = 0L
        truncated = false
        out = context?.let {
            // 文件名与 metrics 的 `<label>.json` 同一命名规则，只多一个 -health 后缀。
            val safe = label.ifBlank { "play" }.replace(Regex("[^A-Za-z0-9._-]"), "_")
            File(File(it.filesDir, "metrics").apply { mkdirs() }, "$safe-health.ndjson")
                .also { f -> runCatching { f.writeText("") } }
        }
    }

    /**
     * 主线程每 1 s 调一次，内部按 [PERIOD_MS] 节流。
     *
     * 【为什么必须主线程】与 [MpvHealthProbe.sample] 同因：vo 类属性
     * （`estimated-vf-fps` / `mistimed-frame-count`）从 `KivoCacheDiag`
     * 后台线程读恒 null，只有主线程能读到。
     */
    fun maybeAppend(native: MpvSession) {
        val f = out ?: return
        if (startAt == 0L || truncated) return
        val t = System.currentTimeMillis() - startAt
        if (t - lastAt < PERIOD_MS) return
        lastAt = t
        runCatching {
            if (f.length() > MAX_BYTES) {
                // 截断要留痕：静默停止会让人以为"后面没有丢帧"。
                truncated = true
                f.appendText("{\"t_ms\":$t,\"truncated\":\"file_over_${MAX_BYTES}_bytes\"}\n")
                return
            }
            f.appendText("{\"t_ms\":$t,\"health\":" + MpvHealthProbe.flat(native) + "}\n")
        }
    }
}
