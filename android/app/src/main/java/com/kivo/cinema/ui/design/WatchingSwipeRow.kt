package com.kivo.cinema.ui.design

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.tween
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalViewConfiguration
import androidx.compose.ui.platform.ViewConfiguration
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.WatchingRow
import kotlinx.coroutines.launch
import kotlin.math.roundToInt

/** 左滑露出的删除按钮宽度；拖过 45% 吸附展开，否则回弹。 */
private val RevealWidth = 84.dp
private const val SnapFraction = 0.45f
private const val AnimMs = 180
private const val LongPressMs = 2000L

/**
 * 在看行的两种移除通道：
 * - **向左滑**（iOS 习惯）→ 右侧露出删除按钮（单条快捷）；
 * - 长按 2 秒 → 进入多选模式（由页面接管勾选/批量移除）。
 *
 * 卡片视觉与旧版逐项一致；多选模式只在最左侧插入勾选圈。
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
fun WatchingSwipeRow(
    row: WatchingRow,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    selectionMode: Boolean,
    selected: Boolean,
    onClick: () -> Unit,
    onToggle: () -> Unit,
    onEnterSelection: () -> Unit,
    onDeleteClick: () -> Unit,
) {
    val scope = rememberCoroutineScope()
    val maxReveal = with(LocalDensity.current) { RevealWidth.toPx() }
    // 0 = 收起；-maxReveal = 左滑露出删除。
    val offsetX = remember { Animatable(0f) }
    // 默认长按 500ms 太容易误触；这里只放宽到 2s，其余手势参数沿用系统。
    val baseConfig = LocalViewConfiguration.current
    val longPressConfig = remember(baseConfig) {
        object : ViewConfiguration by baseConfig {
            override val longPressTimeoutMillis: Long = LongPressMs
        }
    }

    fun collapse() { scope.launch { offsetX.animateTo(0f, tween(AnimMs)) } }

    Box(
        Modifier.fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp, vertical = DesignTokens.gap.dp)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp)),
    ) {
        if (!selectionMode) {
            // iOS trailing swipe action：红块在底层通栏铺满整行高度，卡片盖其上左滑露出。
            // matchParentSize 而非 fillMaxHeight——后者在无固定高度的父 Box 里会塌成内容高度。
            Box(Modifier.matchParentSize()) {
                Box(
                    Modifier.align(Alignment.CenterEnd)
                        .width(RevealWidth)
                        .fillMaxHeight()
                        .background(DesignTokens.dangerRed)
                        .clickable(role = Role.Button, onClick = { collapse(); onDeleteClick() }),
                    contentAlignment = Alignment.Center,
                ) {
                    TrashIcon(
                        tint = DesignTokens.pageBackground,
                        cutout = DesignTokens.dangerRed,
                        modifier = Modifier.size(26.dp),
                    )
                }
            }
        }
        CompositionLocalProvider(LocalViewConfiguration provides longPressConfig) {
            Box(
                Modifier.fillMaxWidth()
                    .offset { IntOffset(offsetX.value.roundToInt(), 0) }
                    .pointerInput(selectionMode) {
                        if (!selectionMode) {
                            detectHorizontalDragGestures(
                                onDragEnd = {
                                    scope.launch {
                                        val target = if (offsetX.value < -maxReveal * SnapFraction) {
                                            -maxReveal
                                        } else 0f
                                        offsetX.animateTo(target, tween(AnimMs))
                                    }
                                },
                            ) { _, dragAmount ->
                                scope.launch {
                                    offsetX.snapTo((offsetX.value + dragAmount).coerceIn(-maxReveal, 0f))
                                }
                            }
                        }
                    }
                    .combinedClickable(
                        role = Role.Button,
                        onLongClickLabel = "进入选择模式",
                        onLongClick = if (selectionMode) null else {
                            { collapse(); onEnterSelection() }
                        },
                        onClick = {
                            when {
                                selectionMode -> onToggle()
                                offsetX.value < -1f -> collapse()
                                else -> onClick()
                            }
                        },
                    )
                    .semantics {
                        contentDescription = if (selectionMode) {
                            "${if (selected) "已选" else "未选"}，${row.title}"
                        } else "${row.title}，${metaOf(row)}"
                    }
                    .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
                    .background(DesignTokens.canvasFill),
            ) {
                WatchingCardContent(row, posterOf, selectionMode, selected)
            }
        }
    }
}

/** 整行卡内容：勾选圈（仅多选态）+ 左缩略图 103pt + 右标题/元信息/黑进度条。 */
@Composable
private fun WatchingCardContent(
    row: WatchingRow,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    selectionMode: Boolean,
    selected: Boolean,
) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        if (selectionMode) {
            SelectionCircle(selected)
            Spacer(Modifier.width(10.dp))
        }
        Box(
            Modifier.padding(start = 10.dp, top = 10.dp, bottom = 10.dp)
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

/** 未选 = 系统灰底 + 极细描边；已选 = iOS 蓝实底 + 白色对勾。 */
@Composable
private fun SelectionCircle(checked: Boolean) {
    Box(
        Modifier.size(22.dp)
            .clip(CircleShape)
            .background(if (checked) DesignTokens.navActive else DesignTokens.groupedFill)
            .then(
                if (!checked) Modifier.border(1.dp, DesignTokens.hairline, CircleShape)
                else Modifier,
            ),
        contentAlignment = Alignment.Center,
    ) {
        if (checked) CheckMarkIcon(DesignTokens.pageBackground, Modifier.size(13.dp))
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

private fun fmtDuration(ms: Long): String {
    if (ms <= 0L) return "--:--"
    val s = ms / 1000L
    val h = s / 3600
    val m = (s % 3600) / 60
    val sec = s % 60
    return if (h > 0) "%d:%02d:%02d".format(h, m, sec) else "%d:%02d".format(m, sec)
}
