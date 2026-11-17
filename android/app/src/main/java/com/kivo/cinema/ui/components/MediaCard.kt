package com.kivo.cinema.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/** 媒体卡比例：Poster 2:3、Wide 16:9 定宽、FullWidth 全宽 16:9。 */
enum class MediaCardVariant { Poster, Wide, FullWidth }

/** 统一媒体卡：媒体槽 + 标题 + 副标题，三种比例共用一套排版与圆角（报告 §6.1）。 */
@Composable
fun MediaCard(
    title: String,
    subtitle: String,
    variant: MediaCardVariant,
    onClick: () -> Unit,
    media: @Composable () -> Unit,
) {
    val containerModifier = when (variant) {
        MediaCardVariant.Poster -> Modifier.width(PosterWidth)
        MediaCardVariant.Wide -> Modifier.width(WideWidth)
        MediaCardVariant.FullWidth -> Modifier.fillMaxWidth()
    }
    val mediaModifier = when (variant) {
        MediaCardVariant.Poster -> Modifier.size(PosterWidth, PosterHeight)
        MediaCardVariant.Wide -> Modifier.size(WideWidth, WideHeight)
        MediaCardVariant.FullWidth -> Modifier.fillMaxWidth().aspectRatio(WideRatio)
    }
    val radius = when (variant) {
        MediaCardVariant.FullWidth -> DesignTokens.radiusCard
        else -> DesignTokens.radiusPoster
    }
    val titleSize = when (variant) {
        MediaCardVariant.Wide -> DesignTokens.fontPosterDate
        else -> DesignTokens.fontPosterTitle
    }
    val subtitleSize = when (variant) {
        MediaCardVariant.Wide -> CompactSubtitleSize
        else -> DesignTokens.fontPosterDate
    }
    Column(
        modifier = containerModifier.clickable(role = Role.Button, onClick = onClick),
    ) {
        Box(
            modifier = mediaModifier
                .clip(RoundedCornerShape(radius.dp))
                .background(DesignTokens.groupedFill),
        ) { media() }
        Spacer(Modifier.height(TextGap))
        Text(
            text = title,
            fontSize = titleSize.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
        Text(
            text = subtitle,
            fontSize = subtitleSize.sp,
            color = DesignTokens.textSub,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
    }
}

// 104×156（2:3）与 161×91（16:9）来自任务规格，令牌无对应宽度。
private val PosterWidth = 104.dp
private val PosterHeight = 156.dp
private val WideWidth = 161.dp
private val WideHeight = 91.dp
private const val WideRatio = 16f / 9f
// Wide 副标题 11sp 与文字间距 6dp 无令牌，按基线海报卡取值。
private const val CompactSubtitleSize = 11f
private val TextGap = 6.dp