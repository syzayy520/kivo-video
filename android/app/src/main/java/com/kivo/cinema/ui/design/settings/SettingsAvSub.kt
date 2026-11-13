package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.playback.PlaybackPrefs
import com.kivo.cinema.ui.design.GlassSurface

/**
 * 音画设置子页（263 音画卡，参考「音画」2 帧：内核 radio 卡 / 画面 / 音频 / 画质增强）。
 * 有内核后端的能力 = 真控件（音频首选语言 alang / 单独调整播放亮度）；
 * 无后端的能力 = 置灰带非空原因（内核切换/起播方向/适配/HDR/音频引擎/SDR↔HDR）。
 * 本页只编辑策略，不直接操作内核 —— 策略归 playback 域，播放器是消费方。
 */
@Composable
internal fun SettingsAvSub(onBack: () -> Unit) {
    val context = LocalContext.current
    val prefs = remember { PlaybackPrefs(context) }
    var revision by remember { mutableIntStateOf(0) }
    var picker by remember { mutableStateOf<String?>(null) }

    val independentBrightness = remember(revision) { prefs.independentBrightness }
    val audioLanguage = remember(revision) { prefs.audioLanguage }

    SubPageScaffold("音画", onBack) {
        // 内核 radio 卡：引擎切换尚未产品化 ⇒ 三行置灰，✓ 标当前 mpv。
        GlassSurface(Modifier.fillMaxWidth()) {
            Column {
                RadioRow("mpv 内核", true, enabled = false,
                    subtitle = "App 自带播放内核，字幕、弹幕、播放控制完整，推荐默认。") {}
                RowDivider()
                RadioRow("原生硬解内核", false, enabled = false,
                    subtitle = "MediaCodec 硬解 + AAudio，当前用于本地片源。") {}
                RowDivider()
                RadioRow("实验内核", false, beta = true, enabled = false,
                    subtitle = "天空声道等实验引擎尚未接入。") {}
            }
        }
        Footnote("内核切换尚未开放，当前固定 mpv 内核；开放后切换需重新进入播放器生效。")
        Spacer(Modifier.height(14.dp))

        SectionCard("画面") {
            StepperRow("视频起播方向", "自适应", enabled = false,
                subtitle = "起播方向固定横屏，自适应尚未接入") {}
            RowDivider()
            StepperRow("画面适配方式", "适应屏幕", enabled = false,
                subtitle = "适配模式尚未接入，比例 override 在播放面板") {}
            RowDivider()
            SwitchRow("单独调整播放亮度", independentBrightness,
                subtitle = "播放时使用独立亮度；返回桌面或结束播放后，恢复原来的屏幕亮度。",
                icon = SettingsIcons.theme, iconColor = SettingsVisuals.orange) {
                prefs.independentBrightness = it; revision++
            }
            RowDivider()
            StepperRow("HDR 动态映射", "自动", enabled = false,
                subtitle = "HDR 逐帧映射尚未实现") {}
            RowDivider()
            StepperRow("HDR 增强层", "自动", enabled = false,
                subtitle = "HDR 逐帧增强尚未实现") {}
        }
        Footnote("HDR 动态映射与增强层需要逐帧处理，播放时更耗电、更易发热。")
        Spacer(Modifier.height(14.dp))

        SectionCard("音频") {
            StepperRow("音频引擎", "标准音频引擎", enabled = false,
                subtitle = "音频引擎随内核选择，暂无独立开关") {}
            RowDivider()
            StepperRow("音频首选语言", PlaybackPrefs.audioLanguageLabel(audioLanguage),
                icon = SettingsIcons.speaker, iconColor = SettingsVisuals.purple) { picker = "alang" }
        }
        Spacer(Modifier.height(14.dp))

        SectionCard("画质增强") {
            SwitchRow("SDR 转 HDR", false, beta = true, enabled = false,
                subtitle = "SDR 转 HDR 增强尚未实现") {}
        }
        Footnote("SDR 转 HDR 需逐帧处理与 HDR 屏幕；尚未实现，当前按兼容画面播放。")
        Spacer(Modifier.height(14.dp))

        GlassSurface(Modifier.fillMaxWidth()) {
            Column {
                SwitchRow("HDR 转 SDR", false, beta = true, enabled = false,
                    subtitle = "HDR 转 SDR 转换尚未实现") {}
            }
        }
        Footnote("尝试以 SDR 显示 HDR 视频；转换尚未实现，当前按兼容画面播放。")
    }

    when (picker) {
        "alang" -> SettingsPicker(true, "音频首选语言",
            listOf(PickerOption("media", "媒体默认")) +
                PlaybackPrefs.LANGUAGES.map { PickerOption(it.code, it.label) },
            audioLanguage, { prefs.audioLanguage = it; revision++ }, { picker = null })
    }
}