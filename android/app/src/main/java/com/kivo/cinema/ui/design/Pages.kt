package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 我的占位 / 共用时间格式化（236 设计稿 §三）。
 * 设置页已迁出到 ui/design/settings/（261 卡）；资源库页在 SourcesScreen；
 * 发现页在 DiscoverPage.kt（239 重写）；在看页在 WatchingPage.kt。
 * 数值全部帧实测；内容图给色、chrome 退后。
 */
private val pad = DesignTokens.screenPad.dp

/** 我的页（有观看统计/Pro——数据未建，v1 极简档案位，明示占位）。 */
@Composable
fun ProfilePage(videoCount: Int) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("我的", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(14.dp))
        GlassSurface(Modifier.fillMaxWidth()) {
            Row(Modifier.padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier.size(56.dp).clip(CircleShape).background(Color(0xFFEFE9F2)),
                    contentAlignment = Alignment.Center
                ) { Text("👤", fontSize = 26.sp) }
                Spacer(Modifier.width(12.dp))
                Column {
                    Text("kivo 观众", fontSize = DesignTokens.fontRowTitle.sp,
                        fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
                    Text("媒体库 ${videoCount} 个视频 · 统计随功能接入",
                        fontSize = 11.sp, color = DesignTokens.textSub)
                }
            }
        }
    }
}

/** 相对时间（t26/0083 帧：「3 天前 · 来源」前半）。WatchingPage/DiscoverPage 共用。 */
internal fun relDays(addedSec: Long): String {
    val days = (System.currentTimeMillis() / 1000 - addedSec) / 86400
    return when {
        days <= 0L -> "今天"
        days == 1L -> "昨天"
        days < 30 -> "" + days + " 天前"
        else -> formatDate(addedSec)
    }
}

internal fun fmtSeen(positionMs: Long): String {
    val s = positionMs / 1000
    return if (s >= 3600) "%d:%02d:%02d".format(s / 3600, s / 60 % 60, s % 60)
    else "%d:%02d".format(s / 60, s % 60)
}

internal fun formatDate(addedSec: Long): String {
    val d = java.util.Calendar.getInstance()
    d.timeInMillis = addedSec * 1000L
    return "%04d-%02d-%02d".format(
        d.get(java.util.Calendar.YEAR), d.get(java.util.Calendar.MONTH) + 1,
        d.get(java.util.Calendar.DAY_OF_MONTH))
}
