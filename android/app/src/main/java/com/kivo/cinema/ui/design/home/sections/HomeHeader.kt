package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/**
 * 发现页分区标题与 TmdbHome 海报卡（✅ 只保留这一份实现；原 `DiscoverPage.kt` 内联版已迁出）。
 *
 * 【为什么单独成文件】拆卡时只允许一套标题/箭头/海报卡 —— 两套并存正是
 * DISC-12 记的「页内两套箭头」来源。所有分区（旧 section 与 B3 新模块）都调这里。
 */
@Composable
internal fun HomeSectionHeader(title: String, fg: Color, onMore: (() -> Unit)? = null) {
    Row(Modifier.fillMaxWidth().height(22.dp).padding(horizontal = 13.dp),
        verticalAlignment = Alignment.CenterVertically) {
        Text(title, fontSize = 16.sp, lineHeight = 22.sp, fontWeight = FontWeight.Bold,
            color = fg, style = androidx.compose.ui.text.TextStyle(
                platformStyle = androidx.compose.ui.text.PlatformTextStyle(includeFontPadding = false)))
        Spacer(Modifier.weight(1f))
        if (onMore != null) Box(
            Modifier.size(28.dp, 22.dp).semantics { contentDescription = "查看全部$title" }
                .clickable(onClick = onMore), contentAlignment = Alignment.CenterEnd,
        ) {
            Canvas(Modifier.size(8.dp, 13.dp)) {
                val inset = 1.dp.toPx()
                drawPath(Path().apply {
                    moveTo(inset, inset)
                    lineTo(size.width - inset, size.height / 2)
                    lineTo(inset, size.height - inset)
                }, fg, style = Stroke(1.7.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
            }
        }
    }
}

/** Home-only 104×156 poster with compact provider labels. */
@Composable
internal fun TmdbPosterCard(
    m: TmdbHome.Item,
    fg: Color = DesignTokens.textMain,
    onClick: () -> Unit,
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = Modifier
            .width(104.dp)
            .clickable { onClick() },
    ) {
        TmdbImage(m.posterUrl, m.title,
            Modifier
                .size(104.dp, 156.dp)
                .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp)))
        Spacer(Modifier.height(6.dp))
        Text(m.title, fontSize = 12.sp,
            fontWeight = FontWeight.Bold, color = fg,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        Text(m.year(), fontSize = 11.sp,
            color = chromeSubFor(fg))
    }
}

/** 次行颜色跟随分区前景（亮底 hero → textSub，暗底 → 白 72%）。 */
internal fun chromeSubFor(fg: Color): Color =
    if (fg == Color.White) Color.White.copy(alpha = 0.72f) else DesignTokens.textSub