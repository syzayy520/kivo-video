package com.kivo.cinema.ui.compose.player

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/**
 * 面板的**唯一决策点**（v3 §1.4 / B4-T1）：kind → 壳 / 宽度 / 锚点 / 过渡。
 *
 * 【为什么要有这一个文件】13 种形态若各自在 Overlay 里写 if-else，
 * 锚点和壳的选择就会散在四处，改一个面板要翻三个文件。
 * 这里把"哪个 kind 用哪个壳、贴在哪"收成一张表，Overlay 只负责挂。
 *
 * 【锚点（B4-T1 重写）】旧版按 kind 算 `trailingPad`/`topPad` —— 结果是**所有面板
 * 都从右上角冒出**（`speed` 也贴在屏幕顶部 43pt），与触发控件完全脱钩。
 * 现在改成「kind → 触发控件」反查，真实坐标由 [PlayerPanelGeometry] 按触发控件的
 * 几何算：顶栏图标 → 图标下方、水平居中于图标；底栏文字钮 → 按钮行正上方、右缘对齐。
 * 钻入子面板（如 audio → audioDelay）自动跟随父面板的图标，锚点不会跳走。
 *
 * 【为什么按 kind 反查而不是记住"最后一次点的是谁"】面板 kind 会因钻入而变
 * （`audio` → `audioDelay`），若记住点击那一刻的控件，钻入后锚点就丢了；
 * kind 本身已经携带"它属于哪个触发器"的信息，反查是幂等的。
 */
internal object PlayerPanelHost {

    /** 底栏内联的两种形态：不浮在画面上，由 [PlayerBottomBar] 撑开/整替。 */
    fun isInline(kind: String): Boolean = kind == "resource" || kind == "episodes"

    fun isDetail(kind: String): Boolean = kind in DETAIL_KINDS

    /**
     * 壳宽（唯一来源，[PlayerPanelShell] 与锚点 clamp 共用）。
     *
     * ◐ `skip` 的 300dp 与 detail 卡的 357dp 都**没有**截图证据（PNL-02 待改，
     * 不在 B4-T1 范围）；锚点计算必须用同一宽度，否则面板会贴边贴错。
     */
    fun panelWidth(kind: String): Dp = when {
        !isDetail(kind) -> PlayerTokens.panelWidth.dp
        kind == "skip" -> SKIP_CARD_WIDTH.dp
        else -> PlayerTokens.detailCardWidth.dp
    }

    /**
     * kind → 触发控件。
     *
     * 顶栏一侧按 `TOP_ACTIONS` 的 **action** 反查下标（顺序真相仍在
     * `PlayerTopBar.kt`）；底栏一侧用 `onTextAction` 的 id。
     * 找不到就 [PlayerPanelAnchor.Center] —— 不猜位置。
     */
    fun anchorFor(kind: String): PlayerPanelAnchor = when (kind) {
        "speed" -> PlayerPanelAnchor.BottomAction("speed")
        "resource" -> PlayerPanelAnchor.BottomAction("resource")
        "episodes" -> PlayerPanelAnchor.BottomAction("episodes")
        // 弹幕操作面板（长按圈弹）也属于弹幕图标。
        "danmaku", "danmaku-panel", "danmakuOps", "danmakuSettings" -> topIcon("danmaku")
        "pip" -> topIcon("pip")
        // 第 4 格（场记板图形）的行为是 `expand → quality`（`PlayerActionRouter.kt:237`）。
        "expand", "quality", "fill" -> topIcon("expand")
        // 第 5 格（圈勾图形）的行为是 clapper；跳过相关微调卡都挂在它下面。
        "clapper", "skip", "skipMode" -> topIcon("clapper")
        "audio", "audioDelay" -> topIcon("audio")
        "captions", "subtitle", "subtitleTracks", "subtitleExternal", "subtitleDisplay" ->
            topIcon("captions")
        "audioMode" -> topIcon("audioMode")
        "cast" -> topIcon("cast")
        else -> PlayerPanelAnchor.Center
    }

    private fun topIcon(action: String): PlayerPanelAnchor = PlayerPanelGeometry.topIconOf(action)

    private const val SKIP_CARD_WIDTH = 300f
    private val DETAIL_KINDS = setOf("audioDelay", "subtitleDisplay", "skip", "danmakuSettings")
}

/**
 * 按 kind 选壳并渲染。锚点与过渡由调用方负责（[FloatingPanel]）。
 */
@Composable
internal fun PlayerPanelShell(
    kind: String,
    title: String,
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    onStep: (PlayerOptionItem, Int) -> Unit,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
) {
    if (PlayerPanelHost.isDetail(kind)) {
        PlayerDetailCard(
            title = title,
            items = items,
            onSelect = onSelect,
            onStep = onStep,
            onDismiss = onDismiss,
            width = PlayerPanelHost.panelWidth(kind),
            showReset = kind != "skip",
            modifier = modifier,
        )
    } else {
        PlayerGlassMenu(
            title = title,
            items = items,
            onSelect = onSelect,
            width = PlayerPanelHost.panelWidth(kind),
            modifier = modifier,
        )
    }
}

// 审查 P1-4：原先这里有一套只查 ANIMATOR_DURATION_SCALE 的 rememberReduceMotion，
// 与 MotionGate 重复且漏掉另两个 scale + App 开关 ⇒ 已删除，调用方统一走 MotionGate。