package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.playback.PlaybackPrefs

/**
 * 系统支持的源语言页（265，参考「系统支持的源语言」帧：目标语言 + 源语言清单 + 刷新）。
 * 目标语言为真实读 PlaybackPrefs.subtitleLanguage（与「播放」子页同源，改字幕语言这里联动）。
 * Android 无 iOS 式系统翻译语言包接口 ⇒ 源语言清单为参考静态列表，每行「未接入」置灰，刷新置灰。
 */
internal val systemSourceLanguages = listOf(
    "俄语", "德语", "意大利语", "日语", "法语", "英语", "葡萄牙语", "西班牙语", "阿拉伯语", "韩语",
)

@Composable
internal fun SettingsSysLangs(prefs: PlaybackPrefs, onBack: () -> Unit) {
    val target = remember { PlaybackPrefs.languageLabel(prefs.subtitleLanguage) }
    SubPageScaffold("系统支持的源语言", onBack) {
        PlainCard {
            ValueRow("目标语言", target)
            systemSourceLanguages.forEach { lang ->
                RowDivider()
                ValueRow(lang, "未接入", dim = true)
            }
        }
        Footnote("这里列出可翻译成当前目标语言的原文语言。Android 无系统翻译语言包接口，当前版本尚未接入翻译内核。")
        Spacer(Modifier.height(8.dp))
        PlainCard {
            LinkRow("刷新已下载语言", subtitle = "Android 无系统翻译语言包 API", enabled = false) {}
        }
    }
}