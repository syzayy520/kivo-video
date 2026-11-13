package com.kivo.cinema.ui.design.settings

import androidx.compose.runtime.Composable
import com.kivo.cinema.tmdb.TmdbClient

/**
 * 数据与资源根页两张卡（266 设计 §一）：内容加载（TMDB 真只读 + 灰）+ 选片资源排序（全灰）。
 * 只组合行原语，不堆业务；真实值来自 SettingsStore/TmdbClient，灰行带非空原因，不放假开关。
 */
@Composable
internal fun DataTmdbCard(store: SettingsStore) {
    SectionCard("内容加载") {
        SwitchRow("自定义 TMDB API", false, enabled = false,
            subtitle = "API 地址固定官方域，自定义未接入") {}
        RowDivider()
        ValueRow("API 地址", TmdbClient.API_BASE, subtitle = "固定官方域，自定义未接入")
        RowDivider()
        ValueRow("API Key", if (store.tmdbKeyConfigured) "已配置" else "未配置",
            subtitle = "经 filesDir/tmdb_key.txt 写入，UI 输入未接入")
        RowDivider()
        LinkRow("测试连接并保存", subtitle = "连接测试未接入", enabled = false) {}
        RowDivider()
        SwitchRow("图片加载修复", false, enabled = false, subtitle = "图片加载修复未接入") {}
        RowDivider()
        ValueRow("代理", store.tmdbProxy ?: "直连", subtitle = "仅作用于 TMDB 域，内网源不受影响")
    }
    Footnote("未配置 Key 时评分徽章整体隐藏（不显示 0 分）。Key 经文件写入，代理仅作用于 TMDB 域。")
}

@Composable
internal fun DataSortCard() {
    SectionCard("选片资源排序") {
        StepperRow("当前排序规则", "未接入", enabled = false, subtitle = "排序规则偏好未接入") {}
        RowDivider()
        StepperRow("色彩范围", "从大到小", enabled = false, subtitle = "按色彩范围排序未接入") {}
        RowDivider()
        StepperRow("分辨率", "从大到小", enabled = false, subtitle = "按分辨率排序未接入") {}
        RowDivider()
        StepperRow("文件大小", "从小到大", enabled = false, subtitle = "按文件大小排序未接入") {}
        RowDivider()
        StepperRow("视频码率", "从大到小", enabled = false, subtitle = "按视频码率排序未接入") {}
    }
    Footnote("聚合搜索当前按「最新更新 / 最新上映 / 影片评分」排序，在搜索页选择。按色彩范围/分辨率/文件大小/码率排序与自定义规则尚未接入。")
}