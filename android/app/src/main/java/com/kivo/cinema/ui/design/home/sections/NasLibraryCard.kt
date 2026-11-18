package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 本地片库的竖版海报卡 —— **严格照抄「正在热映」的 [TmdbPosterCard] 规格**：
 * 104×156（2:3）、标题 12sp Bold 单行、副标题 11sp 灰、图上文字下留 6dp、整卡居中。
 *
 * 用户明确要求：本地片库必须与其他竖向展示一致，不许另搞一套横向卡片。
 * 所以尺寸/字号/间距都从这里对齐，不另立 token。
 */
@Composable
internal fun NasLibraryCard(
    entry: HomeEntry,
    fg: Color,
    sub: Color,
    posterBlock: (@Composable () -> Unit)?,
    onClick: () -> Unit,
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = Modifier.width(POSTER_CARD_WIDTH).clickable { onClick() },
    ) {
        Box(
            Modifier
                .size(POSTER_CARD_WIDTH, POSTER_CARD_HEIGHT)
                .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp))
                .background(Color(0xFF24242A)),
        ) {
            if (posterBlock != null) posterBlock()
        }
        Spacer(Modifier.height(6.dp))
        Text(
            entry.title, fontSize = 12.sp, fontWeight = FontWeight.Bold, color = fg,
            maxLines = 1, overflow = TextOverflow.Ellipsis,
        )
        Text(
            entry.episodeLabel?.let { "${entry.sourceLabel} · $it" } ?: entry.sourceLabel,
            fontSize = 11.sp, color = sub,
        )
    }
}

/** 与 TmdbPosterCard 同值（104 / 156），改这里等于两处一起改，保持两条行完全对齐。 */
private val POSTER_CARD_WIDTH = 104.dp
private val POSTER_CARD_HEIGHT = 156.dp
