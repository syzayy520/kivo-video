package com.kivo.cinema.hls

/**
 * **选哪一档**（纯决策，无 IO、无 Android）。
 *
 * ## 为什么需要它
 * 310 实测：mpv 在真实 HLS master 上只能播**第一个 program**（Apple 流 = 960x540）。
 * 也就是说"选档"这件事压根没人做 —— 内核按 master 的顺序随手拿了一个。
 *
 * 本类把"选档"变成一条**可复算的规则**：
 * 设备解得了 + 分辨率不超屏 + 带宽装得下 ⇒ 在这个集合里取**画质最高的那一档**。
 *
 * ## 为什么不是"永远选最高"
 * 274 已经有教训：码率超过链路能力时，起播就卡（4K 54 Mbps 实测 1.53 fps）。
 * 所以带宽预算是一道硬闸门 —— 它不预测带宽，只保证"不选一个明显装不下的档"。
 *
 * ## 为什么不是"自适应 ABR"
 * 这是**一次性决策**（起播时选一档，之后固定），与 SenPlayer 的做法一致
 * （306/311 逆向结论：它没有运行时 ABR，请求里写死 `VideoBitrate`）。
 * 真正的 ABR 需要"重开流"，代价是可见中断，不在本卡范围。
 */
object HlsVariantPolicy {

    /**
     * 选择结果。
     *
     * @param index 选中的 variant 下标（交给 [HlsMaster.keepOnly]）；
     *              **-1 表示"不该裁"**（没有 variant、或只有一个）
     * @param reason 为什么选它 —— 必须能写进日志（铁律：不许静默决策）
     */
    data class Choice(val index: Int, val reason: String)

    /**
     * @param variants master 里解析出的全部档位（顺序无关）
     * @param maxHeight 分辨率上限（设备可显示/可解的高）；<=0 表示不限制
     * @param maxBps 带宽预算（bps）；<=0 表示不限制
     * @param canDecode 设备能否解这一档（codec × 分辨率闸门）；默认全放行
     */
    fun choose(
        variants: List<HlsVariant>,
        maxHeight: Int,
        maxBps: Long,
        canDecode: (HlsVariant) -> Boolean = { true },
    ): Choice {
        if (variants.isEmpty()) return Choice(-1, "master 里没有 variant（保持原样）")
        if (variants.size == 1) {
            val only = variants.first()
            return Choice(-1, "只有一档（${describe(only)}），无需裁剪")
        }

        // 闸门一：设备解得了 + 不超屏
        val eligible = variants.filter { v ->
            val hOk = maxHeight <= 0 || v.height <= 0 || v.height <= maxHeight
            hOk && canDecode(v)
        }
        if (eligible.isEmpty()) {
            return Choice(-1, "没有任何一档能解（${variants.size} 档全部被闸门拒绝）⇒ 保持原样交给内核")
        }

        // 闸门二：带宽装得下；在里面取画质最高的
        val affordable = if (maxBps <= 0) eligible else eligible.filter { it.bandwidth <= 0 || it.bandwidth <= maxBps }
        if (affordable.isNotEmpty()) {
            val best = affordable.maxByOrNull { it.bandwidth }!!
            val skipped = variants.size - affordable.size
            return Choice(
                best.index,
                "带宽内最高档：${describe(best)}" +
                    if (skipped > 0) "（已排除 $skipped 档：超预算或设备不支持）" else "",
            )
        }

        // 全都超预算 ⇒ 退最小档（仍是最可能播得动的），但要说清楚
        val smallest = eligible.minByOrNull { it.bandwidth }!!
        return Choice(
            smallest.index,
            "全部超预算（$maxBps bps）⇒ 退最小档：${describe(smallest)}",
        )
    }

    private fun describe(v: HlsVariant): String {
        val res = if (v.width > 0 && v.height > 0) "${v.width}x${v.height}" else "分辨率未知"
        val mbps = if (v.bandwidth > 0) "%.1f Mbps".format(java.util.Locale.US, v.bandwidth / 1_000_000.0) else "带宽未知"
        val codec = v.videoCodec.ifEmpty { "codec未知" }
        return "$res $codec $mbps"
    }
}
