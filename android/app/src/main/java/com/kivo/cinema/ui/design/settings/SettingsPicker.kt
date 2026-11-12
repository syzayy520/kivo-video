package com.kivo.cinema.ui.design.settings

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.EaseOut
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.scaleIn
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Popup
import androidx.compose.ui.window.PopupProperties
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 步进行弹出的选择菜单（262 播放设置卡）：白色圆角浮卡 + ✓ 当前项 + 长列表可滚 + 软阴影。
 * 参考形态：点 stepper 值 → 浮出菜单，选中即收。结构照搬、形态按 Android 重画（居中浮卡）。
 * 只负责"画菜单 + 回传选择"，不持有任何播放策略 —— 策略归 PlaybackPrefs。
 */
internal data class PickerOption(val code: String, val label: String)

@Composable
internal fun SettingsPicker(
    visible: Boolean,
    title: String,
    options: List<PickerOption>,
    selected: String,
    onSelect: (String) -> Unit,
    onDismiss: () -> Unit,
) {
    if (!visible) return
    var shown by remember { mutableStateOf(false) }
    LaunchedEffect(Unit) { shown = true }
    Popup(
        alignment = Alignment.Center,
        onDismissRequest = onDismiss,
        properties = PopupProperties(focusable = true),
    ) {
        AnimatedVisibility(
            visible = shown,
            enter = fadeIn(tween(180, easing = EaseOut)) +
                scaleIn(tween(180, easing = EaseOut), initialScale = 0.92f),
        ) {
            val shape = RoundedCornerShape(14.dp)
            Column(
                Modifier.widthIn(min = 240.dp, max = 300.dp).heightIn(max = 420.dp)
                    .shadow(12.dp, shape).clip(shape).background(Color.White),
            ) {
                if (title.isNotBlank()) {
                    Text(
                        title, fontSize = 13.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textSub,
                        modifier = Modifier.padding(start = 16.dp, top = 12.dp, bottom = 4.dp),
                    )
                }
                Column(Modifier.verticalScroll(rememberScrollState())) {
                    options.forEachIndexed { index, option ->
                        if (index > 0)
                            Box(Modifier.fillMaxWidth().padding(start = 16.dp).height(0.5.dp).background(DesignTokens.hairline))
                        Row(
                            Modifier.fillMaxWidth()
                                .clickable { onSelect(option.code); onDismiss() }
                                .padding(horizontal = 16.dp, vertical = 13.dp),
                            verticalAlignment = Alignment.CenterVertically,
                        ) {
                            Text(
                                option.label, fontSize = 16.sp, color = DesignTokens.textMain,
                                modifier = Modifier.weight(1f),
                            )
                            if (option.code == selected)
                                Text("✓", fontSize = 18.sp, fontWeight = FontWeight.Bold, color = Color(0xFF007AFF))
                        }
                    }
                }
            }
        }
    }
}