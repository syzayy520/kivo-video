package com.kivo.cinema.privacy

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.keyframes
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 数字键盘 + 圆点显示（39 卡）。单一职责：纯输入件——不持有密码、不做校验，只回调。
 * 颜色取自 MaterialTheme，暗色（私密区）与亮色（资源库门弹窗）皆适配。
 * 过渡：按键按压回弹、错误左右抖动（铁律：不做瞬现瞬消）。
 */
@Composable
fun PrivacyKeypad(
    entered: String,
    maxLength: Int,
    error: Boolean,
    shakeTick: Int,
    onDigit: (Char) -> Unit,
    onDelete: () -> Unit,
    onSubmit: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val ink = MaterialTheme.colorScheme.onSurface
    val shake = remember { Animatable(0f) }
    LaunchedEffect(shakeTick) {
        if (shakeTick > 0) shake.animateTo(0f, keyframes {
            durationMillis = 320
            0f at 0; -12f at 60; 12f at 120; -8f at 180; 8f at 240; 0f at 320
        })
    }
    Column(modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
        Row(
            Modifier.offset(x = shake.value.dp).height(28.dp),
            horizontalArrangement = Arrangement.spacedBy(18.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            repeat(maxLength) { index ->
                val filled = index < entered.length
                val base = if (error) MaterialTheme.colorScheme.error else ink
                Box(
                    Modifier.size(if (filled) 14.dp else 12.dp).clip(CircleShape)
                        .background(if (filled) base else base.copy(alpha = 0.22f))
                )
            }
        }
        Spacer(Modifier.height(28.dp))
        val rows = listOf(listOf('1', '2', '3'), listOf('4', '5', '6'), listOf('7', '8', '9'))
        rows.forEach { row ->
            Row(horizontalArrangement = Arrangement.spacedBy(28.dp)) {
                row.forEach { digit -> Key(digit.toString(), ink) { onDigit(digit) } }
            }
            Spacer(Modifier.height(16.dp))
        }
        Row(horizontalArrangement = Arrangement.spacedBy(28.dp), verticalAlignment = Alignment.CenterVertically) {
            Key("⌫", ink) { onDelete() }
            Key("0", ink) { onDigit('0') }
            Key("✓", ink, enabled = entered.length >= 4) { onSubmit() }
        }
    }
}

@Composable
private fun Key(label: String, ink: androidx.compose.ui.graphics.Color, enabled: Boolean = true, onClick: () -> Unit) {
    val interaction = remember { MutableInteractionSource() }
    val pressed by interaction.collectIsPressedAsState()
    val scale by animateFloatAsState(if (pressed) 0.9f else 1f, tween(110), label = "keyScale")
    Box(
        Modifier.size(72.dp).scale(scale).clip(CircleShape)
            .background(ink.copy(alpha = if (pressed) 0.18f else 0.06f))
            .clickable(interaction, indication = null, enabled = enabled, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Text(label, fontSize = if (label == "✓" || label == "⌫") 24.sp else 27.sp,
            fontWeight = FontWeight.Medium, color = if (enabled) ink else ink.copy(alpha = 0.28f))
    }
}