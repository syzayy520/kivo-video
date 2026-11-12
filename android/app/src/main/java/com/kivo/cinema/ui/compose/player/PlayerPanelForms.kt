package com.kivo.cinema.ui.compose.player

/**
 * **没有内核能力的那些形态**（v3 §1.4 形态 6/7/8/12 的缺能力部分）。
 *
 * 【为什么这些行必须存在】验收判据是"形态缺项=0"：参考有的面板我们都要有。
 * 但铁律同时要求"不许静默降级" —— 没有后端的能力不能画成能点的样子。
 * 两条合起来的唯一解：**形态照画，行置灰，副标题写清原因**。
 * 用户看到的是"这个功能在，但还没接上"，而不是"点了没反应"或"根本没有"。
 *
 * 这里只产出数据，不碰内核、不碰 UI。
 */
internal object PlayerPanelForms {

    /** 字幕面板的"操作"组＋搜索/导入（形态 1，iPhone 现屏行序）。 */
    fun subtitleActions(): List<PlayerOptionItem> = listOf(
        disabled("搜索字幕", "字幕搜索服务尚未接入", icon = "search"),
        PlayerOptionItem(title = "导入字幕", icon = "import", tag = "sub-import"),
        PlayerOptionItem(
            title = "显示设置",
            icon = "gear",
            section = "操作",
            tag = "subtitleDisplay",
            control = PlayerRowControl.CHEVRON,
        ),
        PlayerOptionItem(title = "关闭字幕", icon = "xcircle", tag = "sub-off"),
    )

    /** 音轨面板的附加行（形态 4：sliders icon + 当前秒数副题 → 钻入延迟卡）。 */
    fun audioExtras(delaySec: Double): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(
            title = "音频延迟",
            subtitle = String.format("%.1f s", delaySec),
            icon = "sliders",
            tag = "audioDelay",
            control = PlayerRowControl.CHEVRON,
        ),
    )

    /** 跳过片段卡（形态 6）。片头/片尾标记没有后端 ⇒ 置灰带原因。 */
    fun skip(): List<PlayerOptionItem> = listOf(
        disabled("跳过片头", "片头标记尚未接入", icon = "backward"),
        disabled("跳过片尾", "片尾标记尚未接入", icon = "forward"),
        PlayerOptionItem(
            title = "跳过片段方式",
            subtitle = "跟随默认",
            icon = "clapper",
            tag = "skipMode",
            control = PlayerRowControl.CHEVRON,
        ),
    )

    /** 跳过方式列表（形态 7）。整组无后端 ⇒ 全部置灰，但形态与选中态照画。 */
    fun skipMode(): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(
            title = "跟随默认",
            subtitle = "自动",
            selected = true,
            enabled = false,
            tag = "default",
            control = PlayerRowControl.CHEVRON,
        ),
        disabled("片头", "片头标记尚未接入", subtitle = "自动", chevron = true),
        disabled("片尾", "片尾标记尚未接入", subtitle = "自动", chevron = true),
        disabled("中间片段", "片段标记尚未接入", subtitle = "自动", chevron = true),
    )

    /** 弹幕操作列表（形态 8）。开关是真的，过滤词没有后端。 */
    fun danmakuOps(on: Boolean, hasItems: Boolean): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(
            title = "显示设置",
            icon = "gear",
            tag = "danmakuSettings",
            control = PlayerRowControl.CHEVRON,
        ),
        disabled("过滤关键词", "过滤词表尚未接入", icon = "captions"),
        if (hasItems) {
            PlayerOptionItem(
                title = "开启弹幕",
                icon = "danmaku",
                tag = "danmaku",
                control = PlayerRowControl.TOGGLE,
                on = on,
            )
        } else {
            // 铁律：没有弹幕文件时开关不能画成能点的样子。
            disabled(
                "开启弹幕",
                "未找到弹幕文件（danmaku/<片名>.xml）",
                icon = "danmaku",
            )
        },
    )

    /** 选集带（形态 12）的内容：资源库还没有剧集列表。 */
    fun episodes(): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(
            title = "剧集列表尚未接入资源库",
            subtitle = "当前片源只有这一部影片",
            enabled = false,
        ),
    )

    /** 内嵌字幕钻入页为空：片源没有内封字幕轨（不是"未就绪"，两者原因不同）。 */
    fun noEmbeddedSubtitles(): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(title = "无内嵌字幕", subtitle = "当前片源没有内封字幕轨", enabled = false),
    )

    /** 外挂字幕钻入页为空：指一条真能走的路（导入），不静默。 */
    fun noExternalSubtitles(): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(title = "无外挂字幕", subtitle = "可在字幕面板用「导入字幕」加载", enabled = false),
    )

    /** 内核未就绪：**非空 reason 行**，不是空列表（空列表会被读成"没有音轨"）。 */
    fun notReady(): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(title = "内核尚未就绪", subtitle = "起播后即可切换", enabled = false),
    )

    private fun disabled(
        title: String,
        reason: String,
        subtitle: String = "",
        icon: String? = null,
        chevron: Boolean = false,
    ): PlayerOptionItem {
        // 副标题优先放用户给的说明（如"自动"），原因追加在后 —— 两者都要可见。
        val sub = if (subtitle.isBlank()) reason else "$subtitle · $reason"
        return PlayerOptionItem(
            title = title,
            subtitle = sub,
            icon = icon,
            enabled = false,
            control = if (chevron) PlayerRowControl.CHEVRON else PlayerRowControl.NONE,
        )
    }
}