package com.kivo.cinema.ui.design.settings

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.playback.PlaybackPrefs
import com.kivo.cinema.ui.design.GlassSurface

/**
 * AI 字幕子页（265，参考「AI 字幕」9 帧：根 OFF/ON + 服务商/模型选择 + 语音识别模型 + 系统源语言）。
 * 审计结论：仓库无 ASR/LLM/系统翻译后端（whisper/openai/deepseek/SpeechRecognizer 全 0 命中），
 * 故所有开关/服务商/模型下载一律置灰带非空原因；ON 态条件行（服务商 13 选/API Key/模型选择/
 * 自定义翻译规则/测试翻译服务）不实现 —— 灰开关不可达，不留死 UI。
 * 真实部分：三级钻入导航、目标语言读 PlaybackPrefs.subtitleLanguage、模型目录实测、用量空态。
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
        AiScreen.ROOT -> AiRoot(onBack) { screen = it }
    }
}

@Composable
private fun AiRoot(onBack: () -> Unit, push: (AiScreen) -> Unit) {
    SubPageScaffold("AI 字幕", onBack) {
        // 组1（无节头卡）：总开关 —— 识别/翻译内核均未接入 ⇒ 置灰不可达。
        GlassSurface(Modifier.fillMaxWidth()) {
            Column {
                SwitchRow("启用 AI 字幕", false, enabled = false,
                    subtitle = "语音识别与翻译内核尚未接入，开关未开放") {}
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
            SwitchRow("使用 AI 翻译", false, enabled = false,
                subtitle = "大模型翻译客户端尚未接入") {}
            RowDivider()
            ChevronRow("系统支持的源语言", value = "0 种 / ${systemSourceLanguages.size}",
                icon = SettingsIcons.globe, iconColor = SettingsVisuals.blue) {
                push(AiScreen.SYSLANGS)
            }
        }
        Footnote("Android 无 iOS 式系统翻译语言包接口；当前版本尚未接入翻译内核，源语言可用数为 0。")
        Spacer(Modifier.height(14.dp))

        SectionCard("翻译用量") {
            ValueRow("还没有翻译用量记录", "", dim = true)
        }
        Footnote("记录这台设备用于字幕翻译的累计 token 用量，仅供参考，实际费用以服务商账单为准。当前未接入翻译，无记录。")
    }
}