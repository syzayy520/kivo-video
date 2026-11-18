package com.kivo.cinema.ui.design

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.WatchingGrouping
import com.kivo.cinema.library.WatchingRow
import com.kivo.cinema.ui.components.HitBox

/**
 * 在看页：**全部来源的观看进度总账**（本地 + NAS + Emby）。
 *
 * 规格（AppWatching 147 键语义 / 05 §三）：整行卡 = 左缩略图 103pt 16:9 +
 * 右标题 17pt 粗 / 元信息 11pt 灰 / 黑进度条 3pt。
 *
 * 三处打磨：
 * 1. **同一部剧合成一行**（[WatchingGrouping]）：连看 5 集不再刷 5 行，代表 = 最近那集；
 * 2. **LazyColumn**：记录上百条时不再一次性组合，滑动不卡；
 * 3. **长按移除**：不想再看到某条时能自己清掉（只删进度，不动视频文件、不动每日统计）。
 */
@Composable
fun WatchingPage(
    continueItems: List<HomeEntry>,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpenItem: (HomeEntry) -> Unit,
    onRemove: (WatchingRow) -> Unit,
    onBack: () -> Unit,
) {
    BackHandler { onBack() }
    val rows = remember(continueItems) { WatchingGrouping.rows(continueItems) }
    var pendingRemove by remember { mutableStateOf<WatchingRow?>(null) }

    Column(Modifier.fillMaxSize()) {
        Spacer(Modifier.height(6.dp))
        WatchingNavBar(onBack)
        Spacer(Modifier.height(4.dp))
        Text(
            if (rows.isEmpty()) "观看进度自动保存" else "观看进度自动保存 · 长按可移除记录",
            fontSize = 11.sp, color = DesignTokens.textSub,
            modifier = Modifier.padding(horizontal = DesignTokens.screenPad.dp),
        )
        Spacer(Modifier.height(12.dp))
        if (rows.isEmpty()) {
            EmptyWatching(onBack)
        } else {
            LazyColumn(Modifier.fillMaxWidth().weight(1f)) {
                items(rows, key = { it.latest.key }) { row ->
                    WatchingCardRow(
                        row = row,
                        posterOf = posterOf,
                        onClick = { onOpenItem(row.latest) },
                        onLongClick = { pendingRemove = row },
                    )
                }
            }
        }
    }

    pendingRemove?.let { row ->
        AlertDialog(
            onDismissRequest = { pendingRemove = null },
            title = { Text("移除这条记录？", fontSize = 17.sp, fontWeight = FontWeight.SemiBold) },
            text = {
                Text(
                    "「${row.title}」的观看进度会从这里移除；视频文件与观看统计都不受影响。",
                    fontSize = 14.sp, lineHeight = 20.sp,
                )
            },
            confirmButton = {
                TextButton(onClick = { onRemove(row); pendingRemove = null }) { Text("移除") }
            },
            dismissButton = { TextButton(onClick = { pendingRemove = null }) { Text("取消") } },
        )
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun WatchingCardRow(
    row: WatchingRow,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onClick: () -> Unit,
    onLongClick: () -> Unit,
) {
    Row(
        Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp, vertical = DesignTokens.gap.dp)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(Color.White.copy(alpha = 0.92f))
            .combinedClickable(
                role = Role.Button,
                onLongClickLabel = "移除记录",
                onLongClick = onLongClick,
                onClick = onClick,
            )
            .semantics { contentDescription = "${row.title}，${metaOf(row)}" },
    ) {
        Box(
            Modifier
                .padding(start = 10.dp, top = 10.dp, bottom = 10.dp)
                .size(103.dp, (103f * 9f / 16f).dp)
                .clip(RoundedCornerShape(8.dp))
                .background(Color(0xFF24242A)),
        ) {
            posterOf(row.latest)?.invoke()
        }
        Column(Modifier.weight(1f).padding(horizontal = 12.dp, vertical = 10.dp)) {
            Text(
                row.title, fontSize = DesignTokens.fontRowTitle.sp,
                fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
                maxLines = 1, overflow = TextOverflow.Ellipsis,
            )
            Spacer(Modifier.height(2.dp))
            Text(metaOf(row), fontSize = 11.sp, color = DesignTokens.textSub, maxLines = 1)
            Spacer(Modifier.height(8.dp))
            // 黑进度条 3pt（05 §三：整行玻璃卡规格）
            Box(Modifier.fillMaxWidth().height(3.dp).background(Color(0x1A000000))) {
                Box(Modifier.fillMaxWidth(row.progress).height(3.dp).background(Color.Black))
            }
            Spacer(Modifier.height(4.dp))
            Text(
                "看到 " + fmtSeen(row.positionMs) + " / " + fmtDuration(row.durationMs) +
                    sinceOf(row.latest),
                fontSize = 10.sp, color = DesignTokens.textSub,
            )
        }
    }
}

/** `S02E03 · 共 5 集 · NAS`；单片只显示来源。 */
private fun metaOf(row: WatchingRow): String = buildList {
    row.episodeLabel?.let { add(it) }
    if (row.isSeries) add("共 ${row.entryCount} 集")
    add(row.sourceLabel)
}.joinToString(" · ")

/** ` · 3 小时前`；时间未知则留空（不编造）。 */
private fun sinceOf(e: HomeEntry): String {
    val sec = e.lastPlayedMs / 1000
    return if (sec > 0) " · " + relDays(sec) else ""
}

@Composable
private fun EmptyWatching(onBack: () -> Unit) {
    Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                "还没有在看记录\n去发现页选一部片，进度会出现在这里",
                fontSize = 14.sp, color = DesignTokens.textSub, lineHeight = 22.sp,
                textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(14.dp))
            Box(
                Modifier.clip(RoundedCornerShape(50.dp))
                    .background(DesignTokens.navActive)
                    .clickable(role = Role.Button, onClick = onBack)
                    .padding(horizontal = 22.dp, vertical = 9.dp),
            ) {
                Text("去发现", color = Color.White, fontSize = 14.sp,
                    fontWeight = FontWeight.Medium)
            }
        }
    }
}

/** 标准二级页导航条：左返回图标 + 中标题 + 右空。 */
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
        Text("在看", fontSize = 17.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
    }
}

// 44dp = 报告 §6.1 最小触控边长；DesignTokens 无对应项。
private val WatchingNavHeight = 44.dp

private fun fmtDuration(ms: Long): String {
    if (ms <= 0L) return "--:--"
    val s = ms / 1000L
    val h = s / 3600
    val m = (s % 3600) / 60
    val sec = s % 60
    return if (h > 0) "%d:%02d:%02d".format(h, m, sec) else "%d:%02d".format(m, sec)
}
