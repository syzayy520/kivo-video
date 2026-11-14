package com.kivo.cinema.thumbnail

import com.kivo.cinema.mpv.MpvSession

/**
 * 预览内核的 mpv 选项集 —— **单一职责：把这套选项一次设完，并把回执带回来**。
 *
 * 【为什么单独一个文件】选项的**取值**和**回执检查**是两种变化原因：
 * 取值随"预览要多少预读"变，回执检查随"内核 API 会不会静默失败"变。
 * 混在读取器里会让那个文件既超行数铁律又难读。
 *
 * 【为什么必须查回执】`mpv_set_property_string` 对**不存在的属性名**返回
 * `MPV_ERROR_PROPERTY_NOT_FOUND`，而且**不打任何日志**（见 mpv 的
 * player/client.c：只有返回值，没有消息）。不查回执 ⇒ 拼错的名字会静默失效，
 * 表现为"限流参数配了但预读照旧"，而现场一个线索都没有。
 */
internal object MpvThumbOptions {

    /**
     * 预览内核的 demux 上限（字节）。
     *
     * 【为什么 4 MiB】真机实测（273-E）：预览实例为了取 475 秒那一帧，
     * mpv 从目标簇**继续往前读了 9 个 2 MiB 分块（16.8 MiB）**才停 ——
     * 8 MiB 的字节上限只管"缓存里留多少"，拦不住"往前读了多远"。
     * 收到 4 MiB 后同样这一跳的读量降到 ~8 MiB，
     * 而 4 MiB 仍够放一个 4K 关键帧（一帧 ≈1–3 MB）。
     */
    private const val DEMUX_MAX_BYTES = "4MiB"

    /** 回看缓存：预览不做往回 seek，给 2 MiB 足够 seek 时的短暂回读。 */
    private const val DEMUX_MAX_BACK_BYTES = "2MiB"

    /**
     * 预读**时间跨度**（秒）。
     *
     * 【为什么必须给预览单独压小】主内核的共享 init 给 cache-secs=60，
     * 预览实例继承它：为了取 0 秒那一帧，mpv 把 4K 片源从 6072 顺读到
     * 35 657 656 字节（35.6 MB ≈ 60 秒媒体时间）——
     * 这段流量和主播放抢同一条宽带，正是拖动时最卡的那一下。
     */
    private const val CACHE_SECS = "5"

    /**
     * 设完这一套选项。
     *
     * @param extra 取证用的追加选项（`name=value`）。放在默认项**之后**生效，
     *        用来在一次装机里跑多组 A/B（见 `PlayerProbeHooks.probeThumbOptions`）。
     *        生产路径传空列表。
     * @return 失败的选项描述（"name=value"）；空列表 = 全部生效。
     */
    fun apply(
        native: MpvSession,
        extra: List<Pair<String, String>> = emptyList(),
    ): List<String> {
        val failures = ArrayList<String>()
        // 预览只要关键帧。native seek(exact=false) 发的是 `absolute`，而 mpv
        // hr-seek 默认=absolute ⇒ 仍精确落位、从关键帧前向解码到目标（真机日志
        // "hr-seek, skipping to 90" → 单帧 4.6s 主因）。no 关掉前向解码。
        set(native, "hr-seek", "no", failures)
        // 预览不出声、不要字幕。真机日志 audio/1 堆到 8MiB 上限触发
        // "Too many packets" 阻塞视频出帧；aid/sid=no 把 demux 队列全留给视频。
        set(native, "aid", "no", failures)
        set(native, "sid", "no", failures)
        set(native, "demuxer-max-bytes", DEMUX_MAX_BYTES, failures)
        set(native, "demuxer-max-back-bytes", DEMUX_MAX_BACK_BYTES, failures)
        set(native, "cache-secs", CACHE_SECS, failures)
        // pause⇒still 走 cache+blit 分支，该面 blit 不落（真机全透明）；dumb 模式直绘。
        set(native, "gpu-dumb-mode", "yes", failures)
        extra.forEach { (name, value) -> set(native, name, value, failures) }
        return failures
    }

    private fun set(native: MpvSession, name: String, value: String, failures: MutableList<String>) {
        val ok = runCatching { native.setProperty(name, value) }.getOrDefault(false)
        if (!ok) failures.add("$name=$value")
    }
}
