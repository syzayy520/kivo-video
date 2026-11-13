package com.kivo.cinema.ui.design.settings

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.AppearancePrefs

/**
 * 外观与交互子页（267 设计 §二）：路由 + 前四卡。
 * 反馈族四卡在 SettingsAppearanceFeedback；导航/Badge 两钻入各一文件。
 * 钻入路由用 266 模式：可空枚举 + rememberSaveable + 内 BackHandler，不升级 nav 栈。
 */
internal enum class AppearanceDrill { NAV, BADGE }

@Composable
internal fun SettingsAppearanceSub(
    previewEnabled: Boolean,
    onPreviewChanged: (Boolean) -> Unit,
    back: () -> Unit,
) {
    val context = LocalContext.current
    LaunchedEffect(Unit) { AppearancePrefs.bind(context) }
    var drill by rememberSaveable { mutableStateOf<AppearanceDrill?>(null) }
    BackHandler(enabled = drill != null) { drill = null }
    when (drill) {
        AppearanceDrill.NAV -> SettingsAppearanceNav { drill = null }
        AppearanceDrill.BADGE -> SettingsAppearanceBadge { drill = null }
        null -> SubPageScaffold("外观与交互", back) {
            AppearanceCoreCards { drill = it }
            Spacer(Modifier.height(14.dp))
            AppearanceFeedbackCards(previewEnabled, onPreviewChanged) { drill = it }
        }
    }
}

/** 前四卡：外观/封面与剧照/首页/日历。灰行一律带非空原因（267 §一）。 */
@Composable
private fun AppearanceCoreCards(push: (AppearanceDrill) -> Unit) {
    PlainCard {
        ValueRow("外观模式", "浅色", subtitle = "深色色板 v2 未实现，当前固定浅色。", dim = true)
        RowDivider()
        SwitchRow("减弱动画效果", false, enabled = false,
            subtitle = "未接全局动效门，各页动效参数独立。", onChange = {})
        RowDivider()
        SwitchRow("减弱视觉效果", false, enabled = false,
            subtitle = "未接全局视觉门，玻璃/模糊按宿主独立。", onChange = {})
        RowDivider()
        SwitchRow("豆瓣评分入口", false, enabled = false,
            subtitle = "豆瓣数据源未接入，评分徽章仅 TMDB。", onChange = {})
        RowDivider()
        ChevronRow("导航", icon = SettingsIcons.menu, iconColor = SettingsVisuals.blue) { push(AppearanceDrill.NAV) }
    }
    Spacer(Modifier.height(14.dp))
    SectionCard("封面与剧照") {
        ValueRow("首页封面样式", "海报", subtitle = "横向封面布局未接入，行卡固定海报比。", dim = true)
        RowDivider()
        ValueRow("详情封面样式", "幕布", subtitle = "海报模式未接入，详情固定满幅幕布。", dim = true)
        RowDivider()
        SwitchRow("显示剧照", AppearancePrefs.showStills,
            icon = SettingsIcons.image, iconColor = SettingsVisuals.purple,
            onChange = { AppearancePrefs.updateShowStills(it) })
        RowDivider()
        SwitchRow("优先使用 TMDB 高清无字海报", false, enabled = false,
            subtitle = "择优按位硬编码：hero 无字优先、详情中文优先、行卡跟元数据语言；全局开关未接。", onChange = {})
    }
    Spacer(Modifier.height(14.dp))
    SectionCard("首页") {
        SwitchRow("固定首页布局", false, enabled = false, onChange = {})
    }
    Footnote("首页布局编辑未开放，无锁定对象；此开关未接线。")
    Spacer(Modifier.height(14.dp))
    PlainCard {
        SwitchRow("自动将待看添加到日历", false, enabled = false,
            subtitle = "系统日历写入未接入；应用内追剧日历在详情页。", onChange = {})
    }
}