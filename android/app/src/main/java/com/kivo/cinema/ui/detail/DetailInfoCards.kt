package com.kivo.cinema.ui.detail

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 详情页「文件信息」卡（V1）——补足「加入私密区」之下的半屏空白。
 *
 * ## 只画真实数据
 * 每一行都由入参是否有效决定去留：分辨率要宽高均 >0，时长要 >0，格式要能从路径取到扩展名，
 * 路径要非空。四者全缺时整卡不渲染（不画空壳、不画占位，遵守「不许静默降级」的反面：
 * 宁可不画，也不写「未知」当数据）。同一个字段的判定口径与 `TechBadges` 的分辨率徽章一致。
 *
 * ## 视觉参数
 * 白卡 canvasFill · 0.5dp hairline 描边 · 圆角 16 · 内边距 14dp；左右 13dp 屏边由调用方的
 * `screenPad` 提供（详情页正文列已是 13dp，卡内不再重复）。标题 16sp Bold，行 12sp，
 * 路径单行省略 12sp textSub。实测卡高（含 13dp 外距之外的卡体）≈148dp：
 * 28（上下内距）+ 22（标题）+ 3×(8+16)（三行）+ 10+16（路径）= 148。
 *
 * ## 未覆盖
 * 深色态：与详情页其它卡一致，取 DesignTokens 浅色语义令牌。
 *
 * @param width 视频宽（像素）；≤0 或缺 height 时不画分辨率行。
 * @param height 视频高（像素）。
 * @param durationMs 时长（毫秒）；≤0 不画时长行。
 * @param path 完整路径；同时用于取扩展名与路径行。
 */
@Composable
fun DetailFileInfoCard(width: Int, height: Int, durationMs: Long, path: String) {
    val rows = fileInfoRows(width, height, durationMs, path)
    val hasPath = path.isNotBlank()
    if (rows.isEmpty() && !hasPath) return
    val shape = RoundedCornerShape(CardCorner)
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(shape)
            .background(DesignTokens.canvasFill)
            .border(BorderWidth, DesignTokens.hairline, shape)
            .padding(CardPad),
    ) {
        Text(
            "文件信息",
            fontSize = 16.sp,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
        rows.forEach { row ->
            Spacer(Modifier.height(RowGap))
            Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                Text(
                    row.label,
                    modifier = Modifier.width(LabelWidth),
                    fontSize = 12.sp,
                    color = DesignTokens.textSub,
                    maxLines = 1,
                )
                Text(
                    row.value,
                    modifier = Modifier.weight(1f),
                    fontSize = 12.sp,
                    fontWeight = FontWeight.Medium,
                    color = DesignTokens.textMain,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
        if (hasPath) {
            Spacer(Modifier.height(PathGap))
            Text(
                path,
                fontSize = 12.sp,
                color = DesignTokens.textSub,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}

private data class FileInfoRow(val label: String, val value: String)

/** 行序：分辨率 → 时长 → 格式；无效项直接不产生行。 */
private fun fileInfoRows(width: Int, height: Int, durationMs: Long, path: String): List<FileInfoRow> =
    buildList {
        if (width > 0 && height > 0) add(FileInfoRow("分辨率", "${width}×${height}"))
        if (durationMs > 0) add(FileInfoRow("时长", durationLabel(durationMs)))
        fileFormat(path)?.let { add(FileInfoRow("格式", it)) }
    }

/** `0:00:05` 口径与 `MediaItem.formatDuration()` 一致（统一交给列表与详情同一串）。 */
private fun durationLabel(durationMs: Long): String {
    val s = durationMs / 1000
    return "%d:%02d:%02d".format(s / 3600, s / 60 % 60, s % 60)
}

/** 取文件名最后一个 `.` 之后的部分并大写；无点、点后为空、或整串就是扩展名时视为缺失。 */
private fun fileFormat(path: String): String? {
    if (path.isBlank()) return null
    val name = path.substringAfterLast('/')
    val ext = name.substringAfterLast('.', "")
    if (ext.isEmpty() || ext == name) return null
    return ext.uppercase()
}

private val CardCorner = 16.dp
private val CardPad = 14.dp
private val RowGap = 8.dp
private val PathGap = 10.dp
private val LabelWidth = 48.dp
private val BorderWidth = 0.5.dp

// ───────────────────────────── Preview ─────────────────────────────

@Preview(widthDp = 360, heightDp = 190)
@Composable
private fun DetailFileInfoCardPreview() {
    Column(Modifier.background(DesignTokens.pageBackground).padding(DesignTokens.screenPad.dp)) {
        DetailFileInfoCard(
            width = 1920,
            height = 1080,
            durationMs = 5_000,
            path = "/sdcard/Movies/exit_sync.mp4",
        )
    }
}

/** 满缺态：宽高 0、时长 0、路径无扩展名 ⇒ 整卡不渲染，画布只剩 13dp 屏边。 */
@Preview(widthDp = 360, heightDp = 40)
@Composable
private fun DetailFileInfoCardEmptyPreview() {
    Column(Modifier.background(DesignTokens.pageBackground).padding(DesignTokens.screenPad.dp)) {
        DetailFileInfoCard(width = 0, height = 0, durationMs = 0, path = "")
    }
}