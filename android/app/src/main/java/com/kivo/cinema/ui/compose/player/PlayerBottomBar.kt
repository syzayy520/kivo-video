package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.roundToInt

/**
 * 播放页底部区（player-ui-v2.md §3.2）。
 *
 * ## 布局：与参考一致的**两行**
 * ```
 *   [00:08] ──────────── [45:28]     ← 时间与进度条**同一行**
 *   ⏸  ⏭                    1080P 选集 倍速
 * ```
 * 第一版把时间单独占一行（下面再放进度条），比参考多一行、也更松散。
 *
 * ## 与参考的差异（都是刻意的）
 * 1. 进度条**默认不显示滑块**，拖动时才出现 8pt 小圆点 ——
 *    参考完全没有滑块，用户不知道往哪按。
 * 2. 底部渐变从**进度条上方**开始 —— 保证字幕带与控制条互不重叠。
 */
@Composable
internal fun PlayerBottomBar(
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    thumbnailBitmap: Bitmap?,
    previewMs: Long,
    resolutionLabel: String,
    activeAction: String?,
    onPlayPause: () -> Unit,
    onNext: () -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onTextAction: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    // 拖动目标时刻（REX：拖动中左端实时显示"会跳到哪"）。
    // 非空 ⇒ 左端时间显示目标；为空 ⇒ 显示当前位置。
    var dragTargetMs by remember { mutableStateOf<Long?>(null) }

    // 渐变层与内容层**平级**：内容不设高度 ⇒ 命中测试在它自己的范围内，
    // 固定高度的容器会把超出的按钮裁掉（第一版实机：底部文字只剩上半、点不到）。
    Box(modifier = modifier.fillMaxWidth()) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(PlayerTokens.bottomScrimHeight.dp)
                .align(Alignment.BottomCenter)
                .background(
                    Brush.verticalGradient(
                        listOf(PlayerTokens.bottomScrimTop, PlayerTokens.bottomScrimBottom),
                    ),
                ),
        )

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.BottomCenter)
                .padding(
                    start = PlayerTokens.bottomSidePad.dp,
                    end = PlayerTokens.bottomSidePad.dp,
                    bottom = 12.dp,
                ),
        ) {
            // ---- 时间轴行：时间 │ 进度条 │ 总长（参考的结构）----
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = formatPlayerTime(dragTargetMs ?: positionMs),
                    color = if (dragTargetMs != null) {
                        PlayerTokens.activeAccent
                    } else {
                        PlayerTokens.textPrimary
                    },
                    fontSize = PlayerTokens.timeSize.sp,
                    lineHeight = PlayerTokens.timeSize.sp,
                    fontFamily = FontFamily.Monospace,
                    maxLines = 1,
                )
                Spacer(Modifier.width(PlayerTokens.trackSideGap.dp))
                PlayerSeekTrack(
                    positionMs = positionMs,
                    durationMs = durationMs,
                    thumbnailBitmap = thumbnailBitmap,
                    previewMs = previewMs,
                    onDragTarget = { dragTargetMs = it },
                    onScrub = onScrub,
                    onSeek = onSeek,
                    modifier = Modifier.weight(1f),
                )
                Spacer(Modifier.width(PlayerTokens.trackSideGap.dp))
                Text(
                    text = formatPlayerTime(durationMs),
                    color = PlayerTokens.textPrimary,
                    fontSize = PlayerTokens.timeSize.sp,
                    lineHeight = PlayerTokens.timeSize.sp,
                    fontFamily = FontFamily.Monospace,
                    maxLines = 1,
                )
            }

            Spacer(Modifier.height(PlayerTokens.buttonRowGap.dp))

            // ---- 按钮行 ----
            //
            // 【为什么不能用固定 height】17sp 文字的**行盒**是 24sp(=66px)，
            // 容器比行盒矮 ⇒ Compose 把文字裁掉（实测只剩 21px），
            // **被裁的文字连命中区一起被裁** ⇒ 按钮永远点不到。
            // 行高改为 defaultMinSize（只保底、不封顶）。
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .defaultMinSize(minHeight = PlayerTokens.buttonRowHeight.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                PlayerGlyph(
                    kind = if (isPlaying) "pause" else "play",
                    color = PlayerTokens.textPrimary,
                    modifier = Modifier
                        .size(PlayerTokens.actionIconSize.dp)
                        .clickable(onClick = onPlayPause),
                )
                Spacer(Modifier.width(PlayerTokens.buttonGap.dp - PlayerTokens.actionIconSize.dp))
                PlayerGlyph(
                    kind = "next",
                    color = PlayerTokens.textPrimary,
                    modifier = Modifier
                        .size(PlayerTokens.actionIconSize.dp)
                        .clickable(onClick = onNext),
                )

                Spacer(Modifier.weight(1f))

                // 右侧文字按钮（画质 / 选集 / 倍速）
                Row(
                    horizontalArrangement = Arrangement.spacedBy(PlayerTokens.textButtonGap.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    if (resolutionLabel.isNotBlank()) {
                        Text(
                            text = resolutionLabel,
                            color = if (activeAction == "quality") {
                                PlayerTokens.activeAccent
                            } else {
                                PlayerTokens.textPrimary
                            },
                            fontSize = PlayerTokens.textButtonSize.sp,
                            lineHeight = PlayerTokens.textButtonSize.sp,
                            maxLines = 1,
                            modifier = Modifier
                                .clickable { onTextAction("quality") }
                                .padding(horizontal = 4.dp),
                        )
                    }
                    Text(
                        text = "选集",
                        color = if (activeAction == "episodes") {
                            PlayerTokens.activeAccent
                        } else {
                            PlayerTokens.textPrimary
                        },
                        fontSize = PlayerTokens.textButtonSize.sp,
                        lineHeight = PlayerTokens.textButtonSize.sp,
                        maxLines = 1,
                        modifier = Modifier
                            .clickable { onTextAction("episodes") }
                            .padding(horizontal = 4.dp),
                    )
                    Text(
                        text = "倍速",
                        color = if (activeAction == "speed") {
                            PlayerTokens.activeAccent
                        } else {
                            PlayerTokens.textPrimary
                        },
                        fontSize = PlayerTokens.textButtonSize.sp,
                        lineHeight = PlayerTokens.textButtonSize.sp,
                        maxLines = 1,
                        modifier = Modifier
                            .clickable { onTextAction("speed") }
                            .padding(horizontal = 4.dp),
                    )
                }
            }
        }
    }
}

/**
 * 时间格式化：`mm:ss`（不足 1 小时）或 `h:mm:ss`。
 *
 * 【为什么分钟要补零】参考的显示是 `00:08` / `45:28` —— 两位补零。
 * 第一版写的是 `0:08`，跟参考不一致，看着像"没对齐"。
 */
internal fun formatPlayerTime(ms: Long): String {
    val totalSec = (ms / 1000).coerceAtLeast(0)
    val h = totalSec / 3600
    val m = (totalSec % 3600) / 60
    val s = totalSec % 60
    return if (h > 0) {
        String.format("%d:%02d:%02d", h, m, s)
    } else {
        String.format("%02d:%02d", m, s)
    }
}
