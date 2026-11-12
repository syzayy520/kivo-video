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
 * 操控设置子页（264 操控卡，参考「操控」2 帧：按钮 / 手势 / 双击 / 长按快进）。
 * 有后端的能力 = 真控件（读写 PlaybackPrefs，播放器消费）；
 * 无后端的能力 = 置灰带非空原因（旋转/转屏钮/按钮自定义/应用内小窗/快进退钮/捏合缩放）。
 * 本页只编辑策略，不直接操作播放器 —— 策略归 playback 域，播放器是消费方。
 */
@Composable
internal fun SettingsControlsSub(onBack: () -> Unit) {
    val context = LocalContext.current
    val prefs = remember { PlaybackPrefs(context) }
    var revision by remember { mutableIntStateOf(0) }
    var picker by remember { mutableStateOf<String?>(null) }

    val showNext = remember(revision) { prefs.showNextButton }
    val showCapture = remember(revision) { prefs.showCaptureButton }
    val seekFwd = remember(revision) { prefs.seekForwardSeconds }
    val seekBack = remember(revision) { prefs.seekBackwardSeconds }
    val centerAction = remember(revision) { prefs.doubleTapCenterAction }
    val sidesAction = remember(revision) { prefs.doubleTapSidesAction }
    val verticalSwipe = remember(revision) { prefs.verticalSwipeEnabled }
    val lockAllow = remember(revision) { prefs.lockAllowGestures }
    val defaultSpeed = remember(revision) { prefs.defaultSpeed }
    val longPress = remember(revision) { prefs.longPressEnabled }
    val longBase = remember(revision) { prefs.longPressBaseSpeed }
    val swipeSpeed = remember(revision) { prefs.longPressSwipeSpeed }
    val swipeLock = remember(revision) { prefs.longPressSwipeLock }
    val restore = remember(revision) { prefs.longPressRestore }

    SubPageScaffold("操控", onBack) {
        // 组1（无节头卡）：旋转跟随 / 转屏按钮 —— 起播固定横屏，两者均无后端 ⇒ 置灰。
        GlassSurface(Modifier.fillMaxWidth()) {
            Column {
                SwitchRow("跟随设备旋转", false, enabled = false,
                    subtitle = "起播固定横屏，旋转跟随尚未接入") {}
                RowDivider()
                SwitchRow("显示转屏按钮", false, enabled = false,
                    subtitle = "转屏按钮尚未接入（固定横屏）") {}
            }
        }
        Spacer(Modifier.height(14.dp))

        SectionCard("播放器按钮") {
            ChevronRow("播放器按钮", "按钮显隐自定义尚未接入", enabled = false) {}
            RowDivider()
            SwitchRow("App 内画中画", false, enabled = false,
                subtitle = "应用内小窗尚未接入；系统画中画见「播放」设置") {}
            RowDivider()
            SwitchRow("下一集 按钮", showNext, subtitle = "剧集切换尚未接入，仅控制按钮显隐") {
                prefs.showNextButton = it; revision++
            }
            RowDivider()
            SwitchRow("快进与快退按钮", false, enabled = false,
                subtitle = "快进/快退按钮尚未接入，当前用双击两侧") {}
            RowDivider()
            SwitchRow("截图 按钮", showCapture) { prefs.showCaptureButton = it; revision++ }
            RowDivider()
            StepperRow("快进时长", PlaybackPrefs.seekLabel(seekFwd)) { picker = "seekFwd" }
            RowDivider()
            StepperRow("快退时长", PlaybackPrefs.seekLabel(seekBack)) { picker = "seekBack" }
        }
        Spacer(Modifier.height(14.dp))

        SectionCard("手势操作") {
            StepperRow("双击中间触发", PlaybackPrefs.doubleTapCenterLabel(centerAction)) { picker = "center" }
            RowDivider()
            StepperRow("双击两侧触发", PlaybackPrefs.doubleTapSidesLabel(sidesAction)) { picker = "sides" }
            RowDivider()
            SwitchRow("全屏上下滑动调整音量和亮度", verticalSwipe) { prefs.verticalSwipeEnabled = it; revision++ }
            RowDivider()
            SwitchRow("双指缩放画面", false, enabled = false, subtitle = "捏合缩放尚未接入") {}
            RowDivider()
            SwitchRow("锁定时允许长按和双击操作", lockAllow) { prefs.lockAllowGestures = it; revision++ }
        }
        Spacer(Modifier.height(14.dp))

        SectionCard("倍速与快进") {
            StepperRow("默认播放速度", PlaybackPrefs.speedLabel(defaultSpeed)) { picker = "speed" }
            RowDivider()
            SwitchRow("长按快进", longPress) { prefs.longPressEnabled = it; revision++ }
            RowDivider()
            StepperRow("默认长按倍速", PlaybackPrefs.speedLabel(longBase)) { picker = "longSpeed" }
            RowDivider()
            SwitchRow("快进时左右滑动调整速度", swipeSpeed) { prefs.longPressSwipeSpeed = it; revision++ }
            RowDivider()
            SwitchRow("快进时向上滑动锁定", swipeLock) { prefs.longPressSwipeLock = it; revision++ }
            RowDivider()
            StepperRow("长按松手时恢复至", PlaybackPrefs.longPressRestoreLabel(restore)) { picker = "restore" }
        }
        Footnote("松开后回到长按前的播放速度。")
    }

    when (picker) {
        "seekFwd" -> SettingsPicker(true, "快进时长",
            PlaybackPrefs.SEEK_SECONDS.map { PickerOption(it.toString(), PlaybackPrefs.seekLabel(it)) },
            seekFwd.toString(), { prefs.seekForwardSeconds = it.toInt(); revision++ }, { picker = null })
        "seekBack" -> SettingsPicker(true, "快退时长",
            PlaybackPrefs.SEEK_SECONDS.map { PickerOption(it.toString(), PlaybackPrefs.seekLabel(it)) },
            seekBack.toString(), { prefs.seekBackwardSeconds = it.toInt(); revision++ }, { picker = null })
        "center" -> SettingsPicker(true, "双击中间触发",
            listOf(PickerOption("playpause", "播放 / 暂停"), PickerOption("none", "无")),
            centerAction, { prefs.doubleTapCenterAction = it; revision++ }, { picker = null })
        "sides" -> SettingsPicker(true, "双击两侧触发",
            listOf(PickerOption("seek", "快进 / 快退"), PickerOption("none", "无")),
            sidesAction, { prefs.doubleTapSidesAction = it; revision++ }, { picker = null })
        "speed" -> SettingsPicker(true, "默认播放速度",
            PlaybackPrefs.DEFAULT_SPEEDS.map { PickerOption(it.toString(), PlaybackPrefs.speedLabel(it)) },
            defaultSpeed.toString(), { prefs.defaultSpeed = it.toDouble(); revision++ }, { picker = null })
        "longSpeed" -> SettingsPicker(true, "默认长按倍速",
            PlaybackPrefs.LONG_PRESS_SPEEDS.map { PickerOption(it.toString(), PlaybackPrefs.speedLabel(it)) },
            longBase.toString(), { prefs.longPressBaseSpeed = it.toDouble(); revision++ }, { picker = null })
        "restore" -> SettingsPicker(true, "长按松手时恢复至",
            listOf(PickerOption("previous", "长按前速度"), PickerOption("keep", "保持快进速度")),
            restore, { prefs.longPressRestore = it; revision++ }, { picker = null })
    }
}