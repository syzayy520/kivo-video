package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * 语音识别模型页（265，参考「语音识别模型」2 帧：统计卡 + 13 语言列表）。
 * 统计为真实实测（filesDir/asr-models 目录文件数/字节，空目录=0/0 B），不造假值。
 * 13 语言为参考截图静态清单；识别内核未接入 ⇒ 每行「未下载」置灰带原因，刷新动作置灰。
 */
private val asrLanguages = listOf(
    "英语", "日语", "韩语", "西班牙语", "德语", "法语", "白俄罗斯语",
    "波兰语", "俄语", "克罗地亚语", "泰语", "乌克兰语", "意大利语",
)

@Composable
internal fun SettingsAsrModels(store: SettingsStore, onBack: () -> Unit) {
    val (count, bytes) = remember { store.asrModelStats() }
    SubPageScaffold("语音识别模型", onBack) {
        PlainCard {
            ValueRow("模型数量", "$count")
            RowDivider()
            ValueRow("空间占用", formatBytes(bytes))
            RowDivider()
            LinkRow("刷新模型列表", subtitle = "模型源尚未接入", enabled = false) {}
        }
        Spacer(Modifier.height(14.dp))

        PlainCard {
            asrLanguages.forEachIndexed { i, lang ->
                if (i > 0) RowDivider()
                ChevronRow(lang, value = "未下载", enabled = false,
                    subtitle = "识别内核尚未接入，下载未开放") {}
            }
        }
        Footnote("接入后，播放器会按模型大小和当前可用内存自动选择可用模型；内存不足时跳过大模型，尝试同语言小模型。")
    }
}