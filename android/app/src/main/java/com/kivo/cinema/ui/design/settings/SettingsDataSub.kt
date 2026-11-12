package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * 数据与资源子页（261 设计 §二）：媒体服务器真实导航 + TMDB 只读真实状态。
 * 海报语言/图质量/元数据语言当前为自动择优策略（#30），未持久化为可切换偏好 ⇒ 只读说明，不假开关。
 */
@Composable
internal fun SettingsDataSub(store: SettingsStore, onOpenSources: () -> Unit, onBack: () -> Unit) {
    SubPageScaffold("数据与资源", onBack) {
        SectionCard("媒体服务器") {
            ChevronRow("媒体服务器管理", "Emby · SMB · 本地资源库", onClick = onOpenSources)
        }
        Footnote("添加、编辑与隐藏资源库在「资源库」页进行；私密区与隐藏源共用应用内数字密码。")
        Spacer(Modifier.height(14.dp))
        SectionCard("TMDB 元数据") {
            ValueRow("API Key", if (store.tmdbKeyConfigured) "已配置" else "未配置")
            ValueRow("代理", store.tmdbProxy ?: "直连")
            InfoRow("海报与图质量", "自动择优：优先高清无字海报、中文标题优先，按网络与缓存自适应。")
        }
        Footnote("未配置 Key 时评分徽章整体隐藏（不显示 0 分）。代理仅作用于 TMDB 域，内网源不受影响。")
    }
}