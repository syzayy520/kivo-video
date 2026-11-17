package com.kivo.cinema.ui.profile.calendar

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.onClick
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 日历顶栏的两个可点控件（从 296 行的 `CalendarPageChrome` 迁出，给它留出门限空间）。
 *
 * D14：`SwitchCapsule` 改用 `selectable(Role.Tab)`，TalkBack 能念"已选中"；
 * `PeriodArrow` 之前是"有权限、无名"的哑控件，现在补上"上个月/下个月"。
 * D6：箭头命中 44 → 48dp，视觉箭头仍是 16dp（透明圆无底色，视觉零变化）。
 */

/** 月 / 年分段切换：两枚独立胶囊，不做轨道（轨道会让 30dp 的胶囊显小）。 */
@Composable
internal fun SwitchCapsule(label: String, selected: Boolean, onClick: () -> Unit) {
    val shape = RoundedCornerShape(SwitchCorner)
    Box(
        modifier = Modifier
            .width(SwitchWidth)
            .height(SwitchHeight)
            .clip(shape)
            .then(
                if (selected) {
                    Modifier.background(DesignTokens.canvasFill).border(1.dp, DesignTokens.hairline, shape)
                } else {
                    Modifier
                }
            )
            .selectable(selected = selected, role = Role.Tab, onClick = onClick)
            // clear：否则 minimumInteractiveComponentSize 会多出一层无名可点节点。
            .clearAndSetSemantics {
                contentDescription = label
                role = Role.Tab
                stateDescription = if (selected) "已选中" else "未选中"
                onClick { onClick(); true }
            },
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = label,
            fontSize = SwitchTextSize,
            fontWeight = if (selected) FontWeight.Medium else FontWeight.Normal,
            color = if (selected) DesignTokens.textMain else DesignTokens.textSub,
        )
    }
}

@Composable
internal fun PeriodArrow(pointsLeft: Boolean, label: String, onClick: () -> Unit) {
    Box(
        modifier = Modifier
            .size(ArrowHit)
            .clip(CircleShape)
            .semantics {
                contentDescription = label
                role = Role.Button
            }
            .clickable(role = Role.Button, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Chevron(pointsLeft = pointsLeft, size = ArrowGlyph, color = DesignTokens.textSub)
    }
}

/** `‹` / `›` 用两笔折线画，避免字体差异导致箭头粗细不一致。顶栏返回键也用它。 */
@Composable
internal fun Chevron(pointsLeft: Boolean, size: Dp, color: Color) {
    Canvas(modifier = Modifier.size(size)) {
        val stroke = ChevronStroke.toPx()
        val tipX = if (pointsLeft) this.size.width * 0.34f else this.size.width * 0.66f
        val backX = if (pointsLeft) this.size.width * 0.64f else this.size.width * 0.36f
        drawLine(
            color = color,
            start = Offset(backX, this.size.height * 0.18f),
            end = Offset(tipX, this.size.height * 0.5f),
            strokeWidth = stroke,
            cap = StrokeCap.Round,
        )
        drawLine(
            color = color,
            start = Offset(tipX, this.size.height * 0.5f),
            end = Offset(backX, this.size.height * 0.82f),
            strokeWidth = stroke,
            cap = StrokeCap.Round,
        )
    }
}

internal val ArrowHit = 48.dp
internal val ArrowGlyph = 16.dp
private val SwitchHeight = 30.dp
private val SwitchWidth = 56.dp
private val SwitchCorner = 15.dp
private val SwitchTextSize = 13.sp
private val ChevronStroke = 1.8.dp
