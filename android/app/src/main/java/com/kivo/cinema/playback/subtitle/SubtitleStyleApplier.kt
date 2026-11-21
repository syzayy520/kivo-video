package com.kivo.cinema.playback.subtitle

import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.NetDiag

/**
 * 把 [SubtitleStyle] 翻成 mpv 属性并下发（P9 · C）。
 *
 * 【为什么在 loadFile 之前】`sub-ass-override` 是 UPDATE_SUB_HARD ——
 * 它决定"用不用源样式"，必须在字幕轨建立前就位；颜色/字号虽然是热属性，
 * 但一起在起播前下发能保证**第一条字幕出现时**就是用户选的样式，
 * 不会先闪一帧默认值。
 *
 * 【失败不许静默】每个属性写入都有回执（`setProperty` 的 bool），
 * 失败项汇总进日志（`NetDiag`）。mpv 不会主动打印被拒的属性。
 */
internal object SubtitleStyleApplier {

    /**
     * 起播前：样式 + 本片源记住的主/次延迟。返回失败项（name=value）。
     *
     * [secondaryDelaySeconds] 来自同一片源键的 `s:` 记录（SUB-M5）；两个延迟各自独立，
     * 非 0 才下发 —— 不发 0 是为了让内核保留它的默认，避免"我们没设却写了一版"。
     */
    fun applyAtStart(
        native: MpvSession,
        style: SubtitleStyle,
        delaySeconds: Double,
        secondaryDelaySeconds: Double = 0.0,
    ): List<String> {
        val failures = applyProperties(native, style.properties())
        if (delaySeconds != 0.0) {
            val value = String.format(java.util.Locale.US, "%.2f", SubtitleStyle.clampDelay(delaySeconds))
            if (!native.setProperty("sub-delay", value)) failures += "sub-delay=$value"
        }
        if (secondaryDelaySeconds != 0.0) {
            val value = String.format(java.util.Locale.US, "%.2f", SubtitleStyle.clampDelay(secondaryDelaySeconds))
            if (!native.setProperty("secondary-sub-delay", value)) failures += "secondary-sub-delay=$value"
        }
        NetDiag.log(
            "[substyle] 起播应用 font=${style.fontSize} pos=${style.pos} marginY=${style.marginY} " +
                "ass=${style.assMode.mpvValue} delay=${"%.2f".format(java.util.Locale.US, delaySeconds)} " +
                "secDelay=${"%.2f".format(java.util.Locale.US, secondaryDelaySeconds)} " +
                "fail=${failures.size}" + if (failures.isEmpty()) "" else " ${failures.joinToString(",")}",
        )
        return failures
    }

    /** 运行期单项写（面板选项循环用）。 */
    fun applyProperty(native: MpvSession, name: String, value: String): Boolean =
        native.setProperty(name, value)

    private fun applyProperties(
        native: MpvSession,
        properties: List<Pair<String, String>>,
    ): MutableList<String> {
        val failures = ArrayList<String>(0)
        properties.forEach { (name, value) ->
            if (!native.setProperty(name, value)) failures += "$name=$value"
        }
        return failures
    }
}
