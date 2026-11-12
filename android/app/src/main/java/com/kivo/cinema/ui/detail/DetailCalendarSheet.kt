package com.kivo.cinema.ui.detail

import android.graphics.Bitmap
import android.os.Build
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.animation.core.updateTransition
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyShows
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.RatingBadges
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.NavigationBackdrop
import com.kivo.cinema.ui.design.home.NavigationBackdropState
import com.kivo.cinema.ui.detail.actions.ActionGlyph
import com.kivo.cinema.ui.detail.actions.DetailActionGlyph
import java.time.LocalDate
import java.time.YearMonth

private val CalInk = Color(0xFF292A30)
private val CalMuted = Color(0xFF77777F)

/**
 * 255 #31 追剧日历浮层（REX 帧复刻）：底部升起的大毛玻璃卡（真背景模糊，复用导航捕获桥）。
 * 头部＝海报 + 中文标题 + 第N季·共M集 + 简介 + 评分 + 出品 logo；中部＝月历；
 * 底栏＝服务器名 + 分享（真 ACTION_SEND）+ 移除（仅在待看时）+ 完成。无播日诚实提示，不造假。
 */
@Composable
internal fun DetailCalendarSheet(
    open: Boolean,
    details: TmdbDetail.Details?,
    season: EmbyShows.Season?,
    episodes: List<EmbyShows.Episode>,
    serverName: String,
    backdrop: NavigationBackdropState,
    loadImage: (suspend (String) -> Bitmap?)?,
    inWatchlist: Boolean,
    onDismiss: () -> Unit,
    onShare: () -> Unit,
    onRemove: () -> Unit,
) {
    val transition = updateTransition(open, label = "calendarSheet")
    val fade by transition.animateFloat(
        transitionSpec = { tween(if (targetState) 200 else 160) }, label = "fade") { if (it) 1f else 0f }
    val slide by transition.animateFloat(
        transitionSpec = { if (targetState) spring(dampingRatio = .82f, stiffness = 360f) else tween(160) },
        label = "slide") { if (it) 0f else 1f }
    if (!transition.currentState && !transition.targetState) return
    val today = remember { LocalDate.now() }
    var ym by remember(open) { mutableStateOf(YearMonth.from(today)) }
    val days = remember(episodes) { DetailCalendarData.airDays(episodes) }
    val month = remember(ym, days, today) { DetailCalendarData.month(ym, days, today) }
    val shape = RoundedCornerShape(topStart = 30.dp, topEnd = 30.dp)
    Box(Modifier.fillMaxSize()) {
        Box(Modifier.fillMaxSize().background(Color.Black.copy(alpha = .32f * fade))
            .clickable(interactionSource = remember { MutableInteractionSource() },
                indication = null) { onDismiss() })
        Box(Modifier.align(Alignment.BottomCenter).fillMaxWidth()
            .graphicsLayer { alpha = fade; translationY = slide * size.height }
            .clip(shape)) {
            val blurOk = Build.VERSION.SDK_INT >= 31
            if (blurOk) NavigationBackdrop(backdrop, Modifier.matchParentSize(), blurRadius = 60f)
            Box(Modifier.matchParentSize()
                .background(Color.White.copy(alpha = if (blurOk) .80f else .97f)))
            Column(Modifier.fillMaxWidth().verticalScroll(rememberScrollState())
                .padding(horizontal = 22.dp).padding(top = 22.dp, bottom = 26.dp)) {
                CalendarHeader(details, season, episodes, loadImage)
                Spacer(Modifier.height(18.dp))
                if (days.isEmpty()) {
                    Box(Modifier.fillMaxWidth().height(120.dp), contentAlignment = Alignment.Center) {
                        Text("暂无播出日程信息", fontSize = 13.sp, color = CalMuted)
                    }
                } else {
                    DetailCalendarGrid(month, CalInk, CalMuted,
                        onPrev = { ym = ym.minusMonths(1) }, onNext = { ym = ym.plusMonths(1) })
                }
                Spacer(Modifier.height(20.dp))
                CalendarBottomBar(serverName, inWatchlist, onShare, onRemove, onDismiss)
            }
        }
    }
}

@Composable
private fun CalendarHeader(
    d: TmdbDetail.Details?, season: EmbyShows.Season?,
    episodes: List<EmbyShows.Episode>, loadImage: (suspend (String) -> Bitmap?)?,
) {
    Row(Modifier.fillMaxWidth()) {
        Box(Modifier.size(92.dp, 132.dp).clip(RoundedCornerShape(12.dp))) {
            TmdbImage(d?.posterUrl.orEmpty(), d?.title.orEmpty(), Modifier.fillMaxSize(),
                loader = loadImage)
        }
        Spacer(Modifier.width(14.dp))
        Column(Modifier.weight(1f)) {
            Text(d?.title.orEmpty(), fontSize = 20.sp, fontWeight = FontWeight.Bold,
                color = CalInk, maxLines = 2, overflow = TextOverflow.Ellipsis)
            val seasonLine = when {
                season != null -> "第${season.index}季 · 共${episodes.size}集"
                episodes.isNotEmpty() -> "共${episodes.size}集"
                else -> ""
            }
            if (seasonLine.isNotEmpty()) {
                Spacer(Modifier.height(5.dp))
                Text(seasonLine, fontSize = 13.sp, color = CalMuted)
            }
            if (!d?.overview.isNullOrBlank()) {
                Spacer(Modifier.height(7.dp))
                Text(d!!.overview, fontSize = 12.sp, lineHeight = 17.sp, color = CalMuted,
                    maxLines = 3, overflow = TextOverflow.Ellipsis)
            }
            if ((d?.rating ?: 0f) > 0f) {
                Spacer(Modifier.height(8.dp))
                RatingBadges(listOf(d!!.ratingSource to d.rating), alignCenter = false, valueColor = CalInk)
            }
        }
        d?.studioLogo?.let { logo ->
            Spacer(Modifier.width(10.dp))
            TmdbImage(logo, "出品方", Modifier.height(22.dp).widthIn(max = 64.dp),
                bg = Color.Transparent, fit = true, loader = loadImage)
        }
    }
}

@Composable
private fun CalendarBottomBar(
    serverName: String, inWatchlist: Boolean,
    onShare: () -> Unit, onRemove: () -> Unit, onDismiss: () -> Unit,
) {
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text(serverName, fontSize = 12.sp, color = CalMuted, maxLines = 1,
            overflow = TextOverflow.Ellipsis, modifier = Modifier.weight(1f))
        CalCircle(ActionGlyph.Share, "分享", onShare)
        if (inWatchlist) {
            Spacer(Modifier.width(8.dp))
            CalCircle(ActionGlyph.Trash, "移出待看", onRemove)
        }
        Spacer(Modifier.width(10.dp))
        Box(Modifier.clip(RoundedCornerShape(50)).background(CalInk)
            .clickable(onClick = onDismiss).padding(horizontal = 20.dp, vertical = 9.dp)) {
            Text("完成", fontSize = 14.sp, fontWeight = FontWeight.Medium, color = Color.White)
        }
    }
}

@Composable
private fun CalCircle(glyph: ActionGlyph, label: String, onClick: () -> Unit) {
    Box(Modifier.size(40.dp).clip(CircleShape).background(Color(0xFFECEAEC))
        .clickable(onClick = onClick), contentAlignment = Alignment.Center) {
        DetailActionGlyph(glyph, CalInk, Modifier.size(19.dp))
    }
}