package com.kivo.cinema.ui.compose.player

/**
 * 浮层面板的**锚点**（B4-T1）：面板出现在**触发它的那个控件**旁边，
 * 而不是一律贴右上角。
 *
 * 【为什么用类型而不是一个 offset】三种锚点不在同一套约束里：
 * · 顶栏图标给的是「水平中心 + 顶栏下沿」，x 要居中后再 clamp；
 * · 底栏按钮给的是「右缘 + 按钮行上沿」，只约束右/下两条边；
 * · 居中态两者都不约束。
 * 把语义留在类型里，几何计算才能各自 clamp，而不是在外面堆 if-else。
 */
sealed interface PlayerPanelAnchor {
    /** 顶栏第 [index] 个图标（`TOP_ACTIONS` 的**下标**，从最左的投屏数起）。 */
    data class TopIcon(val index: Int) : PlayerPanelAnchor

    /** 底栏文字钮，[id] 与 [PlayerBottomBar] 的 `onTextAction` id 一致（resource / episodes / speed）。 */
    data class BottomAction(val id: String) : PlayerPanelAnchor

    /** 屏幕居中（没有触发控件，例如由内核状态自己弹出来的面板）。 */
    data object Center : PlayerPanelAnchor
}

/**
 * 锚点 → 具体坐标。纯函数，便于在无设备时复算。
 *
 * 【顶栏 x 的推导（◐ 推断，未真机复算）】
 * 图标行是 `fillMaxWidth().playerSafeArea().padding(horizontal = iconRowPad)`，
 * 右侧 8 个图标靠 `weight(1f)` 标题压到最右，最后一枚命中盒右缘 =
 * `screenWidth − iconRowPad`：`playerSafeArea` **只应用 start/top，不应用 end**
 * （`PlayerWindow.kt:82-99`），所以右侧不内缩。中心 = 右缘 − actionHitBox/2。
 * 第 k 枚（从左数）= 最右中心 − actionPitch·(count−1−k)。
 * 参考的观测值（844pt 上最右图标中心 769pt）与这条算式的差值落在 1pt 内。
 *
 * 【听视频态】PiP 图标被隐掉（`PlayerTopBar.kt:114`），行内其后的图标整体左移一格
 * ⇒ 用 [visibleSlot] 把它换成「可见序」再乘 pitch。
 */
internal object PlayerPanelGeometry {

    /** 面板与屏幕左右边缘的最小距离。 */
    const val edgeGap = 12f

    /** 面板与触发控件的间隙（顶栏下沿 / 按钮行上沿之外）。 */
    const val anchorGap = 6f

    /** 顶栏图标面板的顶边：顶栏总高 + 间隙（= 80pt，紧贴图标行下沿）。 */
    fun topIconTop(): Float = PlayerTokens.topBarHeight + anchorGap

    /** 底栏按钮面板的底边距屏底：按钮行高 + 下留白 + 间隙（= 63pt，面板底边贴按钮行上方）。 */
    fun bottomActionBottom(): Float = PlayerTokens.bottomPad + PlayerTokens.buttonRowHeight + anchorGap

    /** 第 [index] 个（`TOP_ACTIONS` 下标）图标在**当前可见行**里的位置。 */
    fun visibleSlot(index: Int, audioOnly: Boolean): Int {
        if (!audioOnly) return index
        val pipIndex = TOP_ACTIONS.indexOfFirst { it.action == "pip" }
        return if (pipIndex >= 0 && index > pipIndex) index - 1 else index
    }

    /** 行内可见图标数（听视频态少一枚 PiP）。 */
    fun visibleCount(audioOnly: Boolean): Int =
        TOP_ACTIONS.size - if (audioOnly) 1 else 0

    /** 顶栏第 [index] 个图标的水平中心（dp）。 */
    fun topIconCenterX(screenWidth: Float, index: Int, audioOnly: Boolean): Float {
        val lastCenter = screenWidth - PlayerTokens.iconRowPad - PlayerTokens.actionHitBox / 2f
        val slot = visibleSlot(index, audioOnly)
        return lastCenter - PlayerTokens.actionPitch * (visibleCount(audioOnly) - 1 - slot)
    }

    /**
     * 顶栏图标面板的左边缘：图标中心 − 面板宽/2，再夹到
     * `[edgeGap, screenWidth − panelWidth − edgeGap]`。
     * 最右的「听视频」与最左的「投屏」都会撞到这条 clamp，所以两侧都要夹。
     */
    fun topIconLeft(screenWidth: Float, panelWidth: Float, index: Int, audioOnly: Boolean): Float {
        val left = topIconCenterX(screenWidth, index, audioOnly) - panelWidth / 2f
        val maxLeft = (screenWidth - panelWidth - edgeGap).coerceAtLeast(edgeGap)
        return left.coerceIn(edgeGap, maxLeft)
    }

    /**
     * 由 kind 反查触发控件（供 [PlayerPanelHost.anchorFor] 使用）。
     * 找不到对应图标时回落 [PlayerPanelAnchor.Center] —— 不猜。
     */
    fun topIconOf(kind: String): PlayerPanelAnchor {
        val index = TOP_ACTIONS.indexOfFirst { it.action == kind }
        return if (index >= 0) PlayerPanelAnchor.TopIcon(index) else PlayerPanelAnchor.Center
    }
}