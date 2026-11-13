package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.home.NavGlyph

/** 悬浮导航胶囊与独立搜索钮（269 v3：扁平灰 pill + 蓝选中；暗态保留 E11 语义）。 */
@Composable
fun CapsuleNavBar(
    tabs: List<Pair<String, String>>,
    selected: Int,
    onSelect: (Int) -> Unit,
    onSearch: () -> Unit,
    onDark: Boolean = false,
) {
    val context = LocalContext.current
    LaunchedEffect(Unit) { AppearancePrefs.bind(context) }
    val pill = if (onDark) DesignTokens.navPillDark else DesignTokens.navPill
    val idle = if (onDark) DesignTokens.navIdleDark else DesignTokens.navIdle
    val shape = RoundedCornerShape(25.dp)
    val shadowColor = Color.Black.copy(alpha = .12f)
    Row(Modifier.fillMaxWidth().padding(horizontal = DesignTokens.navSidePad.dp),
        verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(12.dp)) {
        Box(Modifier.weight(1f).height(50.dp)
            .shadow(6.dp, shape, ambientColor = shadowColor).clip(shape).background(pill)) {
            Row(Modifier.fillMaxSize().padding(horizontal = 5.dp), verticalAlignment = Alignment.CenterVertically) {
                tabs.forEachIndexed { index, (label, _) ->
                    val active = index == selected
                    val fg = if (active) DesignTokens.navActive else idle
                    Column(Modifier.weight(1f).fillMaxHeight().padding(vertical = 3.dp)
                        .clip(RoundedCornerShape(22.dp))
                        .background(if (active) (if (onDark) Color.White.copy(alpha = .14f) else Color.White) else Color.Transparent)
                        .clickable { onSelect(index) },
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.Center) {
                        NavGlyph(label, fg, Modifier.size(25.dp))
                        if (!AppearancePrefs.hideNavLabels)
                            Text(label, fontSize = 10.sp, lineHeight = 13.sp, color = fg)
                    }
                }
            }
        }
        Box(Modifier.size(50.dp)
            .shadow(6.dp, CircleShape, ambientColor = shadowColor).clip(CircleShape)
            .background(pill).clickable { onSearch() },
            contentAlignment = Alignment.Center) {
            NavGlyph("search", idle, Modifier.size(28.dp))
        }
    }
}