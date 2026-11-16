package com.kivo.cinema.emby.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.IconButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Popup
import androidx.compose.ui.window.PopupProperties
import com.kivo.cinema.R
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 317 · 服务器首页顶部栏（悬浮在 hero 之上）。
 *
 * ## 为什么单独一个文件
 * 它包含三块独立职责：**返回箭头**、**服务器名胶囊 + 切服弹层**、**两个自绘小图形**。
 * 塞进 `EmbyBrowseScreen` 会让那个文件同时负责布局、数据、弹层与图形绘制
 * （曾经长到 466 行，违反单文件 ≤300 行铁律）。
 *
 * ## 视觉约定（对齐 Rex 真机截图）
 * · 左上只有一个**箭头图标**，不套药丸、不带文字；
 * · 右上角是**浅色胶囊**：官方 Emby 绿标 + **粗体**服名 + ⌄；
 * · 点胶囊弹切服菜单：白卡片、每项同款 **官方 Emby 绿标 + 粗体名**，当前服右侧绿色对勾。
 * 胶囊与弹层是同一套浅色语言，和 App 主体（资源库卡片/详情页）一致。
 */
@Composable
internal fun EmbyBrowseTopBar(
    serverName: String,
    chromeFg: Color,
    currentSourceId: String?,
    sources: List<Pair<String, String>>,
    onSwitchSource: (String) -> Unit,
    onBack: () -> Unit,
) {
    Row(
        Modifier.fillMaxWidth().statusBarsPadding().padding(horizontal = 8.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        IconButton(onClick = onBack) {
            BackArrow(color = chromeFg)
        }
        Spacer(Modifier.weight(1f))
        var menuOpen by remember { mutableStateOf(false) }
        Box {
            // 浅色胶囊：与切服白卡片同一套语言（对齐 Rex 截图中底部那枚服名牌）。
            Surface(
                shape = RoundedCornerShape(50),
                color = Color.White.copy(alpha = .95f),
                shadowElevation = 4.dp,
            ) {
                Row(
                    Modifier.clickable { if (sources.size > 1) menuOpen = true }
                        .padding(horizontal = 10.dp, vertical = 6.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    BrandMark(20.dp)
                    Text(
                        serverName,
                        fontSize = 13.sp,
                        fontWeight = FontWeight.Bold,
                        color = DesignTokens.textMain,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        modifier = Modifier.padding(start = 6.dp).widthIn(max = 160.dp),
                    )
                    if (sources.size > 1) {
                        Text(
                            "⌄", fontSize = 14.sp, color = DesignTokens.textSub,
                            modifier = Modifier.padding(start = 2.dp),
                        )
                    }
                }
            }
            if (menuOpen) {
                SwitchServerPopup(
                    currentSourceId = currentSourceId,
                    sources = sources,
                    onDismiss = { menuOpen = false },
                    onPick = { id ->
                        menuOpen = false
                        if (id != currentSourceId) onSwitchSource(id)
                    },
                    anchorY = with(LocalDensity.current) { 92.dp.roundToPx() },
                )
            }
        }
    }
}

/** 返回箭头（三段线，跟随前景色）。 */
@Composable
private fun BackArrow(color: Color) {
    Canvas(Modifier.size(22.dp)) {
        val w = size.width
        val h = size.height
        val p = Path().apply {
            moveTo(w * 0.62f, h * 0.16f)
            lineTo(w * 0.30f, h * 0.50f)
            lineTo(w * 0.62f, h * 0.84f)
        }
        drawPath(
            p, color = color,
            style = Stroke(width = w * 0.10f, cap = StrokeCap.Round, join = StrokeJoin.Round),
        )
    }
}

/**
 * 官方 Emby 品牌标志（绿色菱形白三角，`R.drawable.emby_logo`）。
 *
 * 之前这里自绘一个空心三角，浅色胶囊/白卡片上又淡又看不清（真机反馈「图标没看见」）。
 * 资源库卡片、其他播放器用的都是这同一张官方图，这里直接复用，风格统一。
 */
@Composable
private fun BrandMark(size: androidx.compose.ui.unit.Dp) {
    Image(
        painterResource(R.drawable.emby_logo),
        contentDescription = null,
        modifier = Modifier.size(size),
    )
}

/**
 * 切服弹层：白卡片 + 深字 + 阴影，与 App 主体的浅色语言一致（不是黑色浮层）。
 *
 * 【为什么自绘 Popup 而不用 DropdownMenu】Material 的 `DropdownMenu` 取
 * `colorScheme.surface`；我们把 surface 设成了吸色后的浅色 ⇒ 白字落浅底，
 * 整块菜单看不清（真机确认）。所以自己定色。
 */
@Composable
private fun SwitchServerPopup(
    currentSourceId: String?,
    sources: List<Pair<String, String>>,
    onDismiss: () -> Unit,
    onPick: (String) -> Unit,
    anchorY: Int,
) {
    Popup(
        alignment = Alignment.TopEnd,
        offset = IntOffset(0, anchorY),
        onDismissRequest = onDismiss,
        properties = PopupProperties(focusable = true),
    ) {
        Column(
            Modifier
                .width(228.dp)
                .shadow(16.dp, RoundedCornerShape(16.dp))
                .clip(RoundedCornerShape(16.dp))
                .background(Color.White)
                .padding(vertical = 6.dp),
        ) {
            sources.forEach { (id, name) ->
                val selected = id == currentSourceId
                Row(
                    Modifier.fillMaxWidth()
                        .clickable { onPick(id) }
                        .padding(horizontal = 16.dp, vertical = 12.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    BrandMark(22.dp)
                    Text(
                        name,
                        color = DesignTokens.textMain.copy(alpha = if (selected) 1f else .86f),
                        fontSize = 15.sp,
                        fontWeight = FontWeight.Bold,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        modifier = Modifier.padding(start = 12.dp).weight(1f),
                    )
                    if (selected) CurrentCheck()
                }
            }
        }
    }
}

/** 当前服右侧的绿色对勾（图标统一用官方 Emby logo，选中态改由对勾表达）。 */
@Composable
private fun CurrentCheck() {
    Canvas(Modifier.size(16.dp)) {
        val w = size.width
        val h = size.height
        val tick = Path().apply {
            moveTo(w * 0.16f, h * 0.54f)
            lineTo(w * 0.40f, h * 0.78f)
            lineTo(w * 0.84f, h * 0.24f)
        }
        drawPath(
            tick, color = Color(0xFF34C759),
            style = Stroke(width = w * 0.14f, cap = StrokeCap.Round, join = StrokeJoin.Round),
        )
    }
}
