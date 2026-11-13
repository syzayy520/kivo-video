package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.home.HomePreviewSetting

/**
 * 设置根页（261 设计 §二/§三）：浅色 inset 分组卡列表，只组合行原语，不堆业务。
 * 每行绑定真实持久化/真实导航；无对应后端者不放（模块/服务/PRO/iCloud/Trakt 等不纳入）。
 * autoplay 跨页状态由 MainActivity 持有（与发现页同步），经 onPreviewChanged 回写 HomePreferences。
 */
@Composable
fun SettingsPage(
    previewEnabled: Boolean,
    onPreviewChanged: (Boolean) -> Unit,
    onOpenSources: () -> Unit,
    onOpenPrivate: () -> Unit,
) {
    val context = LocalContext.current
    val store = remember { SettingsStore(context) }
    SettingsNavHost(
        root = { push ->
            SettingsScaffold("设置") {
                SectionCard("首页") {
                    HomePreviewSetting(previewEnabled, onPreviewChanged)
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("外观与交互") {
                    ChevronRow("外观与交互", "主题 · 封面 · 预告片 · 触感 · 图标") { push(SettingsRoute.APPEARANCE) }
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("播放") {
                    ChevronRow("播放", "缓冲 · 播放习惯 · 后台 · 字幕") { push(SettingsRoute.PLAYBACK) }
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("操控") {
                    ChevronRow("操控", "按钮 · 手势 · 双击 · 长按快进") { push(SettingsRoute.CONTROLS) }
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("音画") {
                    ChevronRow("音画", "内核 · 画面 · 音频 · 画质增强") { push(SettingsRoute.AV) }
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("AI 字幕") {
                    ChevronRow("AI 字幕", "语音转字幕 · 翻译 · 用量") { push(SettingsRoute.AI) }
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("数据与资源") {
                    ChevronRow("数据与资源", "语言 · 聚合 · 来源 · TMDB · 排序") { push(SettingsRoute.DATA) }
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("私密与安全") {
                    ChevronRow("应用内数字密码", "私密区 · 隐藏源 · 改密", onClick = onOpenPrivate)
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("存储与缓存") {
                    ChevronRow("清理缓存", "图片缓存 · 评分接口缓存") { push(SettingsRoute.CACHE) }
                }
                Spacer(Modifier.height(14.dp))
                SectionCard("关于") {
                    ChevronRow("版本信息", "版本号 · 构建 · 私密安全") { push(SettingsRoute.ABOUT) }
                }
            }
        },
        sub = { route, back ->
            when (route) {
                SettingsRoute.PLAYBACK -> SettingsPlaybackSub(back)
                SettingsRoute.CONTROLS -> SettingsControlsSub(back)
                SettingsRoute.APPEARANCE -> SettingsAppearanceSub(previewEnabled, onPreviewChanged, back)
                SettingsRoute.AV -> SettingsAvSub(back)
                SettingsRoute.AI -> SettingsAiSub(back)
                SettingsRoute.DATA -> SettingsDataSub(store, onOpenSources, back)
                SettingsRoute.CACHE -> SettingsCacheSub(store, back)
                SettingsRoute.ABOUT -> SettingsAboutSub(store, onOpenPrivate, back)
            }
        },
    )
}