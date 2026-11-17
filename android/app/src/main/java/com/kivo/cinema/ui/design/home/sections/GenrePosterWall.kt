package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/** 冻结契约：固定宽 64.7dp（`rex-dna-browse.md` §1.1：卡宽 64.7 / gap 15.7），2:3 ⇒ 高 97.05dp。 */
private val GenreWallPosterWidth = 64.7.dp
private val GenreWallPosterAspect = 2f / 3f
private const val GenreWallPlaceholderAlpha = 0.08f

// 与 `TopRatedRow` 完全同步：左 13dp、列间距 15.7dp、末项右侧 13dp（390pt 基准下第 5 张溢出屏缘 9.3dp）。
private val GenreWallSidePad = 13.dp
/** 右侧额外留白：与 TopRatedRow 同口径，保证 K40（411dp）上有 peek。 */
private val GenreWallEndPad = 26.dp
private val GenreWallGap = 15.7.dp

/**
 * 分类浏览海报横滑行：2:3 海报，纯 URL 入参（调用方给 w342 静态片单）。
 *
 * 冻结契约（B-3b 裁决：按 REX 横滑）：
 * - `LazyRow` + `Arrangement.spacedBy(15.7dp)`，`contentPadding` 左右各 13dp；
 * - 单张**固定宽 64.7dp**、`aspectRatio(2f/3f)` ⇒ 高 97.05dp、圆角 `DesignTokens.radiusPoster`；
 * - 一屏（390pt 基准）完整可见 4 张，第 5 张露出 55.4dp（占卡宽 85.6%），横向可滑 22.3dp；
 * - 与 `TopRatedRow` **逐项同口径**（用户二次否决项：两块必须尺寸同步）。
 *
 * 为什么改 `LazyRow`：旧 `chunked(5) + Row + weight(1f)` 是等分自适应，390pt 下只有 60.2dp，
 * 与 REX 实测 64.7 差 4.5dp 且随屏宽漂移；固定宽横滑既有 peek 又能在任意屏宽上保持同一尺寸。
 * `LazyRow` 横向嵌套在首页 `LazyColumn` 内是安全的（无界高度问题只属于纵向 `Lazy*`）。
 *
 * 为什么不吃 `TmdbHome.Item` ◐ 推断：`TmdbGenres.posters()` 只返回图片路径，没有 id/title，
 * 硬造 `TmdbHome.Item` 会伪造元数据（违反「不许静默降级」），故用 `List<String>`。
 * URL 可能重复（同一部片出现在多个分类），因此**不用 url 作 key**，用下标保证不撞键。
 *
 * 交互：每格 `clickable` 把 url 回传给 `onOpen`。**当前调用方传 `onOpen = {}`**（KDoc 明示）：
 * 页面层还没有「按 url 打开详情」的回调，接入详情后由 `DiscoverPage` 换掉空实现；本组件不自造详情 API。
 *
 * 参数 `fg`：仅用于加载中/失败时的占位底色（`fg` 8% 叠色）。
 * `contentDescription` 传空串（装饰图语义）—— url 里没有可朗读的标题。
 * 空列表：不渲染任何东西（`LazyRow` 也不创建）。
 */
@Composable
fun GenrePosterWall(urls: List<String>, fg: Color, onOpen: (String) -> Unit) {
    if (urls.isEmpty()) return
    LazyRow(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(GenreWallGap),
        contentPadding = PaddingValues(start = GenreWallSidePad, end = GenreWallEndPad),
    ) {
        itemsIndexed(urls) { _, url -> GenreWallCell(url, fg, onOpen) }
    }
}

@Composable
private fun GenreWallCell(
    url: String,
    fg: Color,
    onOpen: (String) -> Unit,
) {
    TmdbImage(
        url = url,
        contentDescription = "",
        modifier = Modifier
            .width(GenreWallPosterWidth)
            .aspectRatio(GenreWallPosterAspect)
            .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp))
            .clickable { onOpen(url) },
        bg = fg.copy(alpha = GenreWallPlaceholderAlpha),
        fit = false,
    )
}

@Preview(name = "GenrePosterWall 空 / 7 张（空 url 显示占位）", widthDp = 390, heightDp = 260, showBackground = true)
@Composable
private fun GenrePosterWallPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        GenrePosterWall(emptyList(), DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        GenrePosterWall(List(7) { "" }, DesignTokens.textMain) {}
    }
}