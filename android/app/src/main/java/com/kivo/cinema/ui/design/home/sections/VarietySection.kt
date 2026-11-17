package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface
import com.kivo.cinema.ui.design.TmdbImage

/**
 * 近一月·热门综艺大卡（hero_32 帧实测：全宽玻璃卡 = 顶部特写图（月桂+节题叠印）
 * + 下部 16:9 缩略横排）。详情页 v2（237 §8 勘误 #4：v1 默认模块流）。✅ 等值搬迁。
 */
@Composable
internal fun VarietySection(
    list: List<TmdbHome.Item>,
    fg: Color,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        GlassSurface(Modifier.padding(horizontal = 13.dp)) {
            Column {
                Box(
                    Modifier
                        .fillMaxWidth()
                        .height(150.dp)
                ) {
                    TmdbImage(list.first().backdropUrl, list.first().title,
                        Modifier.fillMaxSize())
                    Box(
                        Modifier
                            .fillMaxSize()
                            .background(Brush.verticalGradient(
                                0.3f to Color.Transparent,
                                1f to Color.Black.copy(alpha = 0.55f))))
                    Column(
                        Modifier.align(Alignment.BottomCenter).padding(bottom = 10.dp),
                        horizontalAlignment = Alignment.CenterHorizontally,
                    ) {
                        Text("近一月 · 热门综艺", fontSize = 20.sp, fontWeight = FontWeight.Bold,
                            color = Color.White)
                        Text("The most popular variety shows in the past month",
                            fontSize = 11.sp, color = Color.White.copy(alpha = 0.8f))
                    }
                }
                Spacer(Modifier.height(8.dp))
                LazyRow(
                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                    contentPadding = PaddingValues(horizontal = 13.dp),
                ) {
                    items(list.take(10)) { m ->
                        Column(Modifier.width(103.dp).clickable { onOpenTmdbItem(m) }) {
                            Box(
                                Modifier
                                    .size(103.dp, 58.dp)
                                    .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
                            ) { TmdbImage(m.backdropUrl, m.title, Modifier.fillMaxSize()) }
                            Spacer(Modifier.height(4.dp))
                            Text(m.title, fontSize = 12.sp, color = DesignTokens.textMain,
                                maxLines = 1, overflow = TextOverflow.Ellipsis)
                            Text(m.tagLine(), fontSize = 11.sp, color = DesignTokens.textSub,
                                maxLines = 1, overflow = TextOverflow.Ellipsis)
                        }
                    }
                }
                Spacer(Modifier.height(10.dp))
            }
        }
    }
}