package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.AppearancePrefs
import com.kivo.cinema.ui.design.home.HomePreferences

/**
 * 外观与交互·反馈族四卡（267）：预告片/家长指南/操作反馈/图标。
 * 自动播放与根页首页卡同键同状态（MainActivity 持有态经入参传入）；声音写 home_display。
 */
@Composable
internal fun AppearanceFeedbackCards(
    previewEnabled: Boolean,
    onPreviewChanged: (Boolean) -> Unit,
    push: (AppearanceDrill) -> Unit,
) {
    val context = LocalContext.current
    val homePrefs = remember { HomePreferences(context) }
    var sound by remember { mutableStateOf(homePrefs.previewSound) }
    var hapticPicker by remember { mutableStateOf(false) }
    SectionCard("预告片") {
        SwitchRow("自动播放", previewEnabled, onChange = onPreviewChanged)
        RowDivider()
        SwitchRow("播放时开启声音", sound, onChange = { sound = it; homePrefs.previewSound = it })
        RowDivider()
        SwitchRow("显示全部预告片", false, enabled = false,
            subtitle = "预告片列表 UI 未接入，首页 hero 单条。", onChange = {})
    }
    Spacer(Modifier.height(14.dp))
    SectionCard("家长指南") {
        SwitchRow("显示内容提示", AppearancePrefs.showContentNotes,
            subtitle = "查看性与裸露、暴力、粗俗语言、烟酒与药物、惊悚场面的分级与内容描述符摘要，作为观影参考。",
            onChange = { AppearancePrefs.updateShowContentNotes(it) })
    }
    Spacer(Modifier.height(14.dp))
    SectionCard("操作反馈") {
        StepperRow("触感反馈", hapticLabel(AppearancePrefs.hapticMode)) { hapticPicker = true }
    }
    Footnote("自动模式将在低电量时自动关闭。")
    Spacer(Modifier.height(14.dp))
    SectionCard("图标") {
        ChevronRow("图标库", subtitle = "图标库未接入，保持默认线框图标。", enabled = false) {}
        RowDivider()
        ChevronRow("资源卡片 Badge", value = "未配置") { push(AppearanceDrill.BADGE) }
    }
    SettingsPicker(
        visible = hapticPicker, title = "触感反馈",
        options = listOf(
            PickerOption("always", "始终开启"),
            PickerOption("auto", "自动"),
            PickerOption("disabled", "禁用"),
        ),
        selected = AppearancePrefs.hapticMode,
        onSelect = { AppearancePrefs.updateHapticMode(it) },
        onDismiss = { hapticPicker = false },
    )
}

private fun hapticLabel(code: String): String = when (code) {
    "always" -> "始终开启"
    "disabled" -> "禁用"
    else -> "自动"
}