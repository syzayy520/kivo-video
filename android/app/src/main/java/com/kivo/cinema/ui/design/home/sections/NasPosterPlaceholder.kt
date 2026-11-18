package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.HomeEntry
import kotlin.math.absoluteValue

/**
 * NAS 影片的封面占位（P3）。
 *
 * 【为什么不是真海报】真机确认过 NAS 影片目录里只有 `.mkv`，没有 nfo/poster.jpg；
 * 抽帧要靠 mpv 内核（播放页预览那种重活），给首页 12 张卡各起一个内核会拖垮首页。
 * 所以这里用**按片名哈希取色的渐变 + 片名首字**做封面：每片颜色稳定且互不相同，
 * 有辨识度，也远好过一块纯黑空框。
 *
 * 后续要真海报时，在这一层替换即可（调用方看到的仍是"一个 posterBlock"）。
 */
@Composable
internal fun NasPosterPlaceholder(
    entry: HomeEntry,
    modifier: Modifier = Modifier,
    shape: Shape = RoundedCornerShape(12.dp),
) {
    val hue = (entry.title.hashCode().absoluteValue % 360).toFloat()
    val top = Color.hsv(hue, 0.42f, 0.58f)
    val bottom = Color.hsv((hue + 24f) % 360f, 0.55f, 0.36f)
    Box(
        modifier.fillMaxSize().clip(shape).background(Brush.verticalGradient(listOf(top, bottom))),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            entry.title.take(1),
            fontSize = 34.sp, fontWeight = FontWeight.Bold,
            color = Color.White.copy(alpha = 0.92f),
        )
    }
}
