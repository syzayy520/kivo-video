package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * 播放与字幕子页（261 设计 §二）：只读真实策略 + 「敢解释」脚注。
 * 播放/字幕/按钮/手势/倍速的逐项控制在播放页面板内（PlayerComposeActivity），且当前不持久化全局默认；
 * 故本页不放未持久化的假开关，也不伪造到不可达面板的导航——只说明真实策略。
 */
@Composable
internal fun SettingsPlaybackSub(store: SettingsStore, onBack: () -> Unit) {
    SubPageScaffold("播放与字幕", onBack) {
        SectionCard("播放") {
            ValueRow("网络明文传输", if (store.clearTextOk) "已开启" else "已关闭")
            InfoRow("解码策略", "硬件解码优先；能力缺失时上报非空 reason，不静默降级（10bit 软解回退一律禁止）。")
        }
        Footnote("明文传输由安装包的 usesCleartextTraffic 决定，用于内网 HTTP 源；此处只读，调整需改构建配置。")
        Spacer(Modifier.height(14.dp))
        SectionCard("字幕") {
            InfoRow("字幕渲染", "在播放页字幕面板选择轨道与渲染样式，设置随当前播放会话生效。")
            InfoRow("轨道偏好", "在播放页字幕面板切换；当前版本不单独持久化全局默认轨道。")
        }
        Footnote("字幕、播放器按钮、手势与倍速的逐项开关在播放页面板内调整；本页只说明策略，不提供未持久化的假开关。")
    }
}