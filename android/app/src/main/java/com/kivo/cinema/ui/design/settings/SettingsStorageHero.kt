package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface

/**
 * 空间占用 hero（268）：只读真实缓存字节（图片 + 评分两类，能真实测量），堆叠条 + 图例 + 总量。
 * 整卡可点 → 清理缓存子页。不造假分类（视频/下载/AI模型无对应测量 ⇒ 不放）；total=0 显示空条不编默认。
 */
@Composable
internal fun SettingsStorageHero(imageBytes: Long, ratingBytes: Long, onManage: () -> Unit) {
    val total = imageBytes + ratingBytes
    val blue = SettingsVisuals.blue
    val purple = SettingsVisuals.purple
    GlassSurface(
        Modifier.fillMaxWidth().clickable(role = Role.Button, onClick = onManage), radius = 16f,
    ) {
        Column(Modifier.fillMaxWidth().padding(16.dp)) {
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically) {
                Text("空间占用", fontSize = 15.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain)
                Text("管理 ›", fontSize = 13.sp, color = blue)
            }
            Spacer(Modifier.height(10.dp))
            Text(formatBytes(total), fontSize = 26.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain, style = TextStyle(fontFeatureSettings = "tnum"))
            Text("图片 + 评分缓存", fontSize = 11.sp, color = DesignTokens.textSub)
            Spacer(Modifier.height(12.dp))
            Row(Modifier.fillMaxWidth().height(8.dp).clip(RoundedCornerShape(4.dp))
                .background(DesignTokens.hairline)) {
                if (total > 0) {
                    val f = (imageBytes.toFloat() / total).coerceIn(0f, 1f)
                    if (f > 0f) Box(Modifier.weight(f).fillMaxHeight().background(blue))
                    if (f < 1f) Box(Modifier.weight(1f - f).fillMaxHeight().background(purple))
                }
            }
            Spacer(Modifier.height(10.dp))
            Row(verticalAlignment = Alignment.CenterVertically) {
                LegendDot(blue)
                Text("图片 ${formatBytes(imageBytes)}", fontSize = 11.sp, color = DesignTokens.textSub,
                    style = TextStyle(fontFeatureSettings = "tnum"))
                Spacer(Modifier.width(14.dp))
                LegendDot(purple)
                Text("评分 ${formatBytes(ratingBytes)}", fontSize = 11.sp, color = DesignTokens.textSub,
                    style = TextStyle(fontFeatureSettings = "tnum"))
            }
        }
    }
}

@Composable
private fun LegendDot(color: Color) {
    Box(Modifier.size(7.dp).clip(CircleShape).background(color))
    Spacer(Modifier.width(5.dp))
}