package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ColorFilter
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 设置行共享零件（268 从 SettingsRows 按"变化原因"拆出）：淡底 chip / 标题区 / Beta / 分隔 / 脚注。
 * 这些是"多行共用的视觉原子"，随视觉语言变；行组合在 SettingsRows.kt，色板在 SettingsVisuals.kt。
 * 单文件单职责：本文件只放原子，不放整行。
 */

/** 同色淡底圆角 chip + 彩色线 glyph（268 视觉语言核心）。置灰态降饱和降透明，不假装可用。 */
@Composable
internal fun RowIcon(icon: ImageVector, color: Color, enabled: Boolean = true) {
    val bg = SettingsVisuals.tint(color).let { if (enabled) it else it.copy(alpha = 0.06f) }
    val glyph = if (enabled) color else color.copy(alpha = 0.45f)
    Box(
        Modifier.size(SettingsVisuals.chipSize.dp)
            .clip(RoundedCornerShape(SettingsVisuals.chipRadius.dp)).background(bg),
        contentAlignment = Alignment.Center,
    ) {
        Image(
            imageVector = icon, contentDescription = null,
            modifier = Modifier.size(SettingsVisuals.chipGlyph.dp),
            colorFilter = ColorFilter.tint(glyph),
        )
    }
}

/** 行标题区：标题 17 + 可选 Beta + 可选灰副标题（保留换行，不截断不可用原因）。所有行共用。 */
@Composable
internal fun RowScope.RowLabels(title: String, subtitle: String?, beta: Boolean, modifier: Modifier = Modifier) {
    Column(modifier.weight(1f).padding(end = 12.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
            if (beta) BetaBadge()
        }
        if (subtitle != null)
            Text(subtitle, fontSize = SettingsVisuals.fontSub.sp, color = DesignTokens.textSub, lineHeight = 16.sp)
    }
}

/** 橙色 Beta 徽标（实验性能力右上角）。 */
@Composable
internal fun BetaBadge() {
    val orange = Color(0xFFFF9500)
    Text("Beta", fontSize = 10.sp, fontWeight = FontWeight.Bold, color = orange,
        modifier = Modifier.padding(start = 6.dp).clip(RoundedCornerShape(4.dp))
            .background(orange.copy(alpha = 0.15f)).padding(horizontal = 5.dp, vertical = 1.dp))
}

/** 卡内行分隔线：左对齐文字（16pt 内缩），右到卡边。 */
@Composable
internal fun RowDivider() {
    Box(Modifier.fillMaxWidth().padding(start = 16.dp).height(0.5.dp).background(DesignTokens.hairline))
}

/** 不可用原因单行脚注（组内行下，降噪但不隐藏 reason）。允许换行，绝不截断。 */
@Composable
internal fun RowFootnote(text: String) {
    Text(text, fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub, lineHeight = 14.sp,
        modifier = Modifier.fillMaxWidth().padding(start = 16.dp, end = 16.dp, bottom = 8.dp))
}

/** 行外框：无脚注直接渲染行；有脚注则行下追加原因（置灰降噪用）。 */
@Composable
internal fun RowFrame(footnote: String?, content: @Composable () -> Unit) {
    if (footnote == null) content()
    else Column(Modifier.fillMaxWidth()) { content(); RowFootnote(footnote) }
}