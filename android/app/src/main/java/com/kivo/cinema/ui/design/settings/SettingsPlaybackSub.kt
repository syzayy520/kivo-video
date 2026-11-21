package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
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
import com.kivo.cinema.playback.subtitle.SubtitleAssMode
import com.kivo.cinema.playback.subtitle.SubtitleStyleStore

/**
 * 播放设置子页（262 播放设置卡，参考 0206–0209 八节）。
 * 有内核后端的能力 = 真控件（持久化到 PlaybackPrefs，播放器起播/seek 时读取）；
 * 无后端的能力 = 置灰带非空原因（铁律：不许静默降级，也不放假开关）。
 * 本页只编辑策略，不直接操作内核 —— 策略归 playback 域，播放器是消费方。
 */
@Composable
internal fun SettingsPlaybackSub(onBack: () -> Unit) {
    val context = LocalContext.current
    val prefs = remember { PlaybackPrefs(context) }
    var revision by remember { mutableIntStateOf(0) }
    var picker by remember { mutableStateOf<String?>(null) }

    val seekExact = remember(revision) { prefs.seekExact }
    val showRemaining = remember(revision) { prefs.showRemaining }
    val specSeconds = remember(revision) { prefs.startupSpecSeconds }
    val scrubPreview = remember(revision) { prefs.scrubPreview }
    val alwaysFromStart = remember(revision) { prefs.alwaysFromStart }
    val backgroundAudio = remember(revision) { prefs.backgroundAudio }
    val autoResume = remember(revision) { prefs.autoResume }
    val autoPip = remember(revision) { prefs.autoPip }
    val subtitleLanguage = remember(revision) { prefs.subtitleLanguage }
    val styleStore = remember { SubtitleStyleStore(context) }
    val assMode = remember(revision) { styleStore.load().assMode }
    val wifiBufferMb = remember(revision) { prefs.wifiBufferMb }
    val cellBufferMb = remember(revision) { prefs.cellBufferMb }
    val renderMode = remember(revision) { prefs.renderMode }
    val interpolation = remember(revision) { prefs.videoInterpolation }

    SubPageScaffold("播放", onBack) {
        SectionCard("缓冲") {
            SwitchRow("预缓冲到本地文件", false, beta = true, enabled = false,
                subtitle = "本地预缓冲尚未接入") {}
            ChipCardDivider()
            StepperRow("Wi-Fi 缓冲上限", PlaybackPrefs.bufferLabel(wifiBufferMb),
                icon = SettingsIcons.download, iconColor = SettingsVisuals.blue) { picker = "wifi" }
            ChipCardDivider()
            StepperRow("蜂窝网络缓冲上限", PlaybackPrefs.bufferLabel(cellBufferMb)) { picker = "cell" }
            ChipCardDivider()
            SwitchRow("关闭播放器时清空视频缓存", false, enabled = false,
                subtitle = "内核缓冲在内存中，关闭即释放，无磁盘缓存可清") {}
        }
        Footnote("缓冲上限决定起播前预读多少数据：越大起播越稳，但占用更多内存。")
        Spacer(Modifier.height(14.dp))

        SectionCard("下一集预加载") {
            SwitchRow("提前准备下一集", false, enabled = false,
                subtitle = "剧集列表尚未接入资源库") {}
        }
        Footnote("接入剧集列表后，将在播放当前集时提前缓冲下一集。")
        Spacer(Modifier.height(14.dp))

        SectionCard("进入详情预加载") {
            StepperRow("进入详情后预加载", "始终开启", enabled = false,
                subtitle = "详情预加载尚未接入") {}
            RowDivider()
            ValueRow("适用资源库", "0", subtitle = "详情预加载尚未接入")
        }
        Footnote("进入影片详情时提前加载播放资源，减少起播等待。")
        Spacer(Modifier.height(14.dp))

        SectionCard("播放习惯") {
            SwitchRow("每次从头播放", alwaysFromStart,
                subtitle = "开启后忽略上次进度，每次从 00:00 起播",
                icon = SettingsIcons.play, iconColor = SettingsVisuals.blue) { prefs.alwaysFromStart = it; revision++ }
            ChipCardDivider()
            SwitchRow("精确定位进度", seekExact,
                subtitle = "拖动进度时精确到帧，而非落在最近的关键帧") { prefs.seekExact = it; revision++ }
            ChipCardDivider()
            SwitchRow("显示剩余时长", showRemaining,
                subtitle = "底栏右侧显示剩余时间而非总时长") { prefs.showRemaining = it; revision++ }
            ChipCardDivider()
            StepperRow("起播时显示片源规格", PlaybackPrefs.specLabel(specSeconds),
                subtitle = "起播时短暂显示分辨率/编码等片源信息") { picker = "spec" }
        }
        Spacer(Modifier.height(14.dp))

        SectionCard("进度预览") {
            SwitchRow("拖动时预览画面", scrubPreview,
                subtitle = "拖动进度条时显示目标位置的画面缩略图") { prefs.scrubPreview = it; revision++ }
            RowDivider()
            SwitchRow("服务器预处理", false, enabled = false,
                subtitle = "服务器预处理尚未接入") {}
            RowDivider()
            ValueRow("来源", "自动", subtitle = "预览来源选择尚未接入")
        }
        Footnote("预览画面由本地解码生成；服务器预处理可在低性能设备上改用服务端缩略图。")
        Spacer(Modifier.height(14.dp))

        SectionCard("片段跳过") {
            StepperRow("默认跳过方式", "自动", enabled = false, subtitle = "片段标记尚未接入") {}
            RowDivider()
            StepperRow("跳过提醒", "10秒前", enabled = false, subtitle = "片段标记尚未接入") {}
            RowDivider()
            SwitchRow("取消自动跳过后保留跳过按钮", false, enabled = false, subtitle = "片段标记尚未接入") {}
            RowDivider()
            SwitchRow("智能过滤 Emby 章节", false, enabled = false, subtitle = "章节跳过尚未接入") {}
            RowDivider()
            ValueRow("画面关键词智能识别", "未接入", subtitle = "画面识别尚未接入")
        }
        Footnote("片头/片尾与片段标记接入后，可自动跳过并保留手动跳过按钮。")
        Spacer(Modifier.height(14.dp))

        SectionCard("视频输出") {
            StepperRow("渲染通路", PlaybackPrefs.renderModeLabel(renderMode),
                subtitle = "兼容模式用于个别设备上新版通路显示异常时退回",
                icon = SettingsIcons.play, iconColor = SettingsVisuals.blue) { picker = "render" }
            ChipCardDivider()
            StepperRow("帧插值", PlaybackPrefs.interpolationLabel(interpolation),
                subtitle = "把 24 帧片源按屏幕刷新率插帧，运动更顺滑（可能失去胶片感）") { picker = "interp" }
        }
        Footnote("新版通路由播放内核完整渲染器输出，高动态范围片源亮度更准确。帧插值在高码率 4K 片上会增加显卡负担。")
        Spacer(Modifier.height(14.dp))

        SectionCard("后台播放") {
            SwitchRow("后台继续播放音频", backgroundAudio,
                subtitle = "退到后台时暂停或继续播放声音") { prefs.backgroundAudio = it; revision++ }
            RowDivider()
            SwitchRow("后台恢复时自动播放", autoResume,
                subtitle = "回到前台时自动恢复播放") { prefs.autoResume = it; revision++ }
            RowDivider()
            SwitchRow("自动开启画中画", autoPip,
                subtitle = "按 Home 键离页时自动进入画中画小窗") { prefs.autoPip = it; revision++ }
        }
        Footnote("后台播放需要通知栏常驻；画中画需系统支持并在播放中触发。")
        Spacer(Modifier.height(14.dp))

        SectionCard("字幕") {
            StepperRow("字幕首选语言", PlaybackPrefs.languageLabel(subtitleLanguage),
                subtitle = "自动选择字幕轨时优先匹配的语言",
                icon = SettingsIcons.subtitle, iconColor = SettingsVisuals.teal) { picker = "lang" }
            ChipCardDivider()
            SwitchRow("启用字幕 HDR 效果", false, enabled = false, subtitle = "字幕 HDR 尚未接入") {}
            ChipCardDivider()
            StepperRow("ASS 字幕样式", assMode.label,
                subtitle = "源样式保留排版特效；仅缩放只放大字号；统一样式覆盖源排版",
                icon = SettingsIcons.subtitle, iconColor = SettingsVisuals.teal) { picker = "ass" }
        }
        Footnote("字幕渲染由播放内核负责；与播放器内「字幕显示设置」共用同一份配置。")
    }

    when (picker) {
        "wifi" -> SettingsPicker(true, "Wi-Fi 缓冲上限",
            PlaybackPrefs.BUFFER_MB.map { PickerOption(it.toString(), PlaybackPrefs.bufferLabel(it)) },
            wifiBufferMb.toString(), { prefs.wifiBufferMb = it.toInt(); revision++ }, { picker = null })
        "cell" -> SettingsPicker(true, "蜂窝网络缓冲上限",
            PlaybackPrefs.BUFFER_MB.map { PickerOption(it.toString(), PlaybackPrefs.bufferLabel(it)) },
            cellBufferMb.toString(), { prefs.cellBufferMb = it.toInt(); revision++ }, { picker = null })
        "spec" -> SettingsPicker(true, "起播时显示片源规格",
            PlaybackPrefs.SPEC_SECONDS.map { PickerOption(it.toString(), PlaybackPrefs.specLabel(it)) },
            specSeconds.toString(), { prefs.startupSpecSeconds = it.toInt(); revision++ }, { picker = null })
        "render" -> SettingsPicker(true, "渲染通路",
            listOf(
                PickerOption(PlaybackPrefs.RENDER_MODE_AUTO, PlaybackPrefs.renderModeLabel(PlaybackPrefs.RENDER_MODE_AUTO)),
                PickerOption(PlaybackPrefs.RENDER_MODE_GPU_NEXT, PlaybackPrefs.renderModeLabel(PlaybackPrefs.RENDER_MODE_GPU_NEXT)),
                PickerOption(PlaybackPrefs.RENDER_MODE_LEGACY, PlaybackPrefs.renderModeLabel(PlaybackPrefs.RENDER_MODE_LEGACY)),
            ),
            renderMode, { prefs.renderMode = it; revision++ }, { picker = null })
        "interp" -> SettingsPicker(true, "帧插值",
            listOf(
                PickerOption(PlaybackPrefs.INTERPOLATION_AUTO, PlaybackPrefs.interpolationLabel(PlaybackPrefs.INTERPOLATION_AUTO)),
                PickerOption(PlaybackPrefs.INTERPOLATION_ON, PlaybackPrefs.interpolationLabel(PlaybackPrefs.INTERPOLATION_ON)),
                PickerOption(PlaybackPrefs.INTERPOLATION_OFF, PlaybackPrefs.interpolationLabel(PlaybackPrefs.INTERPOLATION_OFF)),
            ),
            interpolation, { prefs.videoInterpolation = it; revision++ }, { picker = null })
        "lang" -> SettingsPicker(true, "字幕首选语言",
            PlaybackPrefs.LANGUAGES.map { PickerOption(it.code, it.label) },
            subtitleLanguage, { prefs.subtitleLanguage = it; revision++ }, { picker = null })
        "ass" -> SettingsPicker(true, "ASS 字幕样式",
            SubtitleAssMode.entries.map { PickerOption(it.name, it.label) },
            assMode.name, {
                val mode = SubtitleAssMode.entries.firstOrNull { m -> m.name == it } ?: assMode
                styleStore.update { s -> s.copy(assMode = mode) }
                revision++
            }, { picker = null })
    }
}