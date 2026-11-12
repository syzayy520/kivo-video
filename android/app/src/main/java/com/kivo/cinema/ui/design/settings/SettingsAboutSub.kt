package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * 关于子页（261 设计 §二）：只读真实版本 + 私密入口真实导航。
 * 版本取自 PackageManager（真实 versionName/versionCode），不用硬编码标签。
 */
@Composable
internal fun SettingsAboutSub(store: SettingsStore, onOpenPrivate: () -> Unit, onBack: () -> Unit) {
    SubPageScaffold("关于", onBack) {
        SectionCard("版本") {
            ValueRow("版本", store.versionLabel)
        }
        Spacer(Modifier.height(14.dp))
        SectionCard("私密与安全") {
            ChevronRow("应用内数字密码", "私密区 · 隐藏源 · 改密", onClick = onOpenPrivate)
        }
        Footnote("私密区与隐藏源共用一个应用内 4–6 位数字密码（独立于手机锁屏）；退后台自动重锁。")
    }
}