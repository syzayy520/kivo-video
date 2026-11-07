package com.kivo.cinema.ui.rex

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
 * 底部导航（grid_02/rex_browse_a 帧实测）：单个白玻璃胶囊 4 tab + 分离搜索圆钮。
 * 胶囊高 50pt、距底 22pt、左右 16pt；图标 ~26pt + label 10pt；tab 间距均分。
 */
@Composable
fun CapsuleNavBar(
    tabs: List<Pair<String, String>>,   // (label, emoji/grapheme 图标代理)
    selected: Int,
    onSelect: (Int) -> Unit,
    onSearch: () -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = RexTokens.navSidePad.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        GlassSurface(
            modifier = Modifier
                .weight(1f)
                .height(RexTokens.navHeight.dp),
            radius = 25f,
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
                        Text(icon, fontSize = 22.sp, color = RexTokens.textMain)
                        Text(label, fontSize = RexTokens.fontNavLabel.sp,
                            color = if (i == selected) RexTokens.textMain
                                    else RexTokens.textSub)
                    }
                }
            }
        }
        // 分离搜索圆钮（帧实测 Ø50pt，不进胶囊）
        GlassSurface(
            modifier = Modifier
                .size(RexTokens.circleButton.dp)
                .clip(CircleShape)
                .clickable { onSearch() },
            radius = RexTokens.circleButton,
        ) {
            Box(contentAlignment = Alignment.Center) {
                Text("🔍", fontSize = 20.sp)
            }
        }
    }
}
