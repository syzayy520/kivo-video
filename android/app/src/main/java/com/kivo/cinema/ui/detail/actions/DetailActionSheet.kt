package com.kivo.cinema.ui.detail.actions

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.Text
import androidx.compose.material3.rememberModalBottomSheetState
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.media.marks.FilmMark
import kotlinx.coroutines.launch

private val SheetInk = Color(0xFF292A30)
private val SheetMuted = Color(0xFF77777F)
private val SheetAccent = Color(0xFF655978)

/** 影片标记底部卡（待看/已观看）；持久化仍在 DetailActions。「更多」走 DetailMorePopup。 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
internal fun DetailActionSheet(
    actions: DetailActions,
    onDismiss: () -> Unit,
) {
    val state = rememberModalBottomSheetState(skipPartiallyExpanded = true)
    val scope = rememberCoroutineScope()
    var closing by remember { mutableStateOf(false) }
    fun close(after: () -> Unit = {}) {
        if (closing) return
        closing = true
        scope.launch {
            state.hide()
            onDismiss()
            after()
        }
    }
    ModalBottomSheet(
        onDismissRequest = onDismiss,
        sheetState = state,
        shape = RoundedCornerShape(topStart = 30.dp, topEnd = 30.dp),
        containerColor = Color(0xFFF5F4F2),
        contentColor = SheetInk,
        scrimColor = Color.Black.copy(alpha = .32f),
        tonalElevation = 0.dp,
        dragHandle = {
            Box(Modifier.padding(top = 10.dp, bottom = 2.dp)
                .size(34.dp, 4.dp).clip(CircleShape).background(Color(0xFFD2D0D4)))
        },
    ) {
        Column(Modifier.fillMaxWidth().verticalScroll(rememberScrollState())
            .padding(horizontal = 24.dp).padding(bottom = 24.dp)) {
            Row(Modifier.fillMaxWidth().padding(bottom = 18.dp),
                verticalAlignment = Alignment.CenterVertically) {
                Text("影片标记", fontSize = 20.sp, fontWeight = FontWeight.SemiBold,
                    modifier = Modifier.weight(1f))
                Box(Modifier.size(48.dp).clip(CircleShape)
                    .semantics { contentDescription = "关闭操作卡片" }
                    .clickable(enabled = !closing, role = Role.Button) { close() },
                    contentAlignment = Alignment.Center) {
                    Box(Modifier.size(30.dp).clip(CircleShape).background(Color(0xFFE9E7E7)),
                        contentAlignment = Alignment.Center) {
                        DetailActionGlyph(ActionGlyph.Close, SheetMuted, Modifier.size(14.dp))
                    }
                }
            }
            SheetAction("待看", "留到下次观看", ActionGlyph.Bookmark,
                selected = actions.marks.watchlist, enabled = actions.enabled && !closing) {
                actions.toggle(FilmMark.WATCHLIST)
                close()
            }
            Spacer(Modifier.height(10.dp))
            SheetAction("已观看", "记录看过的影片", ActionGlyph.Watched,
                selected = actions.marks.watched, enabled = actions.enabled && !closing) {
                actions.toggle(FilmMark.WATCHED)
                close()
            }
        }
    }
}

@Composable
private fun SheetAction(
    title: String, subtitle: String, glyph: ActionGlyph,
    selected: Boolean? = null, enabled: Boolean, onClick: () -> Unit,
) {
    val active = selected == true
    val touch = if (selected == null) Modifier.clickable(enabled, role = Role.Button, onClick = onClick)
        else Modifier.toggleable(selected, enabled, role = Role.Checkbox) { onClick() }
    Row(Modifier.fillMaxWidth().clip(RoundedCornerShape(18.dp))
        .background(if (active) Color(0xFFECE8F0) else Color.White.copy(alpha = .85f))
        .then(touch).padding(horizontal = 16.dp, vertical = 16.dp),
        verticalAlignment = Alignment.CenterVertically) {
        Box(Modifier.size(44.dp).clip(RoundedCornerShape(14.dp))
            .background(if (active) Color(0xFFE0D9E8) else Color(0xFFF0EEEF)),
            contentAlignment = Alignment.Center) {
            DetailActionGlyph(glyph, if (active) SheetAccent else SheetInk, Modifier.size(23.dp))
        }
        Column(Modifier.weight(1f).padding(horizontal = 14.dp),
            verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Text(title, color = SheetInk.copy(alpha = if (enabled) 1f else .5f),
                fontSize = 16.sp, fontWeight = FontWeight.Medium)
            Text(subtitle, color = SheetMuted, fontSize = 12.sp, lineHeight = 17.sp)
        }
        if (selected != null) {
            Box(Modifier.size(24.dp).clip(CircleShape)
                .background(if (active) SheetAccent else Color(0xFFE9E7EB)),
                contentAlignment = Alignment.Center) {
                DetailActionGlyph(if (active) ActionGlyph.Tick else ActionGlyph.Plus,
                    if (active) Color.White else SheetMuted, Modifier.size(13.dp))
            }
        } else {
            DetailActionGlyph(ActionGlyph.Chevron, SheetMuted, Modifier.size(16.dp))
        }
    }
}