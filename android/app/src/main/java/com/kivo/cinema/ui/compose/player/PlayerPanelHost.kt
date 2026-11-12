package com.kivo.cinema.ui.compose.player

import android.provider.Settings
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

/**
 * 面板的**唯一决策点**（v3 §1.4）：kind → 壳 / 宽度 / 锚点 / 过渡。
 *
 * 【为什么要有这一个文件】13 种形态若各自在 Overlay 里写 if-else，
 * 锚点和壳的选择就会散在四处，改一个面板要翻三个文件。
 * 这里把"哪个 kind 用哪个壳、贴在哪"收成一张表，Overlay 只负责挂。
 *
 * 【锚点怎么算（K40 实测校准，2026-11-12）】规则＝面板右缘≈触发 icon 右缘+12pt。
 * 顶栏右排 icon 中心距右安全区 = iconRowPad(8) + 22 + 50·idx（idx 从右数），
 * 面板右缘 = 中心 + 22(半盒) + 12 ⇒ trailingPad = 50·idx − 4，**与设备安全区无关**。
 * idx：captions=1→46、audio=2→96、expand=3→146、danmaku=6→296。
 * （v3.1：投屏改直达 DLNA sheet，不再有 cast 面板；首枚图标换 cast 但槽位不变。）
 * 听视频态隐 PiP ⇒ 它左边的 danmaku 右移 50，trailing 减 50。
 * 底栏文字钮（倍速）右缘＝内容右缘 ⇒ trailing 16（实测面板右缘≈倍速右缘）。
 * 微调卡统一 trailing 16 / top 16（§1.4）。
 */
internal object PlayerPanelHost {

    /** 底栏内联的两种形态：不浮在画面上，由 [PlayerBottomBar] 撑开/整替。 */
    fun isInline(kind: String): Boolean = kind == "resource" || kind == "episodes"

    fun isDetail(kind: String): Boolean = kind in DETAIL_KINDS

    fun trailingPad(kind: String, audioOnly: Boolean = false): Float = when (kind) {
        // 字幕钻入页（内嵌/外挂）沿用字幕面板的锚点：原地换内容。
        "subtitle", "subtitleTracks", "subtitleExternal" -> 46f
        "audio" -> 96f
        "quality" -> 146f
        "speed" -> 16f
        "danmakuOps" -> if (audioOnly) 246f else 296f
        in DETAIL_KINDS -> 16f
        else -> 64f
    }

    fun topPad(kind: String): Float = if (isDetail(kind)) 16f else 43f

    private val DETAIL_KINDS = setOf("audioDelay", "subtitleDisplay", "skip", "danmakuSettings")
}

/**
 * 按 kind 选壳并渲染。调用方负责对齐与过渡（[PlayerOverlay]）。
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
            width = if (kind == "skip") 300.dp else PlayerTokens.detailCardWidth.dp,
            showReset = kind != "skip",
            modifier = modifier,
        )
    } else {
        PlayerGlassMenu(
            title = title,
            items = items,
            onSelect = onSelect,
            width = PlayerTokens.panelWidth.dp,
            modifier = modifier,
        )
    }
}

/**
 * G1 降级：系统"移除动画"（animator duration scale = 0）时，
 * 位移/缩放归零、只留 alpha 或瞬时 —— 触觉（G2）与视觉解耦，不受这里影响。
 */
@Composable
internal fun rememberReduceMotion(): Boolean {
    val context = LocalContext.current
    return Settings.Global.getFloat(
        context.contentResolver,
        Settings.Global.ANIMATOR_DURATION_SCALE,
        1f,
    ) == 0f
}