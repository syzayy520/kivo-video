package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 播放页通用面板（player-ui-v2.md §3.4，边界规则 P3）。
 *
 * **只有一个实现**：倍速 / 音轨 / 画质 / 字幕样式都靠 [PlayerOptionItem] 列表驱动。
 * 之前每加一个选项就要写一个面板文件 —— 那正是"为了凑文件而凑"的反面：
 * **四个长得一样的东西，应该是同一个东西的四份数据。**
 *
 * 规格与参考一致：13pt 圆角 / 59pt 行高 / 深色毛玻璃 / 选中态白勾。
 */
@Composable
internal fun PlayerOptionPanel(
    title: String,
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Box(
        modifier = modifier
            .width(PlayerTokens.panelWidth.dp)
            // 【为什么限高】面板贴在右侧、位于底栏**上方**，上下要让开顶栏与底栏。
            // 选项多的时候（如 7 档倍速）内容会超出屏幕 ⇒ 必须能滚动，
            // 否则下面两档永远点不到（实测：1.5x/1.75x/2x 被屏幕边缘切掉）。
            .heightIn(max = PlayerTokens.panelMaxHeight.dp)
            .playerGlassSurface()
            .clickable(onClick = onDismiss),
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .verticalScroll(rememberScrollState()),
        ) {
            // ---- 面板标题 ----
            Text(
                text = title,
                color = PlayerTokens.textPrimary,
                fontSize = 15.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier.padding(
                    start = 18.dp, end = 18.dp, top = 12.dp, bottom = 6.dp,
                ),
            )

            // ---- 行 ----
            items.forEach { item ->
                PlayerOptionRow(item = item, onSelect = { onSelect(item) })
            }
        }
    }
}

/** 面板的一行。 */
@Composable
private fun PlayerOptionRow(
    item: PlayerOptionItem,
    onSelect: () -> Unit,
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .height(PlayerTokens.panelRowHeight.dp)
            .clickable(onClick = onSelect)
            .padding(horizontal = 18.dp),
        verticalArrangement = Arrangement.Center,
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(
                text = item.title,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.panelTitleSize.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier.weight(1f, fill = false),
            )
            if (item.value.isNotBlank()) {
                Text(
                    text = item.value,
                    color = PlayerTokens.textSecondary,
                    fontSize = PlayerTokens.panelSubtitleSize.sp,
                    maxLines = 1,
                    modifier = Modifier.padding(start = 8.dp),
                )
            }
            if (item.selected) {
                // 选中态：白勾（与设置页同语言）
                Box(
                    modifier = Modifier
                        .padding(start = 8.dp)
                        .size(14.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    PlayerGlyph(
                        kind = "check",
                        color = PlayerTokens.textPrimary,
                        modifier = Modifier.size(14.dp),
                        strokeWidth = 2.2f,
                    )
                }
            }
        }
        if (item.subtitle.isNotBlank()) {
            Text(
                text = item.subtitle,
                color = PlayerTokens.textSecondary,
                fontSize = PlayerTokens.panelSubtitleSize.sp,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier.padding(top = 2.dp),
            )
        }
    }
}

/**
 * 面板的一行数据。
 *
 * @param title 行标题。
 * @param value 右侧的值（可空）。
 * @param subtitle 标题下的补充说明（可空）。
 * @param selected 是否选中。
 * @param tag 回调时带回去的标识（用来区分是哪一项）。
 */
data class PlayerOptionItem(
    val title: String,
    val value: String = "",
    val subtitle: String = "",
    val selected: Boolean = false,
    val tag: String = "",
)
