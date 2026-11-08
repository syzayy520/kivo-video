package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
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
import com.kivo.cinema.library.MediaItem

/**
 * 静态占位页（236 设计稿 §三）：资源库入口 / 设置骨架 / 我的占位。
 * 发现页在 DiscoverPage.kt（239 重写）；在看页在 WatchingPage.kt。
 * 数值全部帧实测；内容图给色、chrome 退后。
 */
private val pad = DesignTokens.screenPad.dp

/** 资源库页（云服务矩阵语义 + 我们的源集合；玻璃卡矩阵）。 */
@Composable
fun SourcesPage(
    onOpenSmb: () -> Unit,
    onOpenEmby: () -> Unit,
    onOpenLibrary: () -> Unit,
    onOpenPrivate: () -> Unit,
    videoCount: Int,
) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("资源库", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(4.dp))
        Text("本地 ${videoCount} 个视频 · 云端与网络源", fontSize = 12.sp,
            color = DesignTokens.textSub)
        Spacer(Modifier.height(14.dp))
        SourceCard("本地影片", "MediaStore 扫描 · 海报墙 · 点播", "📁") { onOpenLibrary() }
        Spacer(Modifier.height(DesignTokens.gap.dp))
        SourceCard("SMB / 网络共享", "libsmb2 直连 · 硬解过本地代理 · 断点下载", "🗄") { onOpenSmb() }
        Spacer(Modifier.height(DesignTokens.gap.dp))
        SourceCard("Emby / Jellyfin", "AuthenticateByName 登录 · 直连流播放", "☁") { onOpenEmby() }
        Spacer(Modifier.height(DesignTokens.gap.dp))
        SourceCard("私密区", "生物特征门 · 受保护观看空间", "🔒") { onOpenPrivate() }
    }
}

@Composable
private fun SourceCard(title: String, desc: String, icon: String, onClick: () -> Unit) {
    GlassSurface(Modifier.fillMaxWidth().clickable { onClick() }) {
        Row(Modifier.fillMaxWidth().padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier
                    .size(44.dp)
                    .clip(CircleShape)
                    .background(Color(0xFFEFE9F2)),
                contentAlignment = Alignment.Center
            ) { Text(icon, fontSize = 20.sp) }
            Spacer(Modifier.width(12.dp))
            Column {
                Text(title, fontSize = DesignTokens.fontRowTitle.sp, fontWeight = FontWeight.Bold,
                    color = DesignTokens.textMain)
                Text(desc, fontSize = 11.sp, color = DesignTokens.textSub)
            }
            Spacer(Modifier.weight(1f))
            Text("›", fontSize = 22.sp, color = DesignTokens.textSub)
        }
    }
}

/** 设置页（set_a 帧：inset 分组卡 59pt 行 + 节头 + 多行脚注"敢解释"）。 */
@Composable
fun SettingsPage(clearTextOk: Boolean, buildLabel: String,
                 previewEnabled: Boolean = false, onPreviewChanged: (Boolean) -> Unit = {}) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("设置", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(14.dp))
        GroupSection("首页") {
            com.kivo.cinema.ui.design.home.HomePreviewSetting(previewEnabled, onPreviewChanged)
        }
        Spacer(Modifier.height(14.dp))
        GroupSection("播放") {
            GroupRow("网络明文传输", if (clearTextOk) "已开启" else "已关闭")
            GroupRow("解码", "硬解优先 · 失败报 reason")
        }
        Spacer(Modifier.height(8.dp))
        Text("本卡为设置页骨架：播放/字幕/解码各组随功能卡逐项接入（236 §六）。",
            fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub,
            lineHeight = 15.sp)
        Spacer(Modifier.height(14.dp))
        GroupSection("关于") {
            GroupRow("构建", buildLabel)
            GroupRow("UI 对齐", "设计稿 1.1 录屏规格（236 卡）")
        }
        Spacer(Modifier.height(8.dp))
        Text("kivo video 的界面规格来自 设计稿 真机录屏逐帧拆解；所有数值为实测。",
            fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub,
            lineHeight = 15.sp)
    }
}

@Composable
private fun GroupSection(header: String, content: @Composable () -> Unit) {
    Text(header, fontSize = 13.sp, color = DesignTokens.textSub,
        modifier = Modifier.padding(start = 4.dp))
    Spacer(Modifier.height(6.dp))
    GlassSurface(Modifier.fillMaxWidth()) { Column { content() } }
}

@Composable
private fun GroupRow(title: String, value: String) {
    Row(
        Modifier
            .fillMaxWidth()
            .height(DesignTokens.rowHeight.dp)
            .padding(horizontal = 16.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
        Spacer(Modifier.weight(1f))
        Text(value, fontSize = 15.sp, color = DesignTokens.textSub)
    }
}

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
