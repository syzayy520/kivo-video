package com.kivo.cinema.ui.design.home

import androidx.compose.foundation.layout.*
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.foundation.selection.toggleable
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.settings.SettingsVisuals

@Composable
internal fun HomePreviewSetting(enabled: Boolean, onChange: (Boolean) -> Unit) {
    Row(Modifier.fillMaxWidth().toggleable(value = enabled, role = Role.Switch, onValueChange = onChange)
        .padding(horizontal = 16.dp, vertical = 12.dp), verticalAlignment = Alignment.CenterVertically) {
        Column(Modifier.weight(1f).padding(end = 12.dp)) {
            Text("首页预告片自动播放", fontSize = SettingsVisuals.fontRow.sp, color = DesignTokens.textMain)
            Text("默认显示图片；开启后需要能连接 YouTube", fontSize = SettingsVisuals.fontSub.sp,
                lineHeight = 17.sp, color = SettingsVisuals.sub)
        }
        Switch(checked = enabled, onCheckedChange = null,
            colors = SwitchDefaults.colors(
                checkedTrackColor = DesignTokens.accentGreen,
                checkedThumbColor = Color.White,
                uncheckedTrackColor = Color(0xFFE9E9EA),
                uncheckedThumbColor = Color.White,
                uncheckedBorderColor = Color.Transparent,
            ))
    }
}
