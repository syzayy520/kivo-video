package com.kivo.cinema.ui.compose.player

import android.content.Intent
import android.content.IntentFilter
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.delay
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 顶栏的**自绘状态行**（v3 §1.1：左时钟 x≈63pt / y10–23pt，右网速 + 电池）。
 *
 * 【为什么要自绘】播放页是沉浸式全屏，系统状态栏被隐藏了 ——
 * 但用户仍然需要看时间与电量（录屏里它们一直在）。
 * 隐藏系统栏 ≠ 丢掉这些信息，所以在这里按实测位置画回来。
 *
 * 【数据都是真的】时钟＝系统时间；电量＝`ACTION_BATTERY_CHANGED` 的 sticky 广播
 * （不需要常驻 receiver，读一次即得）；网速＝ mpv `cache-speed`（由外部传入）。
 * 30s 刷新一次足够 —— 电量与分钟级时钟都不需要更高频率。
 */
@Composable
internal fun PlayerStatusRow(
    speedText: String,
    modifier: Modifier = Modifier,
) {
    val context = LocalContext.current
    var clock by remember { mutableStateOf(formatClock(System.currentTimeMillis())) }
    var battery by remember { mutableFloatStateOf(1f) }

    LaunchedEffect(Unit) {
        while (true) {
            clock = formatClock(System.currentTimeMillis())
            battery = readBatteryLevel(context)
            delay(30_000)
        }
    }

    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerTokens.statusRowHeight.dp)
            .playerSafeArea()
            .padding(horizontal = PlayerTokens.sidePad.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = clock,
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.timeSize.sp,
            fontFamily = FontFamily.Monospace,
            maxLines = 1,
        )
        Spacer(Modifier.weight(1f))
        if (speedText.isNotBlank()) {
            Text(
                text = speedText,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.speedSize.sp,
                fontFamily = FontFamily.Monospace,
                maxLines = 1,
            )
            Spacer(Modifier.width(14.dp))
        }
        PlayerGlyph(
            kind = "battery",
            color = PlayerTokens.textPrimary,
            fill = battery,
            modifier = Modifier.size(
                width = PlayerTokens.batteryWidth.dp,
                height = PlayerTokens.batteryHeight.dp,
            ),
        )
    }
}

private fun formatClock(ms: Long): String =
    SimpleDateFormat("HH:mm", Locale.getDefault()).format(Date(ms))

/** sticky 广播：读一次即得当前电量，读不到按满电画（不猜一个低电量吓用户）。 */
private fun readBatteryLevel(context: android.content.Context): Float {
    val intent = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
        ?: return 1f
    val level = intent.getIntExtra(android.os.BatteryManager.EXTRA_LEVEL, -1)
    val scale = intent.getIntExtra(android.os.BatteryManager.EXTRA_SCALE, -1)
    return if (level >= 0 && scale > 0) level.toFloat() / scale else 1f
}