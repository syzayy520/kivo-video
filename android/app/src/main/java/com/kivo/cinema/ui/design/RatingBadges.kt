package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 评分徽章行（grid_02 帧 + 237 方案 F2）：徽章块 + 数值 13pt，横排居中。
 * v1 数据源只有 TMDB（青绿块）；IMDb/豆瓣无公开数据源——缺源的徽章不渲染（不显 0）。
 */
@Composable
fun RatingBadges(
    ratings: List<Pair<String, Float>>,   // (来源名, 分数)
    modifier: Modifier = Modifier,
    alignCenter: Boolean = false,
) {
    if (ratings.isEmpty()) return
    Row(
        modifier = modifier,
        horizontalArrangement = if (alignCenter) Arrangement.Center
                                else Arrangement.spacedBy(10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        ratings.forEach { (source, score) ->
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    source,
                    fontSize = 9.sp,
                    fontWeight = FontWeight.Bold,
                    color = Color.White,
                    modifier = Modifier
                        .background(badgeColor(source), RoundedCornerShape(3.dp))
                        .padding(horizontal = 4.dp, vertical = 1.dp),
                )
                Text(
                    "%.1f".format(score),
                    fontSize = 13.sp,
                    color = DesignTokens.textMain,
                    modifier = Modifier.padding(start = 3.dp),
                )
            }
        }
    }
}

private fun badgeColor(source: String): Color = when (source) {
    "IMDb" -> Color(0xFFF5C518)
    "TMDB" -> Color(0xFF00B8B8)   // 青绿（帧实证）
    "豆瓣" -> Color(0xFF6B52A0)
    else -> Color(0xFF666666)
}
