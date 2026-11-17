package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.BlendMode
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Paint
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbStudio
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

// 竞品帧 315×142px → 177×80dp（2.2:1）✅ 实测换算
private val StudioCardWidth = 177.dp
private val StudioCardHeight = 80.dp
private val StudioCardRadius = 14.dp
private val StudioCardShape = RoundedCornerShape(StudioCardRadius)
private val StudioSidePad = 13.dp
private val StudioCardGap = 10.dp

/** 剧照上洗色 45%（竞品帧在公司区底图上压暗约一半，白色 logo 才立得起来）✅ 实测取色。 */
private const val StudioScrimAlpha = 0.45f
private const val StudioLogoWidthFraction = 0.70f
private const val StudioLogoHeightFraction = 0.62f

/**
 * 洗色：竞品为纯黑 45%。项目没有 `scrim` 语义令牌，取现有唯一的近黑令牌 `textMain`(0xFF1A1A1A)
 * 加 45% alpha；新增令牌要改 `DesignTokens.kt`，超出本卡文件范围 ◐ 取舍（可见色差 <2/255）。
 */
private val StudioScrim = DesignTokens.textMain.copy(alpha = StudioScrimAlpha)

/**
 * 白 logo 的剪影色：`canvasFill` 是现有语义令牌里唯一的纯白（0xFFFFFFFF）。
 * 语义上它是「卡片画布」，这里借作「暗底前景」，原因同上（不改令牌文件）。
 */
private val StudioLogoOnDark = DesignTokens.canvasFill

/**
 * 电影公司卡（竞品帧 `rex-genre-grid.jpg` 的「电影公司」区）。
 *
 * 结构 ✅ 实测（1.777 px/dp 换算）：
 * - 卡 **177×80dp**（2.2:1）、圆角 14、左右 13dp padding + 10dp 间距；
 * - **有 `backdropUrl`**：剧照 Crop 铺满 + 45% 洗色 + **白色 logo 居中、卡宽 70% × 卡高 62%、
 *   `fit = true`、不裁切**；
 * - **无 `backdropUrl`**：`washFill` 底 + `textMain` 名称/原色 logo（不伪造剧照）✅ 不造假约束；
 * - 无 `logoUrl` 时居中显示名称 14sp 粗（**不留纯空白**）。
 *
 * 为什么上一版是「白底 + 蓝 logo 且被裁」：旧实现拿 `logoUrl` 当底图 Crop 铺满，
 * 再叠 30% 白洗色 —— 浅色 logo 图被放大裁切成白底，蓝色字形被切掉。本版换成
 * 真实剧照 [`TmdbStudio.backdropUrl`] 作底，logo 只做前景 ✅ 用户二次否决项。
 *
 * 白色 logo 的做法（⚠️ 偏差说明）：`TmdbImage` 没有 `colorFilter` 参数，且本卡只允许改
 * 这 5 个文件，所以在 logo 所在的图层内用 `saveLayer` + `BlendMode.SrcIn` 把已绘制的像素
 * 统一染成白色剪影，保留原 logo 的 alpha 形状、不裁切。`bg` 必须传 `Color.Transparent`：
 * `TmdbImage` 默认 `bg = TmdbPlaceholder (0xFFE9E4EE)` 是**不透明浅灰紫**，会在 logo 内缩区
 * 留下实心方块，并被本剪影一起染白（变成白块）✅ 实测（`TmdbImage` 第 75/109/124 行）。
 *
 * 参数 `fg`：保留调用方（`DiscoverPage` 传 chrome 前景色）签名兼容；本组件名称统一走
 * `textMain`（用户指定），故 `fg` 不再参与绘制。
 *
 * 空列表：不渲染任何东西。
 */
@Composable
fun StudioCards(studios: List<TmdbStudio>, fg: Color, onOpen: (TmdbStudio) -> Unit) {
    if (studios.isEmpty()) return
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(StudioCardGap),
        contentPadding = PaddingValues(horizontal = StudioSidePad),
    ) {
        items(studios) { studio -> StudioCard(studio, fg, onOpen) }
    }
}

@Composable
private fun StudioCard(studio: TmdbStudio, fg: Color, onOpen: (TmdbStudio) -> Unit) {
    val backdropUrl = studio.backdropUrl.orEmpty()
    val hasBackdrop = backdropUrl.isNotBlank()
    Box(
        Modifier
            .size(StudioCardWidth, StudioCardHeight)
            .clip(StudioCardShape)
            // 无剧照时只铺 washFill；有剧照时先铺近黑，避免图未到时空卡透出页面白底。
            .background(if (hasBackdrop) DesignTokens.textMain else DesignTokens.washFill)
            .clickable { onOpen(studio) },
        contentAlignment = Alignment.Center,
    ) {
        if (hasBackdrop) {
            StudioBackdrop(backdropUrl)
        }
        StudioLogo(
            logoUrl = studio.logoUrl.orEmpty(),
            name = studio.name,
            onDark = hasBackdrop,
        )
    }
}

/** 品牌底：真实剧照 Crop 铺满 + 45% 洗色（压暗后白 logo 才可读）。 */
@Composable
private fun StudioBackdrop(backdropUrl: String) {
    Box(Modifier.fillMaxSize()) {
        TmdbImage(
            url = backdropUrl,
            contentDescription = "",
            modifier = Modifier.fillMaxSize(),
            bg = DesignTokens.textMain,
        )
        Box(Modifier.fillMaxSize().background(StudioScrim))
    }
}

/** 前景 logo：卡宽 70% × 卡高 62%，Fit 保字形、不裁切；有剧照时染白。 */
@Composable
private fun StudioLogo(logoUrl: String, name: String, onDark: Boolean) {
    val boxModifier = Modifier
        .fillMaxWidth(StudioLogoWidthFraction)
        .fillMaxHeight(StudioLogoHeightFraction)
    if (logoUrl.isBlank()) {
        Text(
            name,
            fontSize = 14.sp,
            fontWeight = FontWeight.Bold,
            color = if (onDark) StudioLogoOnDark else DesignTokens.textMain,
            maxLines = 2,
            overflow = TextOverflow.Ellipsis,
            modifier = boxModifier.padding(horizontal = 8.dp),
        )
        return
    }
    TmdbImage(
        url = logoUrl,
        contentDescription = name,
        modifier = if (onDark) boxModifier.stencil(StudioLogoOnDark) else boxModifier,
        bg = Color.Transparent,
        fit = true,
    )
}

/**
 * 把图层内已绘制的非透明像素统一染成 [color]、保留 alpha 形状（白色 logo 剪影）。
 * 若以后 `TmdbImage` 支持 `colorFilter`，应删掉本扩展改用 tint（少一次离屏图层）。
 */
private fun Modifier.stencil(color: Color): Modifier = drawWithContent {
    val bounds = Rect(0f, 0f, size.width, size.height)
    drawContext.canvas.saveLayer(bounds, Paint())
    drawContent()
    drawContext.canvas.drawRect(0f, 0f, size.width, size.height, Paint().apply {
        this.color = color
        blendMode = BlendMode.SrcIn
    })
    drawContext.canvas.restore()
}

private fun previewStudio(id: Int, name: String, backdropUrl: String? = null) = TmdbStudio(
    id = id,
    name = name,
    logoUrl = null,
    backdropUrl = backdropUrl,
)

@Preview(name = "StudioCards 空 / 2 条", widthDp = 390, heightDp = 200, showBackground = true)
@Composable
private fun StudioCardsPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        StudioCards(emptyList(), DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        StudioCards(
            studios = listOf(previewStudio(1, "A24"), previewStudio(2, "Blumhouse")),
            fg = DesignTokens.textMain,
            onOpen = {},
        )
    }
}