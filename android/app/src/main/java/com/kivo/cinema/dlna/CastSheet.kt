package com.kivo.cinema.dlna

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 投屏设备选择 sheet（230 设计稿 D5）。
 *
 * UI 锚点说明（设计稿 §二明示）：设计稿 无 DLNA（iOS 走 AirPlay），本 sheet 按播放器暗色
 * chrome token（#1C1C1E 底 + 白行文字）与 规格 05 规格的行高/圆角补位，不自造样式。
 */
@Composable
fun CastSheet(
    visible: Boolean,
    scanning: Boolean,
    devices: List<DlnaDevice>,
    message: String,
    onSelect: (DlnaDevice) -> Unit,
    onRescan: () -> Unit,
    onDismiss: () -> Unit,
) {
    if (!visible) return
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(Color(0xFF1C1C1E), RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp))
            .padding(16.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("投屏", color = Color.White, fontSize = 18.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.weight(1f))
            Text(
                if (scanning) "扫描中…" else "重新扫描",
                color = Color(0xFF34C759), fontSize = 13.sp,
                modifier = Modifier
                    .background(Color(0xFF2C2C2E), RoundedCornerShape(8.dp))
                    .clickable(enabled = !scanning) { onRescan() }
                    .padding(horizontal = 10.dp, vertical = 4.dp)
            )
        }
        Spacer(Modifier.height(10.dp))
        if (message.isNotBlank()) {
            Text(message, color = Color(0xFF98989E), fontSize = 12.sp)
            Spacer(Modifier.height(8.dp))
        }
        if (devices.isEmpty() && !scanning) {
            Text("未发现渲染器。确认电视/音箱与手机在同一网络。", color = Color(0xFF98989E),
                fontSize = 13.sp)
            Spacer(Modifier.height(8.dp))
        }
        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            devices.forEach { d ->
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(Color(0xFF2C2C2E), RoundedCornerShape(13.dp))
                        .clickable { onSelect(d) }
                        .padding(horizontal = 16.dp, vertical = 12.dp)
                ) {
                    Text("投", fontSize = 13.sp, color = Color(0xFF34C759),
                        fontWeight = FontWeight.Bold)
                    Spacer(Modifier.weight(1f))
                    Column(horizontalAlignment = Alignment.End) {
                        Text(d.friendlyName, color = Color.White, fontSize = 15.sp,
                            maxLines = 1)
                        Text(hostOf(d), color = Color(0xFF98989E), fontSize = 11.sp)
                    }
                }
            }
        }
        Spacer(Modifier.height(4.dp))
        Text("收起", color = Color(0xFF98989E), fontSize = 12.sp,
            modifier = Modifier
                .fillMaxWidth()
                .clickable { onDismiss() }
                .padding(vertical = 8.dp),
            textAlign = androidx.compose.ui.text.style.TextAlign.Center)
    }
}

private fun hostOf(d: DlnaDevice): String =
    Regex("://([^/:]+)").find(d.location)?.groupValues?.get(1) ?: d.location
