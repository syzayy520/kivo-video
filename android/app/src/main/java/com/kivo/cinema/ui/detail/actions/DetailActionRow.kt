package com.kivo.cinema.ui.detail.actions

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.layout.positionInRoot
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.unit.toSize
import com.kivo.cinema.media.marks.FilmMark
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.detail.DETAIL_PAD_DP

/** 独立操作行；共享胶囊含两个独立可达触区。「更多」上报锚点，由 DetailPage 开毛玻璃菜单。 */
@Composable
internal fun DetailActionRow(
    playable: Boolean, fg: Color, actions: DetailActions,
    playSubtitle: String?,
    onPlay: () -> Unit, onOpenMore: (Rect) -> Unit,
) {
    var marksOpen by remember { mutableStateOf(false) }
    var moreAnchor by remember { mutableStateOf(Rect.Zero) }
    val hair = Color.White.copy(alpha = .10f)
    val glyph = fg.copy(alpha = .95f)
    Column {
        Row(Modifier.fillMaxWidth()
            .padding(start = DETAIL_PAD_DP.dp, end = DETAIL_PAD_DP.dp, top = 10.dp),
            horizontalArrangement = Arrangement.spacedBy(9.dp),
            verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.weight(1f).height(45.dp).clip(RoundedCornerShape(50))
                .background(Color.White.copy(alpha = if (playable) .92f else .60f))
                .border(1.dp, hair, RoundedCornerShape(50))
                .clickable(role = Role.Button, onClick = onPlay),
                contentAlignment = Alignment.Center) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(if (playable) "▶  播放" else "未加载到相关资源",
                        fontSize = 15.sp, fontWeight = FontWeight.Medium,
                        color = DesignTokens.textMain, maxLines = 1, overflow = TextOverflow.Ellipsis)
                    if (playable && playSubtitle != null) {
                        Text(playSubtitle, fontSize = 11.sp,
                            color = DesignTokens.textMain.copy(alpha = .55f), maxLines = 1)
                    }
                }
            }
            Row(Modifier.width(101.dp).height(45.dp).clip(RoundedCornerShape(50))
                .background(Color.White.copy(alpha = .147f))
                .border(1.dp, hair, RoundedCornerShape(50))) {
                Box(Modifier.weight(1f).fillMaxHeight()
                    .semantics {
                        contentDescription = "影片标记"
                        stateDescription = listOfNotNull(
                            "待看".takeIf { actions.marks.watchlist },
                            "已观看".takeIf { actions.marks.watched }).joinToString("、")
                            .ifEmpty { "未标记" }
                    }.clickable(enabled = actions.enabled, role = Role.Button) { marksOpen = true },
                    contentAlignment = Alignment.Center) {
                    CollectionPlusIcon(glyph, Modifier.size(22.dp))
                    if (actions.marks.watchlist || actions.marks.watched)
                        Canvas(Modifier.align(Alignment.TopEnd).padding(7.dp).size(5.dp)) {
                            drawCircle(glyph)
                        }
                }
                val heartScale by animateFloatAsState(
                    if (actions.marks.favorite) 1.08f else 1f, label = "favorite")
                Box(Modifier.weight(1f).fillMaxHeight()
                    .semantics { contentDescription = "收藏" }
                    .toggleable(actions.marks.favorite, enabled = actions.enabled,
                        role = Role.Checkbox) { actions.toggle(FilmMark.FAVORITE) },
                    contentAlignment = Alignment.Center) {
                    HeartIcon(if (actions.marks.favorite) Color(0xFFE15479) else glyph,
                        Modifier.size(20.dp).graphicsLayer { scaleX = heartScale; scaleY = heartScale },
                        filled = actions.marks.favorite)
                }
            }
            Box(Modifier.size(45.dp).clip(CircleShape)
                .background(Color.White.copy(alpha = .147f)).border(1.dp, hair, CircleShape)
                .onGloballyPositioned { moreAnchor = Rect(it.positionInRoot(), it.size.toSize()) }
                .semantics { contentDescription = "更多操作" }
                .clickable(role = Role.Button) { onOpenMore(moreAnchor) },
                contentAlignment = Alignment.Center) {
                Canvas(Modifier.size(20.dp, 4.dp)) {
                    val r = 2.dp.toPx()
                    val gap = (size.width - r * 2) / 2f
                    for (i in 0..2) drawCircle(glyph, r, Offset(r + gap * i, size.height / 2f))
                }
            }
        }
        actions.error?.let { error ->
            Text(error, color = fg, fontSize = 13.sp,
                modifier = Modifier.padding(horizontal = DETAIL_PAD_DP.dp, vertical = 8.dp))
            Text("重试标记", color = fg, fontSize = 14.sp,
                modifier = Modifier.padding(horizontal = DETAIL_PAD_DP.dp)
                    .clickable(role = Role.Button, onClick = actions.retry).padding(vertical = 12.dp))
        }
    }
    if (marksOpen) DetailActionSheet(actions, onDismiss = { marksOpen = false })
}