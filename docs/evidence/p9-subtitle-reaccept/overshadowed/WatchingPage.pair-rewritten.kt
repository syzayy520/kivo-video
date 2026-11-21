package com.kivo.cinema.ui.design

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.Image
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
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.ui.components.HitBox

/**
 * 在看页（AppWatching 147 键语义；HomeEditor `viewing.progress.full.width` 契约）：
 * 整行卡 = 左缩略图 103pt 16:9 + 右标题 17pt 粗体/元信息 11pt 灰 + 黑进度条 3pt（05 §三）。
 * 弃剧/折叠记录/追剧日历 = v2。
 *
 * B2 起「在看」不再是发现的子页：它是「我的」模块的二级页，由 Shell 覆盖层承载
 * （`MainActivityShell` 的 `showWatching`），入口见 WATCH-01/SHELL-02。
 * 顶部为标准导航条（左返回 + 中标题 + 右空），系统返回键同样回到调用方。
 */
@Composable
fun WatchingPage(
    continueItems: List<Pair<MediaItem, WatchStore.Progress>>,
    posterOf: @Composable (MediaItem) -> (@Composable () -> Unit)?,
    onOpenItem: (MediaItem) -> Unit,
    onBack: () -> Unit,
) {
    BackHandler { onBack() }
    Column(Modifier.fillMaxSize()) {
        Spacer(Modifier.height(6.dp))
        WatchingNavBar(onBack)
        Spacer(Modifier.height(4.dp))
        Text("观看进度自动保存 · 上划管理", fontSize = 11.sp,
            color = DesignTokens.textSub,
            modifier = Modifier.padding(horizontal = DesignTokens.screenPad.dp))
        Spacer(Modifier.height(12.dp))
        if (continueItems.isEmpty()) {
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text("还没有在看记录\n去发现页选一部片，进度会出现在这里",
                        fontSize = 14.sp, color = DesignTokens.textSub,
                        lineHeight = 22.sp,
                        textAlign = androidx.compose.ui.text.style.TextAlign.Center)
                    Spacer(Modifier.height(14.dp))
                    Box(
                        Modifier.clip(RoundedCornerShape(50.dp))
                            .background(DesignTokens.navActive)
                            .clickable(role = androidx.compose.ui.semantics.Role.Button, onClick = onBack)
                            .padding(horizontal = 22.dp, vertical = 9.dp)
                    ) {
                        Text("去发现", color = Color.White, fontSize = 14.sp,
                            fontWeight = FontWeight.Medium)
                    }
                }
            }
            return
        }
        continueItems.forEach { (item, prog) ->
            val frac = if (prog.durationMs > 0)
                (prog.positionMs.toFloat() / prog.durationMs).coerceIn(0f, 1f) else 0f
            Row(
                Modifier
                    .fillMaxWidth()
                    .padding(horizontal = DesignTokens.screenPad.dp,
                        vertical = DesignTokens.gap.dp)
                    .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
                    .background(Color.White.copy(alpha = 0.92f))
                    .clickable { onOpenItem(item) }
            ) {
                Box(
                    Modifier
                        .padding(start = 10.dp, top = 10.dp, bottom = 10.dp)
                        .size(103.dp, (103f * 9f / 16f).dp)
                        .clip(RoundedCornerShape(8.dp))
                        .background(Color(0xFF24242A))
                ) {
                    posterOf(item)?.invoke()
                }
                Column(
                    Modifier
                        .weight(1f)
                        .padding(horizontal = 12.dp, vertical = 10.dp)
                ) {
                    Text(item.title, fontSize = DesignTokens.fontRowTitle.sp,
                        fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
                        maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Spacer(Modifier.height(2.dp))
                    Text(relDays(item.addedSec), fontSize = 11.sp,
                        color = DesignTokens.textSub)
                    Spacer(Modifier.height(8.dp))
                    // 黑进度条 3pt（05 §三：整行玻璃卡规格）
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .height(3.dp)
                            .background(Color(0x1A000000))
                    ) {
                        Box(
                            Modifier
                                .fillMaxWidth(frac)
                                .height(3.dp)
                                .background(Color.Black)
                        )
                    }
                    Spacer(Modifier.height(4.dp))
                    Text("看到 " + fmtSeen(prog.positionMs) + " / " +
                        item.formatDuration(), fontSize = 10.sp,
                        color = DesignTokens.textSub)
                }
            }
        }
    }
}
/** 标准二级页导航条：左返回图标 + 中标题 + 右空（竞品 IMG_0429 的「我的」">" 下钻页同款）。 */
@Composable
private fun WatchingNavBar(onBack: () -> Unit) {
    Box(
        Modifier.fillMaxWidth().height(WatchingNavHeight)
            .padding(horizontal = DesignTokens.screenPad.dp),
        contentAlignment = Alignment.Center,
    ) {
        Box(
            Modifier.align(Alignment.CenterStart)
                .clickable(role = Role.Button, onClick = onBack)
                .semantics { contentDescription = "返回" },
        ) {
            HitBox { Text("‹", fontSize = 20.sp, color = DesignTokens.textMain) }
        }
        Text("在看", fontSize = 17.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
    }
}

// 44dp = 报告 §6.1 最小触控边长；DesignTokens 无对应项。
private val WatchingNavHeight = 44.dp
