package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 播放体验 2×2 入口卡（268）：播放/音画/操控/AI字幕，各对应真实子页路由。
 * 每卡 = 淡底 chip + 标题 + 单行副标题，整卡可点。只组合入口，不堆业务。
 */
@Composable
internal fun SettingsEntryGrid(onOpen: (SettingsRoute) -> Unit) {
    Column {
        Row(horizontalArrangement = Arrangement.spacedBy(SettingsVisuals.cardGap.dp)) {
            EntryCard("播放", "缓冲 · 习惯 · 字幕", SettingsIcons.play, SettingsVisuals.blue,
                Modifier.weight(1f)) { onOpen(SettingsRoute.PLAYBACK) }
            EntryCard("音画", "内核 · 画面 · 音频", SettingsIcons.speaker, SettingsVisuals.purple,
                Modifier.weight(1f)) { onOpen(SettingsRoute.AV) }
        }
        Spacer(Modifier.height(SettingsVisuals.cardGap.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(SettingsVisuals.cardGap.dp)) {
            EntryCard("操控", "按钮 · 手势 · 双击", SettingsIcons.hand, SettingsVisuals.orange,
                Modifier.weight(1f)) { onOpen(SettingsRoute.CONTROLS) }
            EntryCard("AI 字幕", "语音转字幕 · 翻译", SettingsIcons.subtitle, SettingsVisuals.teal,
                Modifier.weight(1f)) { onOpen(SettingsRoute.AI) }
        }
    }
}

@Composable
private fun EntryCard(
    title: String, subtitle: String, icon: ImageVector, color: Color,
    modifier: Modifier = Modifier, onClick: () -> Unit,
) {
    SettingsCard(modifier.clickable(role = Role.Button, onClick = onClick)) {
        Column(Modifier.fillMaxWidth().heightIn(min = 96.dp).padding(14.dp)) {
            RowIcon(icon, color)
            Spacer(Modifier.height(10.dp))
            Text(title, fontSize = 16.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain)
            Text(subtitle, fontSize = 12.sp, color = SettingsVisuals.sub, maxLines = 1)
        }
    }
}