package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.roundToInt

/**
 * 播放页底部区（player-ui-v2.md §3.2 + §3.3 字幕安全区）。
 *
 * **与 REX 的三处差异（都是刻意为之）**：
 * 1. 进度条**默认不显示滑块**，拖动时才出现 8pt 圆点 —— REX 完全无指示，
 *    用户不知道往哪按；圆点只在拖动时出现，不干扰观看。
 * 2. 底部渐变从**进度条上方 24pt** 开始 —— 保证字幕带（距底 24pt）
 *    与控制条**互不重叠**（本项目已踩过"控制条遮住字幕"的坑）。
 * 3. 拖动预览浮窗尺寸与描边对齐 05 号文档 §6.2（167×90pt，白描边 2pt，圆角 8pt）。
 */
@Composable
internal fun PlayerBottomBar(
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    thumbnailBitmap: Bitmap?,
    previewMs: Long,
    resolutionLabel: String,
    activeAction: String? = "",
    onPlayPause: () -> Unit,
    onNext: () -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onTextAction: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    var dragging by remember { mutableStateOf(false) }
    var dragFraction by remember { mutableFloatStateOf(0f) }
    val density = LocalDensity.current

    // 拖动中显示拖动目标时间；否则显示真实位置。
    val shownMs = if (dragging) (dragFraction * durationMs).roundToInt().toLong() else positionMs
    val fraction = if (durationMs > 0) (shownMs.toFloat() / durationMs).coerceIn(0f, 1f) else 0f

    // ---- 渐变背景层（独立于内容，固定高度）----
    //
    // 【为什么单独一层、且高度固定】
    // 第一版把渐变挂在内容 Box 上，渐变高度 = 内容高度（含顶部 24pt padding），
    // 实测时间行被顶到 405pt（参考是 301pt）—— 底栏吃掉了太多画面。
    // 第二版把内容 align(BottomCenter)，但渐变仍跟着内容走。
    // 现在：渐变是**固定高度**的一层（底部往上 96pt），内容贴底排布。
    // 参考实测：时间行中心在 301pt、屏幕高 390pt ⇒ 底栏占 89pt。
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
            // ---- 时间行 ----
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    text = formatPlayerTime(shownMs),
                    color = PlayerTokens.textPrimary,
                    fontSize = PlayerTokens.timeSize.sp,
                    lineHeight = PlayerTokens.timeSize.sp,
                    fontFamily = FontFamily.Monospace,
                    maxLines = 1,
                )
                Spacer(Modifier.weight(1f))
                Text(
                    text = formatPlayerTime(durationMs),
                    color = PlayerTokens.textPrimary,
                    fontSize = PlayerTokens.timeSize.sp,
                    lineHeight = PlayerTokens.timeSize.sp,
                    fontFamily = FontFamily.Monospace,
                    maxLines = 1,
                )
            }

            Spacer(Modifier.height(PlayerTokens.trackLabelGap.dp))

            // ---- 进度条 + 拖动预览 ----
            //
            // 【结构：两层 Box 叠放】
            // · 底层：进度条（24dp 高，含拖动手势）；
            // · 上层：拖动预览浮窗（**不占布局高度**，用 offset 向上挂在进度条上方）。
            //
            // 【为什么浮窗必须向上、且单独一层】
            // 第一版浮窗在进度条的同一个 Box 里、只 offset(x)，y=0 ⇒ 它从进度条顶部
            // **往下铺 90dp**，把下面的按钮行整个盖住。实机表现：
            // 底部文字只剩上半截（y=809 处被水平切断），且按钮点不到。
            // 放在独立的一层 + 向上偏移 ⇒ 既不盖按钮，也不增加底栏高度。
            BoxWithConstraints(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(24.dp),
            ) {
                // 预览浮窗（拖动中）：向上挂在进度条上方，不占布局高度
                val previewing = previewMs >= 0
                if (dragging || previewing) {
                    val shownFraction = if (dragging) dragFraction else {
                        if (durationMs > 0) (previewMs.toFloat() / durationMs).coerceIn(0f, 1f) else 0f
                    }
                    PlayerSeekPreview(
                        bitmap = thumbnailBitmap,
                        fraction = shownFraction,
                        containerWidth = maxWidth,
                    )
                }

                // 轨道
                PlayerSeekTrack(
                    fraction = fraction,
                    durationMs = durationMs,
                    dragging = dragging,
                    onScrub = { ms ->
                        dragFraction = if (durationMs > 0) {
                            (ms.toFloat() / durationMs).coerceIn(0f, 1f)
                        } else 0f
                        onScrub(ms)
                    },
                    onSeek = onSeek,
                    onDragStart = { dragging = true },
                    onDragEnd = {
                        dragging = false
                        if (durationMs > 0) {
                            onSeek((dragFraction * durationMs).roundToInt().toLong())
                        }
                    },
                )
            }

            // ---- 按钮行 ----
            //
            // 【为什么不能用固定 height】第一版给这一行写了 `.height(24.dp)`，
            // 而 17sp 文字的**行盒**是 24sp(=66px) —— 行盒比容器高，
            // Compose 就把文字**裁掉**。实测：字只剩 y=1009..1029（21px，
            // 而 17sp 汉字应有 ~40px）。
            // **被裁的文字连命中区一起被裁** ⇒ "倍速"永远点不到
            // （第一版实机：点"倍速"无反应、点击穿透到进度条）。
            //
            // 现在：行高由内容决定（defaultMinSize 只保底，不封顶）。
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

                // 右侧文字按钮（4K / 选集 / 倍速）
                Row(
                    horizontalArrangement = Arrangement.spacedBy(PlayerTokens.textButtonGap.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    if (resolutionLabel.isNotBlank()) {
                        Text(
                            text = resolutionLabel,
                            color = if (activeAction == "quality") PlayerTokens.activeAccent else PlayerTokens.textPrimary,
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
                        color = if (activeAction == "episodes") PlayerTokens.activeAccent else PlayerTokens.textPrimary,
                        fontSize = PlayerTokens.textButtonSize.sp,
                        lineHeight = PlayerTokens.textButtonSize.sp,
                        maxLines = 1,
                        modifier = Modifier
                            .clickable { onTextAction("episodes") }
                            .padding(horizontal = 4.dp),
                    )
                    Text(
                        text = "倍速",
                        color = if (activeAction == "speed") PlayerTokens.activeAccent else PlayerTokens.textPrimary,
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

/** 时间格式化：`h:mm:ss`（超过一小时才带小时位）。 */
internal fun formatPlayerTime(ms: Long): String {
    val totalSec = (ms / 1000).coerceAtLeast(0)
    val h = totalSec / 3600
    val m = (totalSec % 3600) / 60
    val s = totalSec % 60
    return if (h > 0) String.format("%d:%02d:%02d", h, m, s) else String.format("%d:%02d", m, s)
}
