package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface

/**
 * TMDB 错误态卡（home.error.network.* 五类原文文案 + 重试，M04-S04）。
 * ✅ 从 `DiscoverPage.kt` 等值搬迁 —— 五类文案不可合并成一句泛化错误（DISC-09）。
 */
@Composable
internal fun TmdbErrorCard(kind: String, onRetry: () -> Unit) {
    val kindText = when (kind) {
        "timeout" -> "连接超时，请稍后重试"
        "offline" -> "网络未连接，请检查网络后重试"
        "secure" -> "无法建立安全连接，请检查服务器设置"
        "server" -> "无法连接服务器，请稍后重试"
        else -> "网络连接已中断，请重试"
    }
    Spacer(Modifier.height(20.dp))
    GlassSurface(Modifier.fillMaxWidth().padding(horizontal = 13.dp)) {
        Column(
            Modifier.fillMaxWidth().padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text("无法加载 TMDB 内容", fontSize = 15.sp,
                fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
            Spacer(Modifier.height(4.dp))
            Text(kindText, fontSize = 13.sp, color = DesignTokens.textSub)
            Spacer(Modifier.height(10.dp))
            Text("重试", fontSize = 14.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.accentGreen,
                modifier = Modifier
                    .clip(RoundedCornerShape(8.dp))
                    .clickable { onRetry() }
                    .padding(horizontal = 16.dp, vertical = 6.dp))
        }
    }
}