package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 设置单行（B-3c 行式化）：REX `0374` 的「分组内一行」，整行可点，行间分割线由 [SettingsGroupList] 画。
 *
 * 冻结契约（改签名前先看 `docs/design/rex-dna-browse.md` §1.3 / `kivo-play-baseline.md` §6 C6）：
 * ```
 * fun SettingsRowItem(title, subtitle = null, trailing = null, showChevron = true, onClick)
 * ```
 *
 * | 维度 | REX 实测（pt） | 本实现（dp/sp） | 说明 |
 * | --- | --- | --- | --- |
 * | 行高 | 44–46（文字块 22 + 上下 11） | 46 min | `heightIn(min)`，见下方偏离 ① |
 * | 左右内边距 | 14.7（页边距） | 14 | 与页边距同值，文字距屏缘 28 |
 * | 标题 | 13 Medium（卡标题） | 15 / textMain | 行标题取设置行字号，非卡标题 |
 * | 副标题 | 11（元信息） | 12 / textSub | 灰色次级 |
 * | 右值 | 13 Regular | 13 / textSub | 当前值（计数/开关态/缓存量） |
 * | chevron | 17 Bold「›」 | 12dp 描边折线 / textSub | 见偏离 ② |
 *
 * 偏离：
 * ① 不用 `height(46.dp)`：系统字体放大时「标题 + 副标题」会溢出被裁，46 记为下限；
 *    单行/两行内容在 46 内垂直居中，实测排版高度 19 + 15 = 34，上下各 6。
 * ② chevron 按任务口径做 12dp 几何（描边 1.4dp 圆头），不是 17sp 字形 —— 12dp 折线与 REX
 *    「细线图标」气质一致，且不随字体缩放变形。
 * ③ 标题/副标题 `maxLines = 1`：允许省略号，避免长文案把行撑破 46（REX 同样单行）。
 *
 * 无 chevron 时右侧留 20dp（[SettingsRowSidePadNoChevron]），保证右值不贴卡片圆角。
 */
@Composable
fun SettingsRowItem(
    title: String,
    subtitle: String? = null,
    trailing: String? = null,
    showChevron: Boolean = true,
    onClick: () -> Unit,
) {
    Row(
        Modifier
            .fillMaxWidth()
            .heightIn(min = SettingsRowMinHeight)
            .clickable(role = Role.Button, onClick = onClick)
            .padding(horizontal = SettingsRowSidePad),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(Modifier.weight(1f)) {
            Text(
                title, fontSize = SettingsRowTitleSize.sp, color = DesignTokens.textMain,
                lineHeight = SettingsRowTitleLine.sp, maxLines = 1, overflow = TextOverflow.Ellipsis,
            )
            if (subtitle != null) {
                Text(
                    subtitle, fontSize = SettingsRowSubSize.sp, color = DesignTokens.textSub,
                    lineHeight = SettingsRowSubLine.sp, maxLines = 1, overflow = TextOverflow.Ellipsis,
                )
            }
        }
        if (trailing != null) {
            Spacer(Modifier.width(SettingsRowValueGap))
            Text(
                trailing, fontSize = SettingsRowValueSize.sp, color = DesignTokens.textSub,
                maxLines = 1, overflow = TextOverflow.Ellipsis,
            )
        }
        if (showChevron) {
            Spacer(Modifier.width(SettingsRowValueGap))
            RowChevron()
        } else {
            Spacer(Modifier.width(SettingsRowSidePadNoChevron))
        }
    }
}

/** 右「›」：12dp 折线，描边圆头，色 `textSub`（不用字形，避免字体缩放改变几何）。 */
@Composable
private fun RowChevron() {
    Canvas(Modifier.size(SettingsRowChevronSize)) {
        val w = size.width
        val h = size.height
        val stroke = SettingsRowChevronStroke.toPx()
        drawLine(
            DesignTokens.textSub,
            Offset(w * 0.28f, h * 0.20f),
            Offset(w * 0.72f, h * 0.50f),
            strokeWidth = stroke, cap = StrokeCap.Round,
        )
        drawLine(
            DesignTokens.textSub,
            Offset(w * 0.72f, h * 0.50f),
            Offset(w * 0.28f, h * 0.80f),
            strokeWidth = stroke, cap = StrokeCap.Round,
        )
    }
}

private val SettingsRowMinHeight = 46.dp
private val SettingsRowSidePad = 14.dp
private val SettingsRowSidePadNoChevron = 20.dp
private val SettingsRowChevronSize = 12.dp
private val SettingsRowValueGap = 6.dp
private val SettingsRowChevronStroke = 1.4.dp
private const val SettingsRowTitleSize = 15f
private const val SettingsRowTitleLine = 19f
private const val SettingsRowSubSize = 12f
private const val SettingsRowSubLine = 15f
private const val SettingsRowValueSize = 13f

@Preview(name = "SettingsRowItem 单行 / 带副标题 / 带右值 / 无 chevron", widthDp = 360, showBackground = true)
@Composable
private fun SettingsRowItemPreview() {
    Column(Modifier.fillMaxWidth().background(DesignTokens.canvasFill)) {
        SettingsRowItem("播放", "缓冲 · 习惯 · 字幕") {}
        SettingsRowItem("Emby", trailing = "2 个已配置") {}
        SettingsRowItem("首页预告片自动播放", "默认显示图片", trailing = "开", showChevron = false) {}
    }
}