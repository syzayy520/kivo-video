package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.home.NavGlyph
import com.kivo.cinema.ui.design.home.NavigationBackdrop
import com.kivo.cinema.ui.design.home.NavigationBackdropState

/** Floating navigation capsule and separate search button. */
@Composable
fun CapsuleNavBar(
    tabs: List<Pair<String, String>>,
    selected: Int,
    onSelect: (Int) -> Unit,
    onSearch: () -> Unit,
    onDark: Boolean = false,
    backdrop: NavigationBackdropState? = null,
) {
    val context = LocalContext.current
    LaunchedEffect(Unit) { AppearancePrefs.bind(context) }
    val fg = Color.Black
    val shape = RoundedCornerShape(25.dp)
    val glass = Brush.verticalGradient(listOf(
        Color.White.copy(alpha = if (onDark) .63f else .82f),
        Color(0xFFEAE3EF).copy(alpha = if (onDark) .59f else .76f)))
    fun Modifier.material() = shadow(6.dp, shape, ambientColor = Color.Black.copy(alpha=.12f))
        .clip(shape).border(.7.dp, Color.White.copy(alpha=.45f), shape)
    Row(Modifier.fillMaxWidth().padding(horizontal = DesignTokens.navSidePad.dp),
        verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(12.dp)) {
        Box(Modifier.weight(1f).height(50.dp).material()) {
            if (backdrop != null) NavigationBackdrop(backdrop, Modifier.matchParentSize())
            Box(Modifier.matchParentSize().background(glass))
            Row(Modifier.fillMaxSize().padding(horizontal = 5.dp), verticalAlignment = Alignment.CenterVertically) {
            tabs.forEachIndexed { index, (label, _) ->
                Column(Modifier.weight(1f).fillMaxHeight().padding(vertical = 3.dp)
                    .clip(RoundedCornerShape(22.dp))
                    .background(if (index == selected) Color.White.copy(alpha=.14f) else Color.Transparent)
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
        Box(Modifier.size(50.dp).material().clip(CircleShape).clickable { onSearch() },
            contentAlignment = Alignment.Center) {
            if (backdrop != null) NavigationBackdrop(backdrop, Modifier.matchParentSize())
            Box(Modifier.matchParentSize().background(glass))
            NavGlyph("search", fg, Modifier.size(28.dp))
        }
    }
}
