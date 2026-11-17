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

/** 选中项胶囊底：浅色态白色 60%（半透明，透出下层的灰 pill）。✅ 实拍 `docs/audit/golden/rex/2026_11_16_21_22_14_IMG_0366.jpg` */
private val SelectedCapsuleLight = Color.White.copy(alpha = 0.60f)

/** 选中项胶囊底：暗色态白色 14%（同一张实拍的半透明做法，压到暗底上等效可读）。✅ 同帧 + 暗态 alpha 为 ◐ 推断 */
private val SelectedCapsuleDark = Color.White.copy(alpha = 0.14f)

/**
 * 悬浮导航胶囊与独立搜索钮。
 *
 * 选中口径（2026-11-16 B1 起）：**选中项 = 浅灰/白半透明胶囊底 + 原色前景**，不是蓝色 tint。
 * - 浅色态前景 `DesignTokens.textMain`（近黑），暗色态 `Color.White`；
 * - 未选中前景保持 `navIdle` / `navIdleDark`，搜索钮始终 `idle`（不参与选中）；
 * - 证据：`docs/audit/golden/rex/2026_11_16_21_22_14_IMG_0366.jpg`（资源库页底部胶囊，
 *   选中项为白胶囊 + 黑色图标与文字，safari 式语义）。
 */
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
                    val fg = if (active) (if (onDark) Color.White else DesignTokens.textMain) else idle
                    Column(Modifier.weight(1f).fillMaxHeight().padding(vertical = 3.dp)
                        .clip(RoundedCornerShape(22.dp))
                        .background(
                            if (active) (if (onDark) SelectedCapsuleDark else SelectedCapsuleLight)
                            else Color.Transparent
                        )
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