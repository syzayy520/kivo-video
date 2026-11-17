package com.kivo.cinema.ui.profile.chrome

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 「我的」页环境头：左「本地」26sp Bold，右侧 `[日历入口] [编辑]` 两枚控件。
 *
 * 尺寸口径（基准 `profile-1.png` 量得）：左右 13dp、上 6dp、下 10dp；
 * 编辑胶囊高 36dp / 圆角 18dp / 左右内边距 16dp / 文字 13sp `navActive`；
 * 日历入口 36dp 正圆、白底 `canvasFill`、日历线框图标 16dp 居中、与胶囊间距 8dp
 * （◐ 推断：基准帧里该位置只有「编辑」，圆形入口是本页新增能力，直径与胶囊同高保证基线齐平）。
 *
 * 只画控件、不导航：两个回调由页面提供，本文件不持有 Context、不构造 Intent。
 */
@Composable
fun ProfileEnvironmentHeader(
    title: String,
    onOpenCalendar: () -> Unit,
    onEdit: () -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .padding(top = HeaderTopPad, bottom = HeaderBottomPad),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(EntryGap),
    ) {
        Text(
            text = title,
            fontSize = HeaderTitleSize,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            modifier = Modifier.weight(1f),
        )
        CalendarEntryButton(onClick = onOpenCalendar)
        Box(
            modifier = Modifier
                .height(EditPillHeight)
                .clip(RoundedCornerShape(EditPillRadius))
                .background(DesignTokens.canvasFill)
                .clickable(role = Role.Button, onClick = onEdit)
                .padding(horizontal = EditPillSidePad)
                .semantics { contentDescription = "编辑环境" },
            contentAlignment = Alignment.Center,
        ) {
            Text(text = "编辑", fontSize = EditLabelSize, color = DesignTokens.navActive)
        }
    }
}

/** 36dp 白圆 + 日历线框图标（外壳圆角矩形、顶部两个挂环、中部横线）。整圆都是命中区。 */
@Composable
private fun CalendarEntryButton(onClick: () -> Unit) {
    Box(
        modifier = Modifier
            .size(EntrySize)
            .clip(CircleShape)
            .background(DesignTokens.canvasFill)
            .clickable(role = Role.Button, onClick = onClick)
            .semantics { contentDescription = "打开日历" },
        contentAlignment = Alignment.Center,
    ) {
        Canvas(modifier = Modifier.size(GlyphSize)) {
            val stroke = Stroke(width = GlyphStroke.toPx())
            val bodyTop = size.height * 0.18f
            // 外壳：圆角矩形，留出顶部挂环的位置
            drawRoundRect(
                color = DesignTokens.textMain,
                topLeft = Offset(0f, bodyTop),
                size = Size(size.width, size.height - bodyTop),
                cornerRadius = CornerRadius(GlyphCorner.toPx()),
                style = stroke,
            )
            // 中部横线（日历的「日」分隔）
            val dividerY = bodyTop + (size.height - bodyTop) * 0.34f
            drawLine(
                color = DesignTokens.textMain,
                start = Offset(0f, dividerY),
                end = Offset(size.width, dividerY),
                strokeWidth = GlyphStroke.toPx(),
            )
            // 两个挂环
            val ringX = size.width * 0.28f
            val ringX2 = size.width * 0.72f
            drawLine(
                color = DesignTokens.textMain,
                start = Offset(ringX, 0f),
                end = Offset(ringX, bodyTop + GlyphStroke.toPx()),
                strokeWidth = GlyphStroke.toPx(),
            )
            drawLine(
                color = DesignTokens.textMain,
                start = Offset(ringX2, 0f),
                end = Offset(ringX2, bodyTop + GlyphStroke.toPx()),
                strokeWidth = GlyphStroke.toPx(),
            )
        }
    }
}

private val HeaderTopPad = 6.dp
private val HeaderBottomPad = 10.dp
private val HeaderTitleSize = 26.sp
private val EntryGap = 8.dp
private val EntrySize = 36.dp
private val GlyphSize = 16.dp
private val GlyphStroke = 1.5.dp
private val GlyphCorner = 2.dp
private val EditPillHeight = 36.dp
private val EditPillRadius = 18.dp
private val EditPillSidePad = 16.dp
private val EditLabelSize = 13.sp

@Preview(name = "环境头 · 日历入口", widthDp = 390, showBackground = true)
@Composable
private fun ProfileEnvironmentHeaderPreview() {
    Box(Modifier.background(DesignTokens.pageBackground)) {
        ProfileEnvironmentHeader(title = "本地", onOpenCalendar = {}, onEdit = {})
    }
}