package com.kivo.cinema.ui.profile.lists

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.WatchEntry

/**
 * 「收藏」计数卡（R2 复刻，基准 `docs/audit/golden/kivo/profile-1.png` 双卡区左卡）。
 *
 * 版式（✅ 实测）：半宽白卡 178dp × 86dp（490×237px）、圆角 16dp、内 14dp；
 * 大数字「{n}」30sp Bold [DesignTokens.textMain]（实测 #1A1A1A）、下方 4dp、标签「收藏」15sp。
 *
 * 宽度偏差（有意）：冻结签名不带 `Modifier` 形参，本卡按 `fillMaxWidth()` 自适应可用宽度；
 * 基准的半宽由编排方放进 `Row` 的 `weight(1f)` 容器决定（13 + 178 + 10 + 178 + 13 = 392dp）。
 * 直接放进 `Column` 会变成满宽卡 —— 这是编排约束，不是本组件的偏差。
 *
 * 颜色偏差（有意）：任务书写标签用 `textSub`，基准实测为主色 #1A1A1A（`textMain`），按 1:1 取 `textMain`。
 *
 * 交互：`onOpen` 在收藏为空时不可触发（无条目可打开）；非空时整卡点击打开第一条。
 * 未覆盖：收藏计数点击后的列表页（冻结签名只给 onOpen(WatchEntry)，不给「全部收藏」入口）。
 */
@Composable
fun CollectionSection(entries: List<WatchEntry>, onOpen: (WatchEntry) -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(CardShape)
            .background(DesignTokens.canvasFill)
            .border(HairlineWidth, DesignTokens.hairline, CardShape)
            .then(if (entries.isEmpty()) Modifier else Modifier.clickable(role = Role.Button) { onOpen(entries.first()) })
            .padding(CardPad),
    ) {
        Text(
            text = entries.size.toString(),
            fontSize = CountSize.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
        Spacer(Modifier.height(CountGap))
        Text(
            text = Label,
            fontSize = LabelSize.sp,
            color = DesignTokens.textMain,
        )
    }
}

private const val Label = "收藏"
private const val CountSize = 30f
private const val LabelSize = 15f

private val CardPad = 14.dp
private val CountGap = 4.dp
private val CardShape = RoundedCornerShape(16.dp)
private val HairlineWidth = 1.dp
// 预览按基准半宽 178dp 作画，外加两侧 13dp 页边距。
private val PreviewPad = 13.dp
private val PreviewCardWidth = 178.dp + PreviewPad * 2

@Preview(name = "收藏卡 · 空", widthDp = 180, showBackground = true)
@Composable
private fun PreviewCollectionEmpty() {
    Box(Modifier.width(PreviewCardWidth).background(DesignTokens.pageBackground).padding(PreviewPad)) {
        CollectionSection(entries = emptyList(), onOpen = {})
    }
}

@Preview(name = "收藏卡 · 5 部", widthDp = 180, showBackground = true)
@Composable
private fun PreviewCollectionFilled() {
    Box(Modifier.width(PreviewCardWidth).background(DesignTokens.pageBackground).padding(PreviewPad)) {
        CollectionSection(
            entries = List(5) { WatchEntry("收藏 $it", "2024 · 电影", null, null) },
            onOpen = {},
        )
    }
}