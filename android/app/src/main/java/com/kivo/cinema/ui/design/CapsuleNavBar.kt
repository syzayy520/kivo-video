package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 底部导航（grid_02/t22 实测）：单个白玻璃胶囊 4 tab + 分离搜索圆钮。
 * 胶囊高 50pt、距底 22pt、左右 16pt；图标 ~26pt + label 10pt；tab 间距均分。
 * onDark（E11）：深色环境上玻璃转暗玻璃、图标文字转白（hero_14/20 实证）。
 */
@Composable
fun CapsuleNavBar(
    tabs: List<Pair<String, String>>,   // (label, emoji/grapheme 图标代理)
    selected: Int,
    onSelect: (Int) -> Unit,
    onSearch: () -> Unit,
    onDark: Boolean = false,
) {
    val glassAlpha = if (onDark) 0.42f else DesignTokens.glassAlpha
    val glassColor = if (onDark) Color.Black else Color.White
    val fg = if (onDark) Color.White else DesignTokens.textMain
    val fgSub = if (onDark) Color.White.copy(alpha = 0.62f) else DesignTokens.textSub
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.navSidePad.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        GlassSurface(
            modifier = Modifier
                .weight(1f)
                .height(DesignTokens.navHeight.dp),
            radius = 25f,
            alpha = glassAlpha,
            colorOverride = glassColor,
        ) {
            Row(
                Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 6.dp),
                horizontalArrangement = Arrangement.SpaceEvenly,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                tabs.forEachIndexed { i, (label, icon) ->
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        modifier = Modifier
                            .clip(RoundedCornerShape(12.dp))
                            .clickable { onSelect(i) }
                            .padding(horizontal = 8.dp, vertical = 4.dp)
                    ) {
                        Text(icon, fontSize = 22.sp, color = fg)
                        Text(label, fontSize = DesignTokens.fontNavLabel.sp,
                            color = if (i == selected) fg else fgSub)
                    }
                }
            }
        }
        // 分离搜索圆钮（帧实测 Ø50pt，不进胶囊）
        GlassSurface(
            modifier = Modifier
                .size(DesignTokens.circleButton.dp)
                .clip(CircleShape)
                .clickable { onSearch() },
            radius = DesignTokens.circleButton,
            alpha = glassAlpha,
            colorOverride = glassColor,
        ) {
            Box(contentAlignment = Alignment.Center) {
                Text("🔍", fontSize = 20.sp, color = fg)
            }
        }
    }
}
