package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 播放页顶栏（v3 §1.1）：**自绘状态行 + 图标行**，无遮罩渐变。
 *
 * 实测几何（844×390pt 基准）：
 * · 状态行 时钟 x≈63 / y10–23，右端网速 + 电池；
 * · 图标行中心 y≈55；✕ 心 74.3（glyph 17）、主页心 121.8（glyph 20）；
 * · 标题 x148 起、16pt；右 8 图标中心 419.7→769.0，**pitch 50pt**；
 * · 左右 ink 边距 ≈66pt。
 *
 * 【v2 的死按钮是怎么修的】v2 每个图标是 `size(21.5).clickable{}` ——
 * 命中区只有 21.5pt，手指按偏一点就没反应（用户实测"按钮点不动"）。
 * v3 命中盒一律 **44pt**，pitch 靠 `spacedBy(pitch − 44)` 补回来：
 * 中心距仍是实测的 50pt，命中区却大了一倍。
 * v2 注释里"44pt 盒会把间距撑到 94pt"的说法是错的 —— 错的是它没减间距。
 *
 * 【为什么没有渐变】实测 REX 顶栏白图标直压画面。v2 加的黑 28% 渐变是自创，
 * 会让画面顶部出现一条黑带。这里按参考去掉（[PlayerTokens.topScrimTop] 全透明）。
 */
@Composable
internal fun PlayerTopBar(
    title: String,
    speedText: String,
    activeAction: String,
    danmakuOn: Boolean,
    audioOnly: Boolean,
    onClose: () -> Unit,
    onHome: () -> Unit,
    onAction: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    Column(modifier = modifier.fillMaxWidth()) {
        PlayerStatusRow(speedText = speedText)
        Spacer(modifier = Modifier.height(PlayerTokens.statusIconGap.dp))

        Row(
            modifier = Modifier
                .fillMaxWidth()
                .height(PlayerTokens.iconRowHeight.dp)
                .playerSafeArea()
                .padding(horizontal = PlayerTokens.iconRowPad.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Row(
                horizontalArrangement = Arrangement.spacedBy(PlayerTokens.leftIconGap.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                TopIconButton(
                    kind = "close",
                    glyphSize = PlayerTokens.closeIconSize,
                    label = "关闭",
                    active = false,
                    onClick = onClose,
                )
                TopIconButton(
                    kind = "home",
                    glyphSize = PlayerTokens.homeIconSize,
                    label = "主页",
                    active = false,
                    onClick = onHome,
                )
            }

            Text(
                text = title,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.titleSize.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier
                    .padding(start = 4.dp, end = 8.dp)
                    .weight(1f),
            )

            Row(
                horizontalArrangement = Arrangement.spacedBy(
                    (PlayerTokens.actionPitch - PlayerTokens.actionHitBox).dp,
                ),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                TOP_ACTIONS.forEach { (kind, label) ->
                    // 听视频态没有画面 ⇒ PiP 无意义，REX 0172 也隐掉它。
                    if (audioOnly && kind == "pip") return@forEach
                    TopIconButton(
                        kind = kind,
                        glyphSize = PlayerTokens.actionIconSize,
                        label = label,
                        active = activeAction == kind || (kind == "audioMode" && audioOnly),
                        // 弹幕关掉时图标变暗：这是**真状态**，不是装饰。
                        dimmed = kind == "danmaku" && !danmakuOn,
                        onClick = { onAction(kind) },
                        // 圈弹长按 = 弹幕操作面板（形态8，◐ 待 iPhone 补证）。
                        onLongClick = if (kind == "danmaku") {
                            { onAction("danmaku-panel") }
                        } else {
                            null
                        },
                    )
                }
            }
        }
    }
}

/** 语义序与实测一致（投屏·弹幕·PiP·场记板·比例·**音轨**·字幕·**听视频**）。
 *  v3.1：首枚由 AirPlay 换成通用投屏符（DLNA）——Apple 专有协议在 Android 做不了，
 *  图标/面板行/文案全链路移除；槽位不动，其余图标锚点不变。 */
private val TOP_ACTIONS = listOf(
    "cast" to "投屏",
    "danmaku" to "弹幕",
    "pip" to "画中画",
    "clapper" to "跳过片段",
    "expand" to "画面比例",
    "audio" to "音轨",
    "captions" to "字幕",
    "audioMode" to "听视频",
)

/**
 * 一个顶栏按钮：44pt 命中盒 + 居中 glyph。
 *
 * 用 `pointerInput` 而不是 `clickable`：后者会在 44pt 盒上画一圈涟漪，
 * 一排八个图标同时起涟漪很脏；参考的反馈是**图标本身变色**（[active]）。
 */
@Composable
private fun TopIconButton(
    kind: String,
    glyphSize: Float,
    label: String,
    active: Boolean,
    onClick: () -> Unit,
    dimmed: Boolean = false,
    onLongClick: (() -> Unit)? = null,
) {
    Box(
        modifier = Modifier
            .size(PlayerTokens.actionHitBox.dp)
            .alpha(if (dimmed) 0.45f else 1f)
            .semantics { contentDescription = label }
            .pointerInput(Unit) {
                detectTapGestures(onTap = { onClick() }, onLongPress = { onLongClick?.invoke() })
            },
        contentAlignment = Alignment.Center,
    ) {
        // 听视频激活＝白圆底黑耳机（REX 0172 的反白圆），不是通用绿色高亮。
        val inverted = kind == "audioMode" && active
        Box(
            modifier = Modifier
                .size((if (inverted) glyphSize + 6f else glyphSize).dp)
                .then(
                    if (inverted) Modifier.background(PlayerTokens.textPrimary, CircleShape)
                    else Modifier,
                ),
            contentAlignment = Alignment.Center,
        ) {
            PlayerGlyph(
                kind = kind,
                color = when {
                    inverted -> Color.Black
                    active -> PlayerTokens.activeAccent
                    else -> PlayerTokens.textPrimary
                },
                modifier = Modifier.size(glyphSize.dp),
            )
        }
    }
}