package com.kivo.cinema.ui.design.home.v6.list

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.home.v6.V6Metrics

/*
 * 二级列表页的共用外框（TMDB 榜单页与 NAS 分类页同一套）：浮动顶栏 + 居中状态文案。
 */

@Composable
internal fun V6ListHint(text: String) {
    Box(Modifier.fillMaxWidth().padding(vertical = 24.dp), contentAlignment = Alignment.Center) {
        Text(text, color = DesignTokens.textSub, fontSize = 13.sp)
    }
}


/** 顶栏：页面底色向下渐隐（内容滑到下面不突兀）+ 左侧 44dp 玻璃返回钮 + 居中标题。 */
@Composable
internal fun V6ListTopBar(title: String, top: androidx.compose.ui.unit.Dp, onBack: () -> Unit) {
    val bg = DesignTokens.pageBackground
    Box(
        Modifier.fillMaxWidth().height(top + 68.dp)
            .background(Brush.verticalGradient(0f to bg, .78f to bg.copy(alpha = .92f), 1f to bg.copy(alpha = 0f)))
            .padding(top = top, start = V6Metrics.screenPad, end = V6Metrics.screenPad),
    ) {
        Box(
            Modifier.align(Alignment.CenterStart).size(44.dp).shadow(6.dp, CircleShape).clip(CircleShape)
                .background(Color.White.copy(alpha = .9f)).clickable(onClick = onBack),
            contentAlignment = Alignment.Center,
        ) {
            Canvas(Modifier.size(20.dp)) {
                val w = size.width
                drawPath(Path().apply { moveTo(w * .62f, w * .2f); lineTo(w * .32f, w * .5f); lineTo(w * .62f, w * .8f) },
                    DesignTokens.textMain, style = Stroke(2.2.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
            }
        }
        Text(title, Modifier.align(Alignment.Center).width(240.dp), color = DesignTokens.textMain, fontSize = 17.sp,
            fontWeight = FontWeight.SemiBold, maxLines = 1, overflow = TextOverflow.Ellipsis,
            textAlign = androidx.compose.ui.text.style.TextAlign.Center)
    }
}
