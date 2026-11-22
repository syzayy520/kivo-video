package com.kivo.cinema.ui.design.settings

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.playback.PlaybackPrefs
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * AI 字幕子页（265 版全置灰；2026-11-22 引擎链路落地后接真，AI-DEVICE §19 之后）。
 * 本轮接真：总开关（aiSubtitleEnabled，控制播放页「AI 中文字幕」入口行显隐）、
 * 翻译开关（只读联动总开关，识别内核接入后再拆分）、翻译用量（ai_subs 产物实测）。
 * 保持置灰（如实，无后端）：语音转字幕（无识别内核）/ 系统源语言（无系统翻译接口）/ ASR 模型页。
 * 内部三级用可空 screen 枚举（ROOT/ASR/SYSLANGS），不升级 SettingsNav 栈（子页互不嵌套铁律）。
 */
private enum class AiScreen { ROOT, ASR, SYSLANGS }

@Composable
internal fun SettingsAiSub(onBack: () -> Unit) {
    val context = LocalContext.current
    val store = remember { SettingsStore(context) }
    val prefs = remember { PlaybackPrefs(context) }
    var screen by rememberSaveable { mutableStateOf(AiScreen.ROOT) }
    BackHandler(enabled = screen != AiScreen.ROOT) { screen = AiScreen.ROOT }

    when (screen) {
        AiScreen.ASR -> SettingsAsrModels(store) { screen = AiScreen.ROOT }
        AiScreen.SYSLANGS -> SettingsSysLangs(prefs) { screen = AiScreen.ROOT }
        AiScreen.ROOT -> AiRoot(prefs, store, onBack) { screen = it }
    }
}

@Composable
private fun AiRoot(prefs: PlaybackPrefs, store: SettingsStore, onBack: () -> Unit, push: (AiScreen) -> Unit) {
    var aiEnabled by remember { mutableStateOf(prefs.aiSubtitleEnabled) }
    val usage by produceState(Triple(0, 0, 0L), store) {
        value = withContext(Dispatchers.IO) { store.aiTranslationStats() }
    }
    val (translatedCount, _, usageBytes) = usage
    SubPageScaffold("AI 字幕", onBack) {
        // 组1（无节头卡）：总开关 —— 翻译内核已接入（T-B/T-C 真机全链），接真；
        // 关闭 = 播放页面板隐藏「AI 中文字幕」入口行（UX v3 禁用态）。
        PlainCard {
            SwitchRow("启用 AI 字幕", aiEnabled,
                subtitle = "关闭后播放页隐藏「AI 中文字幕」入口") { enabled ->
                prefs.aiSubtitleEnabled = enabled
                aiEnabled = enabled
            }
        }
        Spacer(Modifier.height(14.dp))

        SectionCard("语音转字幕", beta = true) {
            ChevronRow("识别音轨中的语音，生成字幕。", value = "缺少模型", valueColor = Color(0xFFFF9500),
                icon = SettingsIcons.subtitle, iconColor = SettingsVisuals.teal) {
                push(AiScreen.ASR)
            }
        }
        Footnote("识别在本机运行，需下载对应语言模型。当前版本尚未接入识别内核，模型列表为空。")
        Spacer(Modifier.height(14.dp))

        SectionCard("字幕翻译") {
            // 翻译内核已接（端侧 llama.cpp，AI-DEVICE §1/§18/§19）——与总开关同一
            // 消费点（入口行显隐），识别内核接入前不拆第二开关，如实联动显示。
            SwitchRow("使用 AI 翻译", aiEnabled, enabled = false,
                subtitle = "随「启用 AI 字幕」开关联动，识别内核接入后再拆分") {}
            ChipCardDivider()
            ChevronRow("系统支持的源语言", value = "0 种 / ${systemSourceLanguages.size}",
                icon = SettingsIcons.globe, iconColor = SettingsVisuals.blue) {
                push(AiScreen.SYSLANGS)
            }
        }
        Footnote("翻译在本机运行（端侧大模型），无需服务商与 token 计费。系统翻译语言包接口当前未接入，源语言可用数为 0。")
        Spacer(Modifier.height(14.dp))

        SectionCard("翻译用量") {
            if (translatedCount == 0) {
                ValueRow("还没有翻译用量记录", "", dim = true)
            } else {
                ValueRow("已产出译文字幕", "$translatedCount 份")
                RowDivider()
                ValueRow("空间占用", formatBytes(usageBytes))
            }
        }
        Footnote("统计本机已完成翻译并保存的字幕产物（含译文文件与翻译日志）。端侧推理不产生服务商费用。")
    }
}
