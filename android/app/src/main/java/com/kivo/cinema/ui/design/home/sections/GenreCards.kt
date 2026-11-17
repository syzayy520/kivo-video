package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbGenre
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

// 竞品帧 315×105px → 177×59dp（3:1）✅ 实测换算
private val GenreCardWidth = 177.dp
private val GenreCardHeight = 59.dp
private val GenreCardRadius = 12.dp
private val GenreCardShape = RoundedCornerShape(GenreCardRadius)
private val GenreSidePad = 13.dp
private val GenreCardGap = 10.dp
private val GenreCardInset = 12.dp

/**
 * 卡容器描边 1dp：`groupedFill (0xFFF2F2F7)` 与 `pageBackground (0xFFF2F2F7)` **同值**，
 * 只填色等于没画容器（这就是用户看到的「卡片几乎不可见」）✅ 实测（DesignPalette.Light 第 18–19 行）。
 */
private val GenreCardBorderWidth = 1.dp

// 左侧层叠缩略图：顶层 64×40dp，底层偏移 (6,4) 露出右下边，形成「一叠卡片」
private val GenreThumbWidth = 64.dp
private val GenreThumbHeight = 40.dp
private val GenreThumbOffsetX = 6.dp
private val GenreThumbOffsetY = 4.dp
private val GenreThumbRadius = 8.dp
private val GenreThumbGap = 8.dp
private const val GenreUnderlayWashAlpha = 0.25f
private const val GenrePlaceholderLines = 3
private const val GenrePlaceholderLastLineFraction = 0.62f

/**
 * 分类浏览卡（竞品帧 `rex-genre-grid.jpg` 的「分类浏览」两列卡区）。
 *
 * 结构 ✅ 实测（1.777 px/dp 换算）：
 * - 卡 **177×59dp**（3:1）、圆角 12、底 `groupedFill` + **1dp `hairline` 描边**；
 * - 左 64×40dp 缩略图圆角 8，**两张叠放**：底层 `offset(6dp, 4dp)` 露出右下边，
 *   并叠 `canvasFill` 25% 洗色与顶层区分；图整体在卡高内**垂直居中**（Row 的
 *   `CenterVertically` 对齐 44dp 高的层叠盒，上下留 7.5dp）；
 * - 右侧文字 14sp 粗、`Alignment.CenterStart`（**不是 BottomStart**，用户二次否决项）。
 *
 * 设计取舍：
 * - 描边是**功能必需**而非装饰：底 `groupedFill` 与页面底同值，不描边则容器边界不可见；
 * - 底层用同一张 `genre.backdropUrl`（模型只有一个底图字段），只靠洗色拉开层次 ◐ 推断；
 * - 无底图时画 3 条横线抽象占位（第 3 条占 62% 宽），**不留纯空白** ✅ 用户可见约束；
 * - 文字最多 1 行：类型名最长 4 字，1 行足够且避免撑开卡高。
 *
 * 空列表：不渲染任何东西。
 */
@Composable
fun GenreCards(genres: List<TmdbGenre>, fg: Color, onOpen: (TmdbGenre) -> Unit) {
    if (genres.isEmpty()) return
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(GenreCardGap),
        contentPadding = PaddingValues(horizontal = GenreSidePad),
    ) {
        items(genres) { genre -> GenreCard(genre, fg, onOpen) }
    }
}

@Composable
private fun GenreCard(genre: TmdbGenre, fg: Color, onOpen: (TmdbGenre) -> Unit) {
    Row(
        Modifier
            .size(GenreCardWidth, GenreCardHeight)
            .clip(GenreCardShape)
            .background(DesignTokens.groupedFill)
            .border(GenreCardBorderWidth, DesignTokens.hairline, GenreCardShape)
            .clickable { onOpen(genre) }
            .padding(horizontal = GenreCardInset),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        GenreThumbStack(genre)
        Spacer(Modifier.width(GenreThumbGap))
        Box(
            Modifier.weight(1f).fillMaxHeight(),
            contentAlignment = Alignment.CenterStart,
        ) {
            Text(
                genre.name,
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold,
                color = fg,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}

@Composable
private fun GenreThumbStack(genre: TmdbGenre) {
    Box(
        Modifier
            .width(GenreThumbWidth + GenreThumbOffsetX)
            .height(GenreThumbHeight + GenreThumbOffsetY),
    ) {
        GenreThumbLayer(
            backdropUrl = genre.backdropUrl,
            description = genre.name,
            modifier = Modifier.offset(GenreThumbOffsetX, GenreThumbOffsetY),
            washAlpha = GenreUnderlayWashAlpha,
        )
        GenreThumbLayer(
            backdropUrl = genre.backdropUrl,
            description = genre.name,
            modifier = Modifier,
            washAlpha = 0f,
        )
    }
}

@Composable
private fun GenreThumbLayer(
    backdropUrl: String?,
    description: String,
    modifier: Modifier,
    washAlpha: Float,
) {
    val url = backdropUrl.orEmpty()
    if (url.isBlank()) {
        GenreThumbPlaceholder(modifier)
        return
    }
    Box(modifier.size(GenreThumbWidth, GenreThumbHeight).clip(RoundedCornerShape(GenreThumbRadius))) {
        TmdbImage(
            url = url,
            contentDescription = description,
            modifier = Modifier.fillMaxSize(),
            bg = DesignTokens.groupedFill,
        )
        if (washAlpha > 0f) {
            Box(Modifier.fillMaxSize().background(DesignTokens.canvasFill.copy(alpha = washAlpha)))
        }
    }
}

@Composable
private fun GenreThumbPlaceholder(modifier: Modifier) {
    Column(
        modifier
            .size(GenreThumbWidth, GenreThumbHeight)
            .clip(RoundedCornerShape(GenreThumbRadius))
            .background(DesignTokens.canvasFill)
            .padding(horizontal = 8.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(4.dp),
    ) {
        repeat(GenrePlaceholderLines) { index ->
            val fraction = if (index == GenrePlaceholderLines - 1) GenrePlaceholderLastLineFraction else 1f
            Box(
                Modifier
                    .fillMaxWidth(fraction)
                    .height(2.dp)
                    .clip(RoundedCornerShape(1.dp))
                    .background(DesignTokens.textSub.copy(alpha = 0.35f)),
            )
        }
    }
}

private fun previewGenre(id: Int, name: String, backdropUrl: String? = null) = TmdbGenre(
    id = id,
    name = name,
    backdropUrl = backdropUrl,
)

@Preview(name = "GenreCards 空 / 2 条", widthDp = 390, heightDp = 200, showBackground = true)
@Composable
private fun GenreCardsPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        GenreCards(emptyList(), DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        GenreCards(
            genres = listOf(previewGenre(1, "动作"), previewGenre(2, "悬疑")),
            fg = DesignTokens.textMain,
            onOpen = {},
        )
    }
}