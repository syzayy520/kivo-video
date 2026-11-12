package com.kivo.cinema.ui.detail

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.detail.actions.ActionGlyph
import com.kivo.cinema.ui.detail.actions.DetailActionGlyph

/**
 * 255 剧集节（REX-EMBY 0178/0179）：节标题即选季触发器（「第 N 季 ⌄」左对齐），
 * 下接集缩略图横排。集卡 meta 四行＝日期 / 第N集+时长 / 粗标题 / 两行简介（0179），
 * 播放按钮指向的下一集带白描边（0178 新状态 E01 带边）。
 * 缩略图需 token，经 [loadImage] 注入；空/错一律诚实文案。浮层本体在 [SeasonMenuOverlay]。
 */
@Composable
internal fun SeasonEpisodeSection(
    seasons: List<EmbyShows.Season>,
    current: EmbyShows.Season?,
    episodes: List<EmbyShows.Episode>,
    loading: Boolean,
    error: String?,
    fg: Color,
    fgSub: Color,
    loadImage: (suspend (String) -> Bitmap?)?,
    calendarLabel: String,
    onOpenCalendar: () -> Unit,
    menuOpen: Boolean,
    onOpenMenu: () -> Unit,
    onAnchor: (Rect) -> Unit,
    onPickSeason: (EmbyShows.Season) -> Unit,
    onRetry: () -> Unit,
    onPlay: (EmbyShows.Episode) -> Unit,
) {
    val nextUpId = episodes.firstOrNull { !it.played && it.positionTicks > 0 }?.id
        ?: episodes.firstOrNull { !it.played }?.id
    Column(Modifier.fillMaxWidth()) {
        Box(Modifier.padding(horizontal = DETAIL_PAD_DP.dp)) {
            SeasonTrigger(
                name = current?.name ?: "剧集",
                expandable = seasons.size > 1,
                expanded = menuOpen,
                fg = fg,
                onAnchor = onAnchor,
                onClick = onOpenMenu,
            )
        }
        Spacer(Modifier.height(14.dp))
        if (calendarLabel.isNotEmpty()) {
            CalendarBar(calendarLabel, fg, fgSub, onOpenCalendar)
            Spacer(Modifier.height(14.dp))
        }
        when {
            seasons.isEmpty() -> SectionNote("该服务器未返回剧集信息", fgSub)
            loading -> Box(Modifier.fillMaxWidth().height(120.dp),
                contentAlignment = Alignment.Center) {
                CircularProgressIndicator(Modifier.size(28.dp), color = fg)
            }
            error != null -> Column(Modifier.fillMaxWidth(),
                horizontalAlignment = Alignment.CenterHorizontally) {
                SectionNote(error, fgSub)
                TextButton(onClick = onRetry) { Text("重试", color = fg) }
            }
            episodes.isEmpty() -> SectionNote("本季暂无剧集", fgSub)
            else -> LazyRow(
                horizontalArrangement = Arrangement.spacedBy(12.dp),
                contentPadding = PaddingValues(horizontal = DETAIL_PAD_DP.dp),
            ) {
                items(episodes, key = { it.id }) { ep ->
                    EpisodeCard(ep, ep.id == nextUpId, fg, fgSub, loadImage, onPlay)
                }
            }
        }
        if (episodes.isNotEmpty()) {
            Spacer(Modifier.height(16.dp))
            Box(Modifier.fillMaxWidth(), contentAlignment = Alignment.Center) {
                Box(Modifier.clip(RoundedCornerShape(50)).background(fg.copy(alpha = .08f))
                    .padding(horizontal = 14.dp, vertical = 6.dp)) {
                    Text("${episodes.first().index} · ${episodes.last().index}",
                        fontSize = 13.sp, color = fgSub)
                }
            }
        }
    }
}

/** 播出日历条（REX）：左日历图标+「播出日历」，右进度标签+chevron；点击开日历浮层。 */
@Composable
private fun CalendarBar(label: String, fg: Color, fgSub: Color, onClick: () -> Unit) {
    Row(Modifier.fillMaxWidth().padding(horizontal = DETAIL_PAD_DP.dp)
        .clip(RoundedCornerShape(14.dp)).background(fg.copy(alpha = .06f))
        .clickable(role = Role.Button, onClick = onClick)
        .padding(horizontal = 14.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically) {
        DetailActionGlyph(ActionGlyph.Calendar, fg, Modifier.size(19.dp))
        Spacer(Modifier.width(9.dp))
        Text("播出日历", fontSize = 15.sp, fontWeight = FontWeight.Medium, color = fg)
        Spacer(Modifier.weight(1f))
        Text(label, fontSize = 13.sp, color = fgSub, maxLines = 1)
        Spacer(Modifier.width(6.dp))
        DetailActionGlyph(ActionGlyph.Chevron, fgSub, Modifier.size(15.dp))
    }
}

@Composable
private fun EpisodeCard(ep: EmbyShows.Episode, highlight: Boolean, fg: Color, fgSub: Color,
                        loadImage: (suspend (String) -> Bitmap?)?,
                        onPlay: (EmbyShows.Episode) -> Unit) {
    val thumbShape = RoundedCornerShape(DesignTokens.radiusCard.dp)
    Column(Modifier.width(200.dp).clip(thumbShape).clickable { onPlay(ep) }) {
        Box(
            Modifier.fillMaxWidth().aspectRatio(16f / 9f)
                .clip(thumbShape).background(Color(0xFF2A2731))
                .then(if (highlight) Modifier.border(2.dp, Color.White, thumbShape)
                    else Modifier),
            contentAlignment = Alignment.Center,
        ) {
            EpisodeThumb(ep.thumbUrl, loadImage)
            if (ep.played) {
                Box(Modifier.align(Alignment.TopEnd).padding(6.dp)
                    .clip(RoundedCornerShape(50)).background(Color.Black.copy(alpha = .55f))
                    .padding(horizontal = 7.dp, vertical = 2.dp)) {
                    Text("已看", fontSize = 10.sp, color = Color.White)
                }
            } else if (ep.positionTicks > 0 && ep.runtimeMinutes > 0) {
                val frac = (ep.positionTicks.toDouble() / (ep.runtimeMinutes * 600_000_000L))
                    .coerceIn(0.0, 1.0).toFloat()
                Box(Modifier.align(Alignment.BottomStart)
                    .fillMaxWidth(frac).height(3.dp).background(Color.White.copy(alpha = .9f)))
            }
        }
        Spacer(Modifier.height(8.dp))
        if (ep.airDate.isNotBlank()) {
            Text(ep.airDate, fontSize = 13.sp, color = fgSub, maxLines = 1)
            Spacer(Modifier.height(2.dp))
        }
        Text("第${ep.index}集" + if (ep.runtimeMinutes > 0) "  ${ep.runtimeMinutes}分钟" else "",
            fontSize = 13.sp, color = fgSub, maxLines = 1)
        Spacer(Modifier.height(2.dp))
        Text(ep.name, fontSize = 15.sp, fontWeight = FontWeight.Bold, color = fg,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        if (ep.overview.isNotBlank()) {
            Spacer(Modifier.height(3.dp))
            Text(ep.overview, fontSize = 12.sp, lineHeight = 16.sp, color = fgSub,
                maxLines = 2, overflow = TextOverflow.Ellipsis)
        }
    }
}

@Composable
private fun EpisodeThumb(url: String, loadImage: (suspend (String) -> Bitmap?)?) {
    var bitmap by remember(url) { mutableStateOf<Bitmap?>(null) }
    LaunchedEffect(url) {
        bitmap = if (url.isBlank() || loadImage == null) null else loadImage(url)
    }
    if (bitmap != null) {
        Image(bitmap!!.asImageBitmap(), contentDescription = null,
            contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
    } else {
        Text("暂无缩略图", fontSize = 12.sp, color = Color.White.copy(alpha = .5f))
    }
}

@Composable
private fun SectionNote(text: String, color: Color) {
    Box(Modifier.fillMaxWidth().height(80.dp), contentAlignment = Alignment.Center) {
        Text(text, fontSize = 13.sp, color = color)
    }
}