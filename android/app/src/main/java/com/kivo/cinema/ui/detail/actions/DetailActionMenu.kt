package com.kivo.cinema.ui.detail.actions

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.DpOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.media.marks.FilmMark

// Apple 系统灰阶：标签 #1C1C1E、次要 #8E8E93、分隔 #3C3C43@8%、选中底色为品牌紫的极淡版。
private val Ink = Color(0xFF1C1C1E)
private val Muted = Color(0xFF8E8E93)
private val Hairline = Color(0x143C3C43)
private val Accent = Color(0xFF655978)
private val AccentWash = Color(0xFFF3F0F8)
private val ChipFill = Color(0xFFF2F2F7)

/**
 * 影片标记菜单（待看 / 已观看）。
 *
 * 【形态】锚点弹出，不是底部弹层：`DropdownMenu` 会锚定它的父布局（收藏按钮），
 * 所以本组件**必须**放在该按钮所在的 Box 内，不能提到页面级。
 *
 * 【质感】按 iOS popover 的口径做：近白底 + 极细高光边 + 大扩散柔和阴影 + 系统灰阶层级；
 * 选中态用极淡品牌紫底 + 右侧对勾，未选**不画任何符号**（iOS 菜单的惯例，不用 "+"）。
 */
@Composable
internal fun DetailActionMenu(
    actions: DetailActions,
    expanded: Boolean,
    onDismiss: () -> Unit,
) {
    DropdownMenu(
        expanded = expanded,
        onDismissRequest = onDismiss,
        modifier = Modifier.width(272.dp),
        // 与按钮留 6dp 气口；水平方向交给系统避让（贴屏时自动内收，不硬顶到边缘外）。
        offset = DpOffset(0.dp, 6.dp),
        shape = RoundedCornerShape(20.dp),
        containerColor = Color(0xFFFCFCFE),
        tonalElevation = 0.dp,
        shadowElevation = 22.dp,
        border = BorderStroke(0.5.dp, Color.White.copy(alpha = .9f)),
    ) {
        Column(Modifier.padding(vertical = 6.dp)) {
            MarkRow("待看", "留到下次观看", ActionGlyph.Bookmark,
                selected = actions.marks.watchlist, enabled = actions.enabled) {
                actions.toggle(FilmMark.WATCHLIST)
                onDismiss()
            }
            Box(Modifier.padding(start = 58.dp, end = 14.dp).height(0.5.dp)
                .fillMaxWidth().background(Hairline))
            MarkRow("已观看", "记录看过的影片", ActionGlyph.Watched,
                selected = actions.marks.watched, enabled = actions.enabled) {
                actions.toggle(FilmMark.WATCHED)
                onDismiss()
            }
        }
    }
}

@Composable
private fun MarkRow(
    title: String, subtitle: String, glyph: ActionGlyph,
    selected: Boolean, enabled: Boolean, onClick: () -> Unit,
) {
    val active = selected
    Row(
        Modifier.fillMaxWidth()
            .background(if (active) AccentWash else Color.Transparent)
            .toggleable(active, enabled, role = Role.Checkbox) { onClick() }
            .padding(horizontal = 14.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(
            Modifier.size(32.dp).clip(RoundedCornerShape(9.dp))
                .background(if (active) Color(0xFFE7E1F0) else ChipFill),
            contentAlignment = Alignment.Center,
        ) {
            DetailActionGlyph(glyph, if (active) Accent else Ink, Modifier.size(17.dp))
        }
        Column(Modifier.weight(1f).padding(start = 12.dp),
            verticalArrangement = Arrangement.spacedBy(2.dp)) {
            Text(
                title,
                color = Ink.copy(alpha = if (enabled) 1f else .35f),
                fontSize = 15.sp, fontWeight = FontWeight.Medium, letterSpacing = (-0.2).sp,
            )
            Text(
                subtitle,
                color = Muted.copy(alpha = if (enabled) 1f else .45f),
                fontSize = 11.5.sp, lineHeight = 14.sp, letterSpacing = 0.sp,
            )
        }
        if (active) {
            Box(Modifier.size(20.dp).clip(CircleShape).background(Accent),
                contentAlignment = Alignment.Center) {
                DetailActionGlyph(ActionGlyph.Tick, Color.White, Modifier.size(11.dp))
            }
        }
    }
}
