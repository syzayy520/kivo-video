package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface

/**
 * 资源库概览双卡（268）：Emby / SMB 真实配置计数（来自 accounts.bin，非实时连接探针）。
 * 诚实口径：副标题 = "N 个已配置" / "未配置"，不显示假"在线/已连接"点；整卡可点 → 数据与资源。
 * 无实时连接状态后端 ⇒ 不做连接指示灯（铁律·不造假状态）。
 */
@Composable
internal fun SettingsStatusTiles(embyCount: Int, smbCount: Int, onOpen: () -> Unit) {
    Column {
        Text("资源库", fontSize = SettingsVisuals.fontSection.sp, fontWeight = FontWeight.SemiBold,
            color = DesignTokens.textSub, modifier = Modifier.padding(start = 4.dp, bottom = 6.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(SettingsVisuals.cardGap.dp)) {
            StatusTile("Emby", embyCount, SettingsIcons.cloud, SettingsVisuals.blue,
                Modifier.weight(1f), onOpen)
            StatusTile("SMB", smbCount, SettingsIcons.drive, SettingsVisuals.indigo,
                Modifier.weight(1f), onOpen)
        }
    }
}

@Composable
private fun StatusTile(
    title: String, count: Int, icon: ImageVector, color: Color,
    modifier: Modifier = Modifier, onClick: () -> Unit,
) {
    GlassSurface(modifier.clickable(role = Role.Button, onClick = onClick), radius = 16f) {
        Row(Modifier.fillMaxWidth().padding(14.dp), verticalAlignment = Alignment.CenterVertically) {
            RowIcon(icon, color)
            Spacer(Modifier.width(10.dp))
            Column {
                Text(title, fontSize = 15.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain)
                Text(if (count > 0) "$count 个已配置" else "未配置",
                    fontSize = 11.sp, color = DesignTokens.textSub)
            }
        }
    }
}