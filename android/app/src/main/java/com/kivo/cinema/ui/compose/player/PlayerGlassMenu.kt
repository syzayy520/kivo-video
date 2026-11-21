package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.onClick
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.motion.MotionGate

/**
 * **面板列表壳**（P10 §5/§7/§11）：字幕、倍速、画面比例、音轨、跳过方式、弹幕、投屏共用。
 *
 * 与 P10 之前相比只换了"皮"与三处交互，**结构与数据没动**：
 * · 材质换成 [playerPanelSurface]（冷调纵向渐变、仅顶边受光线、无内嵌卡）；
 * · 行高按 [PlayerPanelStyle.Density] 分档（Comfort 56/72，Compact 42）；
 * · 勾/chevron 从行首搬到**行尾**，行首不再放装饰图标；
 * · 滚动加了边缘渐隐 + 自绘指示条，内容空→有 120ms 淡入。
 *
 * 【必须保留的三条】① 宽度 248（REX 已验收几何）；② `rememberUpdatedState` 行槽修复
 * （面板切换瞬间 `items` 还是上一个面板的，旧闭包会把"导入字幕"派发成上个面板的条目，FAIL-3）；
 * ③ 遮罩点击消费（`pointerInput` 吃掉面板内 tap，否则"点面板＝关面板"）。
 *
 * 【为什么限高 + 滚动】面板浮在底栏之上，可用高度 ≈311dp；
 * 不限高 ⇒ 最后几行被屏幕切掉、永远点不到（v2 实机踩过）。
 */
@Composable
internal fun PlayerGlassMenu(
    title: String,
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    modifier: Modifier = Modifier,
    subtitle: String = "",
    width: Dp = PlayerTokens.panelWidth.dp,
    density: PlayerPanelStyle.Density = PlayerPanelStyle.Density.Comfort,
    /** P9 · H：长按行（外挂字幕删除入口）。默认空 = 行不支持长按语义的副作用。 */
    onLongSelect: (PlayerOptionItem) -> Unit = {},
    /** 抓帧模糊背景图；null（默认）时分文不画，视觉与不带背景图完全一致。 */
    backdrop: ImageBitmap? = null,
) {
    val scroll = rememberScrollState()
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    val contentAlpha = panelContentAlpha(items.isNotEmpty(), reduceMotion)
    val hasHeader = title.isNotBlank() && density != PlayerPanelStyle.Density.Compact
    Box(
        modifier = modifier
            .width(width)
            .heightIn(max = PlayerTokens.panelMaxHeight.dp)
            .playerPanelSurface(
                // 页眉带 = 顶部受光带：头部高度内白 2.5% 渐隐到 0。它只是"光"，
                // 不是一条随滚动要跟着走的分区线 —— 所以滚动后留着也不穿帮。
                // Compact 列表没有头部，画了就是无源之亮。
                headerWash = if (hasHeader) PlayerPanelStyle.headerWash else androidx.compose.ui.graphics.Color.Transparent,
                headerHeight = if (hasHeader) PlayerPanelStyle.headerHeight + PlayerPanelStyle.verticalPad(density) else 0.dp,
                backdrop = backdrop,
            )
            // 吃掉面板内的点击：手势层在下面兜底，不消费就会"点面板＝关面板"。
            .pointerInput(Unit) { detectTapGestures(onTap = {}) },
    ) {
        // 内容层（滚动列 + 滚动条）一起裁到卡圆角内：滚动条贴到卡顶/卡底时不能压出方角。
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(PlayerPanelStyle.radius)),
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    // 渐隐必须挂在 verticalScroll 之前（外层），否则它跟着内容一起滚。
                    .panelVerticalFade(
                        top = scroll.canScrollBackward,
                        bottom = scroll.canScrollForward,
                    )
                    // fade 留在外层（DstOut 只削内容 alpha），裁剪在内层。
                    .clip(RoundedCornerShape(PlayerPanelStyle.radius))
                    .verticalScroll(scroll)
                    .padding(
                        start = PlayerPanelStyle.hPadStart,
                        end = 0.dp,
                        top = PlayerPanelStyle.verticalPad(density),
                        bottom = PlayerPanelStyle.scrollBottomPad(density),
                    )
                    .graphicsLayer { alpha = contentAlpha },
            ) {
                // Compact（倍速/画质比例）无头部：7 档 ×42 才装得进 311dp 可用高度（§5）。
                if (hasHeader) {
                    MenuHeader(title = title, subtitle = subtitle)
                }
                renderRows(
                    items = items,
                    density = density,
                    hasHeader = hasHeader,
                    onSelect = onSelect,
                    onLongSelect = onLongSelect,
                )
            }
            PanelScrollbar(state = scroll, modifier = Modifier.align(Alignment.CenterEnd))
        }
    }
}

/** 分组头 + 行。组间只画一条 1dp 白 6% 分隔线（Compact 无分隔线，只有留白）。 */
@Composable
private fun renderRows(
    items: List<PlayerOptionItem>,
    density: PlayerPanelStyle.Density,
    hasHeader: Boolean,
    onSelect: (PlayerOptionItem) -> Unit,
    onLongSelect: (PlayerOptionItem) -> Unit,
) {
    var lastSection: String? = null
    items.forEach { item ->
        val newGroup = item.section.isNotBlank() && item.section != lastSection
        if (newGroup) {
            if (lastSection != null || hasHeader) {
                Spacer(Modifier.height(PlayerPanelStyle.groupGap))
                if (PlayerPanelStyle.hasGroupDivider(density)) MenuSeparator()
            }
            Text(
                text = item.section,
                color = PlayerPanelStyle.groupHeadColor,
                fontSize = PlayerPanelStyle.groupHeadSize.sp,
                lineHeight = PlayerPanelStyle.groupHeadLineHeight.sp,
                letterSpacing = PlayerPanelStyle.groupHeadTracking.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
                modifier = Modifier.padding(
                    top = PlayerPanelStyle.groupHeadTop,
                    bottom = PlayerPanelStyle.groupHeadBottom,
                ),
            )
        }
        lastSection = item.section
        MenuRow(
            item = item,
            density = density,
            onSelect = { onSelect(item) },
            onLongPress = { onLongSelect(item) },
        )
    }
}

/** 一行：Comfort 单行 56 / 双行 72，Compact 42；勾与 chevron 一律在行尾。 */
@Composable
private fun MenuRow(
    item: PlayerOptionItem,
    density: PlayerPanelStyle.Density,
    onSelect: () -> Unit,
    onLongPress: () -> Unit,
) {
    // pointerInput 协程只在 key 变化时重启，会冻住首帧回调。面板切换时 `items`
    // 先短暂保留上一个面板的列表（LaunchedEffect 读内核是异步的），行槽若沿用
    // 旧闭包，"导入字幕"就会派发成上个面板的条目 —— 无 toast、无动作（FAIL-3）。
    val currentOnSelect by rememberUpdatedState(onSelect)
    val currentOnLongPress by rememberUpdatedState(onLongPress)
    val tall = item.subtitle.isNotBlank()
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(PlayerPanelStyle.rowHeight(density, tall))
            .alpha(if (item.enabled) 1f else 0.38f)
            .panelRowWash(selected = item.selected)
            .panelPressable(
                enabled = item.enabled,
                onLongPress = currentOnLongPress,
                onClick = currentOnSelect,
            )
            // clear：行内是 Text + 图标，直接 semantics 会裂成"外层无名可点 + 内层按钮"两层。
            .clearAndSetSemantics {
                contentDescription = listOfNotNull(
                    item.title,
                    item.value.takeIf { it.isNotBlank() },
                    item.subtitle.takeIf { it.isNotBlank() },
                ).joinToString("，")
                role = Role.Button
                when {
                    item.selected -> stateDescription = "已选中"
                    item.on -> stateDescription = "已开启"
                }
                if (item.enabled) onClick { currentOnSelect(); true }
            }
            .padding(end = PlayerPanelStyle.rowTrailingPad),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = item.title,
                color = PlayerPanelStyle.rowTitleColor,
                fontSize = PlayerPanelStyle.rowTitleSize.sp,
                lineHeight = PlayerPanelStyle.rowTitleLineHeight.sp,
                letterSpacing = PlayerPanelStyle.rowTitleTracking.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            if (tall) {
                Text(
                    text = item.subtitle,
                    color = PlayerPanelStyle.rowSubtitleColor,
                    fontSize = PlayerPanelStyle.subtitleSize.sp,
                    lineHeight = PlayerPanelStyle.subtitleLineHeight.sp,
                    letterSpacing = PlayerPanelStyle.subtitleTracking.sp,
                    maxLines = 2,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
        if (item.value.isNotBlank()) {
            Text(
                text = item.value,
                color = PlayerPanelStyle.valueColor,
                fontSize = PlayerPanelStyle.valueSize.sp,
                lineHeight = PlayerPanelStyle.valueLineHeight.sp,
                // material3 Text 没有 fontFeatureSettings 参数，只能走 TextStyle（§4 的 tnum）。
                style = TextStyle(fontFeatureSettings = PlayerPanelStyle.valueFontFeature),
                fontWeight = FontWeight.Medium,
                textAlign = TextAlign.End,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier
                    .padding(start = 8.dp)
                    .width(PlayerPanelStyle.valueColumn),
            )
        }
        MenuTrailing(item)
    }
}