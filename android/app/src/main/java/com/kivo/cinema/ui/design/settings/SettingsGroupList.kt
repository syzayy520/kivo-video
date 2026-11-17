package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 设置分组白卡（B-3c）：标题小字 + 一张 [DesignTokens.canvasFill] 卡，行按序排列，行间 0.5dp 分割线。
 *
 * 冻结契约：
 * ```
 * fun SettingsGroupList(title: String?, rows: List<SettingsRowSpec>)
 * ```
 *
 * | 维度 | REX 实测（pt） | 本实现（dp/sp） |
 * | --- | --- | --- |
 * | 白卡 | 纯白 + 极淡分隔 | `canvasFill` + `hairline` 0.5dp 描边 |
 * | 圆角 | 卡 16 | 16 |
 * | 组标题 | 13 灰、左缩 14 | 13sp `textSub`、左 14dp |
 * | 组标题 → 卡顶 | 14.6 | 6（标题自带行高，见偏离 ①） |
 * | 行高 / 行内距 | 44–46 / 左右 14 | 由 [SettingsRowItem] 负责 |
 * | 分割线 | 行间 1px 黑 5% | 0.5dp `hairline`，最后一行不画 |
 *
 * 偏离：
 * ① 组标题下 6dp 而非 14.6dp：14.6 是「节标题底 → 卡顶」（标题块 17 Bold 的节），
 *    设置页组标题是 13sp 小字，行高自带约 8dp 下留白，6 + 行高留白 ≈ 14.6 的视觉等效。
 * ② 分割线全宽（不内缩）：本页行无 leading 图标，文字左缩 14 已在行内，全宽线与卡片边缘对齐。
 * ③ 分割线由本文件画、不由行画 —— 行只管「一行长什么样」，组合才知道「哪一行是最后一行」。
 *
 * `title = null` 时不出组标题（无头分组）。
 */
@Composable
fun SettingsGroupList(title: String?, rows: List<SettingsRowSpec>) {
    Column(Modifier.fillMaxWidth()) {
        if (title != null) {
            Text(
                title, fontSize = GroupTitleSize.sp, color = DesignTokens.textSub,
                modifier = Modifier.padding(start = GroupTitleStartPad, bottom = GroupTitleBottomGap),
            )
        }
        val shape = RoundedCornerShape(GroupCardRadius)
        Column(
            Modifier
                .fillMaxWidth()
                .clip(shape)
                .background(DesignTokens.canvasFill, shape)
                .border(GroupCardStroke, DesignTokens.hairline, shape),
        ) {
            rows.forEachIndexed { index, row ->
                SettingsRowItem(
                    title = row.title,
                    subtitle = row.subtitle,
                    trailing = row.trailing,
                    showChevron = row.showChevron,
                    onClick = row.onClick,
                )
                if (index != rows.lastIndex) {
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .height(GroupDividerHeight)
                            .background(DesignTokens.hairline),
                    )
                }
            }
        }
    }
}

/** 一行设置的数据快照：无状态，点击行为由 [onClick] 携带（路由/开关切换都在调用方）。 */
data class SettingsRowSpec(
    val title: String,
    val subtitle: String? = null,
    val trailing: String? = null,
    val showChevron: Boolean = true,
    val onClick: () -> Unit,
)

private val GroupCardRadius = 16.dp
private val GroupCardStroke = 0.5.dp
private val GroupDividerHeight = 0.5.dp
private val GroupTitleStartPad = 14.dp
private val GroupTitleBottomGap = 6.dp
private const val GroupTitleSize = 13f

@Preview(name = "SettingsGroupList 有标题 / 无标题", widthDp = 360, showBackground = true)
@Composable
private fun SettingsGroupListPreview() {
    Column(Modifier.padding(14.dp).background(DesignTokens.pageBackground)) {
        SettingsGroupList(
            "播放体验",
            listOf(
                SettingsRowSpec("播放", "缓冲 · 习惯 · 字幕") {},
                SettingsRowSpec("音画", "内核 · 画面 · 音频") {},
                SettingsRowSpec("操控", "按钮 · 手势 · 双击") {},
            ),
        )
        Box(Modifier.height(26.dp))
        SettingsGroupList(
            null,
            listOf(
                SettingsRowSpec("Emby", trailing = "2 个已配置") {},
                SettingsRowSpec("SMB", trailing = "未配置") {},
            ),
        )
    }
}