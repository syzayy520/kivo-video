package com.kivo.cinema.ui.compose.player

/**
 * **没有内核能力的那些形态**（v3 §1.4 形态 6/7/8/12 的缺能力部分）。
 *
 * 【为什么这些行必须存在】验收判据是"形态缺项=0"：参考有的面板我们都要有。
 * 但铁律同时要求"不许静默降级" —— 没有后端的能力不能画成能点的样子。
 * 两条合起来的唯一解：**形态照画，行置灰，副标题写清原因**。
 * 用户看到的是"这个功能在，但还没接上"，而不是"点了没反应"或"根本没有"。
 *
 * P10：**行首装饰图标全部去掉**（§5：无图标行文字从 20 开始），
 * 字幕面板的「操作」组头前移到「搜索字幕」行（组头跟着组的第一行走，不吊在第三行上）。
 *
 * 这里只产出数据，不碰内核、不碰 UI。
 */
internal object PlayerPanelForms {

    private const val ACTION_SECTION = "操作"

    /**
     * 字幕面板的"操作"组＋搜索/导入（形态 1，iPhone 现屏行序）。
     * **行序与既有 tag 不变**；「粘贴字幕链接」(`sub-import-url`) 紧跟「导入字幕」。
     */
    fun subtitleActions(): List<PlayerOptionItem> = listOf(
        disabled("搜索字幕", "搜索服务未接入", section = ACTION_SECTION),
        PlayerOptionItem(title = "导入字幕", tag = "sub-import"),
        PlayerOptionItem(title = "粘贴字幕链接", tag = "sub-import-url"),
        PlayerOptionItem(
            title = "显示设置",
            section = ACTION_SECTION,
            tag = "subtitleDisplay",
            control = PlayerRowControl.CHEVRON,
        ),
        // AI 中文字幕（v1 静态行：状态投影在点击反馈，UX v3 §3 的行内实时
        // 状态列 T-C 后续——需要 Controller 状态流接 Compose，评审已登记偏差）。
        PlayerOptionItem(title = "AI 中文字幕", tag = "sub-ai"),
        PlayerOptionItem(title = "关闭字幕", tag = "sub-off"),
    )

    /** 音轨面板的附加行（形态 4：当前秒数副题 → 钻入延迟卡）。 */
    fun audioExtras(delaySec: Double): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(
            title = "音频延迟",
            subtitle = String.format("%.1f s", delaySec),
            tag = "audioDelay",
            control = PlayerRowControl.CHEVRON,
        ),
    )

    /** 跳过片段卡（形态 6）。片头/片尾标记没有后端 ⇒ 置灰带原因。 */
    fun skip(): List<PlayerOptionItem> = listOf(
        disabled("跳过片头", "片头标记未接入"),
        disabled("跳过片尾", "片尾标记未接入"),
        PlayerOptionItem(
            title = "跳过片段方式",
            subtitle = "跟随默认",
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
        disabled("片头", "片头标记未接入", subtitle = "自动", chevron = true),
        disabled("片尾", "片尾标记未接入", subtitle = "自动", chevron = true),
        disabled("中间片段", "片段标记未接入", subtitle = "自动", chevron = true),
    )

    /** 弹幕操作列表（形态 8）。开关是真的，过滤词没有后端。 */
    fun danmakuOps(on: Boolean, hasItems: Boolean): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(
            title = "显示设置",
            tag = "danmakuSettings",
            control = PlayerRowControl.CHEVRON,
        ),
        disabled("过滤关键词", "过滤词表未接入"),
        if (hasItems) {
            PlayerOptionItem(
                title = "开启弹幕",
                tag = "danmaku",
                control = PlayerRowControl.TOGGLE,
                on = on,
            )
        } else {
            // 铁律：没有弹幕文件时开关不能画成能点的样子。
            disabled("开启弹幕", "未找到弹幕文件")
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

    /** 外挂字幕钻入页为空：指一条真能走的路（导入），不静默。
     *  AI 行**空态常驻**（UX v3 §3 入口常驻，评审 F2/G5：入口行不消失）——
     *  保持可点：点击走 handleAiSubtitleRow → 检测无英文 SRT → toast 给原因。 */
    fun noExternalSubtitles(): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(title = "无外挂字幕", subtitle = "用「导入字幕」加载", enabled = false),
        PlayerOptionItem(title = "AI 中文字幕", subtitle = "需先添加英文 SRT 字幕", tag = "sub-ai"),
    )

    /** 内核未就绪：**非空 reason 行**，不是空列表（空列表会被读成"没有音轨"）。 */
    fun notReady(): List<PlayerOptionItem> = listOf(
        PlayerOptionItem(title = "内核尚未就绪", subtitle = "起播后即可切换", enabled = false),
    )

    private fun disabled(
        title: String,
        reason: String,
        subtitle: String = "",
        section: String = "",
        chevron: Boolean = false,
    ): PlayerOptionItem {
        // 副标题优先放用户给的说明（如"自动"），原因追加在后 —— 两者都要可见。
        val sub = if (subtitle.isBlank()) reason else "$subtitle · $reason"
        return PlayerOptionItem(
            title = title,
            subtitle = sub,
            section = section,
            enabled = false,
            control = if (chevron) PlayerRowControl.CHEVRON else PlayerRowControl.NONE,
        )
    }
}