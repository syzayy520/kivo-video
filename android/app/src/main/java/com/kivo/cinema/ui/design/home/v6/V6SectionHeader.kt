package com.kivo.cinema.ui.design.home.v6

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.foundation.Canvas
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp

/**
 * v6 区块标题行：标题 21sp/600 + 可选副标题（如「本周 +23」）+ 36dp 圆形「更多」点击区。
 *
 * [onMore] 为 null 时不画箭头 —— 没有落地页就不给可点的假象。
 */
@Composable
internal fun V6SectionHeader(
    title: String,
    fg: Color,
    sub: Color,
    subtitle: String? = null,
    onMore: (() -> Unit)? = null,
) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = 36.dp).padding(horizontal = V6Metrics.screenPad),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(title, color = fg, fontSize = V6Metrics.fsSection, fontWeight = FontWeight.SemiBold)
        if (subtitle != null) {
            Spacer(Modifier.width(8.dp))
            Text(subtitle, color = sub, fontSize = V6Metrics.fsSectionSub)
        }
        Spacer(Modifier.weight(1f))
        if (onMore != null) Box(
            Modifier.offset(x = 8.dp).size(36.dp).clip(CircleShape).clickable(onClick = onMore),
            contentAlignment = Alignment.Center,
        ) { Chevron(sub) }
    }
}

/** 线性右箭头（与 v6 稿的 2.1 描边一致）。 */
@Composable
private fun Chevron(color: Color) {
    Canvas(Modifier.size(20.dp)) {
        val w = size.width
        val path = Path().apply {
            moveTo(w * .40f, w * .25f); lineTo(w * .65f, w * .50f); lineTo(w * .40f, w * .75f)
        }
        drawPath(path, color, style = Stroke(width = 2.1.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
    }
}

