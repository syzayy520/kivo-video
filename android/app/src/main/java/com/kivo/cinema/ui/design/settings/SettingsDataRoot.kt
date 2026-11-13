package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.ui.design.GlassSurface

/**
 * 数据与资源根页（266 设计 §一）：语言偏好/资源聚合/数据来源/内容加载/选片资源排序 5 卡。
 * 只组合行原语 + 调 Tmdb 卡，不堆业务。数据来源计数为真：聚合/搜索按实测源记录算，
 * 本机类「1/1」（仅本机可用），片段标记「未接入」（钻入页六源全灰）。灰行带非空原因。
 */
@Composable
internal fun SettingsDataRoot(
    store: SettingsStore,
    records: List<SourceRecord>,
    onOpenSources: () -> Unit,
    onBack: () -> Unit,
    push: (DataSourceCategory) -> Unit,
) {
    SubPageScaffold("数据与资源", onBack) {
        GlassSurface(Modifier.fillMaxWidth()) {
            Column {
                StepperRow("元数据语言偏好", "简体中文", enabled = false,
                    subtitle = "语言固定简体中文，偏好未持久化") {}
                RowDivider()
                StepperRow("Logo 语言", "自动择优", enabled = false,
                    subtitle = "Logo 语言随自动择优，未持久化") {}
                RowDivider()
                StepperRow("海报语言", "自动择优", enabled = false,
                    subtitle = "海报语言随自动择优，未持久化") {}
            }
        }
        Footnote("元数据固定简体中文，Logo 与海报按自动择优（优先高清无字、中文标题）。语言偏好持久化尚未接入。")
        Spacer(Modifier.height(14.dp))
        SectionCard("资源聚合") {
            SwitchRow("聚合有效资源库", false, enabled = false,
                subtitle = "无全局聚合开关，按资源库单独配置") {}
            RowDivider()
            SwitchRow("打开资源库后保持聚合", false, enabled = false,
                subtitle = "无全局聚合开关，按资源库单独配置") {}
            RowDivider()
            SwitchRow("浏览资源库时优先当前库", false, enabled = false,
                subtitle = "无全局聚合开关，按资源库单独配置") {}
        }
        Footnote("聚合按资源库单独配置：在「资源聚合(详情)」与「搜索来源」中逐源开关，或在资源库管理页修改。无全局聚合开关。")
        Spacer(Modifier.height(14.dp))
        val visible = records.filter { !it.hidden }
        val aggOn = visible.count { it.aggregateEnabled }
        val emby = visible.filter { !it.isSmb }
        val searchOn = emby.count { it.aggregateSearchEnabled }
        SectionCard("数据来源") {
            IconChevronRow("▦", Color(0xFF007AFF), "资源聚合(详情)", "$aggOn/${visible.size}") { push(DataSourceCategory.AGGREGATE) }
            RowDivider()
            IconChevronRow("◎", Color(0xFF34C759), "搜索来源", "$searchOn/${emby.size}") { push(DataSourceCategory.SEARCH) }
            RowDivider()
            IconChevronRow("◷", Color(0xFFFF9500), "观看记录来源", "1/1") { push(DataSourceCategory.WATCH) }
            RowDivider()
            IconChevronRow("ⓘ", Color(0xFF5856D6), "影片信息来源", "1/1") { push(DataSourceCategory.META) }
            RowDivider()
            IconChevronRow("◪", Color(0xFFFF2D55), "片段标记来源", "未接入") { push(DataSourceCategory.MARKS) }
            RowDivider()
            IconChevronRow("★", Color(0xFFFF2D55), "收藏数据来源", "1/1") { push(DataSourceCategory.FAVORITE) }
            RowDivider()
            IconChevronRow("▥", Color(0xFF34C759), "观影统计", "1/1") { push(DataSourceCategory.STATS) }
            RowDivider()
            IconChevronRow("≡", Color(0xFF007AFF), "待看来源", "1/1") { push(DataSourceCategory.WATCHLIST) }
        }
        Footnote("来源开关仅作用于当前环境。本机记录（观看/收藏/统计/待看）存于本机，不随资源库区分；Trakt 等外部源尚未接入。")
        Spacer(Modifier.height(14.dp))
        DataTmdbCard(store)
        Spacer(Modifier.height(14.dp))
        DataSortCard()
        Spacer(Modifier.height(14.dp))
        LinkRow("媒体服务器管理", subtitle = "添加、编辑与隐藏资源库", onClick = onOpenSources)
    }
}