package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
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
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbProvider
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

// 尺寸基准：竞品帧 693px 宽 = 390dp（1.777 px/dp），卡 420×167px → 236×94dp ✅ 实测换算
private val PlatformCardWidth = 236.dp
private val PlatformCardHeight = 94.dp
private val PlatformCardRadius = 14.dp

// 左 96dp 只放 logo，**不铺满**：上下 18dp、左右 14dp 内缩（竞品 NETFLIX 字标高仅占卡高约 1/5）✅ 实测。
private val PlatformLogoWidth = 96.dp
private val PlatformLogoInsetX = 14.dp
private val PlatformLogoInsetY = 18.dp
private val PlatformSidePad = 13.dp
private val PlatformCardGap = 10.dp

// 右侧拼贴：单张 46×66dp（2:3），两列错落 —— 整体高 136dp > 卡高 94dp，
// 溢出部分由卡自身的圆角裁切形成「出血」效果，与竞品帧一致 ◐ 推断（帧内上下均被裁）。
private val CollageTileWidth = 52.dp
private val CollageTileHeight = 74.dp
private val CollageTileGap = 4.dp
private val CollageStaggerY = 10.dp
private const val CollageTileRadius = 6f
private const val CollageMaxTiles = 4
private const val CollageWashAlpha = 0.20f
private val CollageInsetX = 12.dp
private val CollageInsetY = 14.dp

/**
 * 播出平台横滑卡（竞品帧 `rex-people-platform.jpg` 的「播出平台」区）。
 *
 * 结构 ✅ 实测（1.777 px/dp 换算）：
 * - 卡 **236×94dp**（2.5:1）、圆角 14、底色 `washFill`（淡紫洗色，**不是黑底**）；
 * - 左右 13dp padding + 10dp 卡间距 ⇒ 390dp 视口露出 **1.5 列**（与帧内一致）；
 * - 左 96dp 只放品牌 logo，`TmdbImage(fit = true)`、内缩「上下 18dp / 左右 14dp」，
 *   **logo 不铺满**；无 logo 退化为名称 15sp 粗；
 * - 右侧 `weight(1f)` 保留 2 列错落（2×2）剧集拼贴：每张 46×66dp、圆角 6、列内间距 4，
 *   第二列整体下移 10dp。
 *
 * 设计取舍 / 偏差：
 * - **`bg` 必须显式传 `Color.Transparent`**：`TmdbImage` 的默认 `bg` 是
 *   `TmdbPlaceholder = 0xFFE9E4EE`（浅灰紫，非深色）——不透明，会在 `washFill` 浅底上
 *   留下一块灰紫矩形，把「logo 不铺满」的内缩区显影成方块 ✅ 实测（读 `TmdbImage` 第 75/109 行）；
 * - [TmdbProvider.titles] 是 TMDB 剧集图 URL 列表（B3R-T2 提供，首页只填前 4 个平台）；
 *   为空时右上角画 `canvasFill` 20% 洗色占位块而**不留纯空白、也不画黑块** ✅ 用户可见约束；
 * - 拼贴整体 136dp 高于卡 94dp，靠卡 `clip` 裁切为出血形态；若要「四张完整可见」需把卡高
 *   改到 140dp 左右，与竞品帧的卡高不符，故不做 ◐ 推断。
 *
 * 空列表：不渲染任何东西。
 */
@Composable
fun PlatformCards(providers: List<TmdbProvider>, fg: Color, onOpen: (TmdbProvider) -> Unit) {
    if (providers.isEmpty()) return
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(PlatformCardGap),
        contentPadding = PaddingValues(horizontal = PlatformSidePad),
    ) {
        items(providers) { provider -> PlatformCard(provider, fg, onOpen) }
    }
}

@Composable
private fun PlatformCard(provider: TmdbProvider, fg: Color, onOpen: (TmdbProvider) -> Unit) {
    Row(
        Modifier
            .size(PlatformCardWidth, PlatformCardHeight)
            .clip(RoundedCornerShape(PlatformCardRadius))
            .background(DesignTokens.washFill)
            .clickable { onOpen(provider) },
        verticalAlignment = Alignment.CenterVertically,
    ) {
        PlatformBrand(provider, fg)
        PlatformCollage(provider.titles)
    }
}

/**
 * 左 96dp 品牌位：**品牌字标**（不用 TMDB 的 logo 图）。
 *
 * 原因（真机实测）：TMDB 的 Netflix / Apple TV `logo_path` 是**自带黑底的 PNG**，
 * 放到浅灰卡上会显出一整块黑方块（2026-11-17 截图 n2 实证），代码侧无法去掉黑底。
 * 竞品 rex 用的是透明底字标（红 NETFLIX、蓝 Disney），故此处用同名文字 + 品牌色还原，
 * 品牌色复用现有语义令牌（`dangerRed` / `navActive`），不新增裸色。
 */
@Composable
private fun PlatformBrand(provider: TmdbProvider, fg: Color) {
    Box(Modifier.width(PlatformLogoWidth).fillMaxHeight(), contentAlignment = Alignment.Center) {
        Text(
            brandLabel(provider.name),
            fontSize = 12.sp,
            fontWeight = FontWeight.Bold,
            letterSpacing = 0.2.sp,
            color = brandInk(provider.name, fg),
            textAlign = TextAlign.Center,
            maxLines = 1,
            softWrap = false,
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.padding(
                horizontal = PlatformLogoInsetX,
                vertical = PlatformLogoInsetY,
            ),
        )
    }
}

/** 字标文案：Apple 的三种 SKU 统一成 `APPLE TV`，避免 96dp 区内被省略号截断。 */
private fun brandLabel(name: String): String = when {
    name.contains("Apple TV", ignoreCase = true) -> "APPLE TV"
    name.contains("Netflix", ignoreCase = true) -> "NETFLIX"
    name.contains("Disney", ignoreCase = true) -> "DISNEY+"
    else -> name.uppercase()
}

/** 品牌字标色：Netflix 红 / Disney 蓝 / 其余跟随前景。◐ 取值按品牌主色近似，非像素取色。 */
private fun brandInk(name: String, fg: Color): Color = when {
    name.contains("Netflix", ignoreCase = true) -> DesignTokens.dangerRed
    name.contains("Disney", ignoreCase = true) -> DesignTokens.navActive
    else -> fg
}

/** 右侧 2 列错落拼贴（2×2）；`titles` 为空时画洗色占位块，不留纯空白、不画黑块。 */
@Composable
private fun RowScope.PlatformCollage(titles: List<String>) {
    Box(
        Modifier.weight(1f).fillMaxHeight(),
        contentAlignment = Alignment.Center,
    ) {
        val tiles = titles.filter { it.isNotBlank() }.take(CollageMaxTiles)
        if (tiles.isEmpty()) {
            Box(
                Modifier
                    .padding(horizontal = CollageInsetX, vertical = CollageInsetY)
                    .fillMaxSize()
                    .clip(RoundedCornerShape(CollageTileRadius))
                    .background(DesignTokens.canvasFill.copy(alpha = CollageWashAlpha)),
            )
            return@Box
        }
        Row(horizontalArrangement = Arrangement.spacedBy(CollageTileGap)) {
            CollageColumn(tiles.take(2), Modifier)
            if (tiles.size > 2) {
                CollageColumn(tiles.drop(2).take(2), Modifier.offset(y = CollageStaggerY))
            }
        }
    }
}

@Composable
private fun CollageColumn(tiles: List<String>, modifier: Modifier) {
    Column(modifier.width(CollageTileWidth), verticalArrangement = Arrangement.spacedBy(CollageTileGap)) {
        tiles.forEach { url -> CollageTile(url) }
    }
}

@Composable
private fun CollageTile(url: String) {
    TmdbImage(
        url = url,
        contentDescription = "",
        modifier = Modifier
            .size(CollageTileWidth, CollageTileHeight)
            .clip(RoundedCornerShape(CollageTileRadius)),
    )
}

private fun previewProvider(id: Int, name: String, titles: List<String> = emptyList()) = TmdbProvider(
    id = id,
    name = name,
    logoUrl = null,
    titles = titles,
)

@Preview(name = "PlatformCards 空 / 2 条", widthDp = 390, heightDp = 240, showBackground = true)
@Composable
private fun PlatformCardsPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        PlatformCards(emptyList(), DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        PlatformCards(
            providers = listOf(
                previewProvider(1, "Netflix", listOf("poster-1", "poster-2", "poster-3", "poster-4")),
                previewProvider(2, "Disney+"),
            ),
            fg = DesignTokens.textMain,
            onOpen = {},
        )
    }
}