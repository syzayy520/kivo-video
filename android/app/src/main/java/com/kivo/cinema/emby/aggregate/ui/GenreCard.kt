package com.kivo.cinema.emby.aggregate.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.emby.aggregate.AggregateSource
import com.kivo.cinema.emby.aggregate.GenreWallData
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 256 类型墙单卡：渐变底 + 左上白色粗体类型名 + 右侧扇形旋转的**真实**海报堆。
 * 一个组件二十四份数据（[GenreWallData.wall]）。海报懒加载（进入视口才拉），无海报只显渐变与名字，不放假图。
 * 窄卡（手机两列）收为两张小海报，避免扇形压住类型名；宽卡（平板）三张。
 */
@Composable
fun GenreCard(
    genre: GenreWallData.Genre, source: AggregateSource?, onClick: () -> Unit,
) {
    var posters by remember(genre.name) { mutableStateOf<List<EmbyItem>>(emptyList()) }
    LaunchedEffect(genre.name, source?.id) {
        if (source == null) return@LaunchedEffect
        posters = withContext(Dispatchers.IO) { GenreWallData.posters(listOf(source), genre.name) }
    }
    BoxWithConstraints(Modifier.fillMaxWidth().aspectRatio(1.9f).clip(RoundedCornerShape(16.dp))
        .background(Brush.linearGradient(listOf(genre.start, genre.end)))
        .clickable(interactionSource = remember { MutableInteractionSource() }, indication = null, onClick = onClick)) {
        val compact = maxWidth < 220.dp
        val shown = posters.take(if (compact) 2 else 3)
        if (source != null && shown.isNotEmpty()) {
            val pw = if (compact) 44.dp else 60.dp
            val ph = if (compact) 64.dp else 88.dp
            val spread = if (compact) 22f else 32f
            Box(Modifier.align(Alignment.CenterEnd).padding(end = 8.dp)
                .width(if (compact) 96.dp else 150.dp).height(if (compact) 70.dp else 110.dp)) {
                val mid = (shown.size - 1) / 2f
                shown.forEachIndexed { i, item ->
                    AggregatePoster(source, item.posterUrl,
                        Modifier.align(Alignment.Center).width(pw).height(ph)
                            .graphicsLayer {
                                rotationZ = (i - mid) * 8f
                                translationX = (i - mid) * spread
                            },
                        shape = RoundedCornerShape(8.dp))
                }
            }
        }
        Text(genre.name, color = Color.White, fontSize = if (compact) 18.sp else 20.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.align(Alignment.TopStart).padding(14.dp))
    }
}