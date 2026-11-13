package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository

/** 数据来源钻入类别（266 设计 §一）：根页 8 计数行各对应一类。片段标记无后端，根页置灰不钻入。 */
internal enum class DataSourceCategory { AGGREGATE, SEARCH, WATCH, META, MARKS, FAVORITE, STATS, WATCHLIST }

/**
 * 8 来源钻入页通用渲染器（数据驱动，单文件单职责）：
 * 本机/TMDB 恒启用 = 只读 Switch（checked=true enabled=false，淡绿轨，非假可切换）；
 * 外部源（Trakt/AI/片段标记）= 灰 Switch + 非空原因；每源聚合/搜索 = 真 Switch 写回 accounts.bin。
 * 不读 WatchStore/FilmMarks 数据造假计数——本机可用=1 为真；记录仅存本机，不列假服务器行。
 */
@Composable
internal fun DataSourcePage(
    category: DataSourceCategory,
    records: List<SourceRecord>,
    onControl: (SourceRecord, SourceRepository.Control, Boolean) -> Unit,
    onOpenSources: () -> Unit,
    onBack: () -> Unit,
) {
    val title = when (category) {
        DataSourceCategory.AGGREGATE -> "资源聚合(详情)"
        DataSourceCategory.SEARCH -> "搜索来源"
        DataSourceCategory.WATCH -> "观看记录来源"
        DataSourceCategory.META -> "影片信息来源"
        DataSourceCategory.MARKS -> "片段标记来源"
        DataSourceCategory.FAVORITE -> "收藏数据来源"
        DataSourceCategory.STATS -> "观影统计"
        DataSourceCategory.WATCHLIST -> "待看来源"
    }
    SubPageScaffold(title, onBack) {
        when (category) {
            DataSourceCategory.AGGREGATE -> AggregateContent(records, onControl, onOpenSources)
            DataSourceCategory.SEARCH -> SearchContent(records, onControl)
            DataSourceCategory.META -> MetaContent(records)
            DataSourceCategory.MARKS -> MarksContent()
            DataSourceCategory.WATCH -> LocalTraktContent("本机观看记录，始终启用",
                "观看记录仅存本机（watch.json），按文件路径记录续播位，「继续观看」读取本机记录。Trakt 同步未接入。")
            DataSourceCategory.FAVORITE -> LocalTraktContent("本机收藏，始终启用",
                "收藏与待看仅存本机（media/marks/titles.json），按影片标记。Trakt 同步未接入。")
            DataSourceCategory.STATS -> LocalTraktContent("本机统计，始终启用",
                "观影统计仅存本机，按天累计观看时长。30 天跨设备同步与 Trakt 未接入。")
            DataSourceCategory.WATCHLIST -> LocalTraktContent("本机待看，始终启用",
                "收藏与待看仅存本机（media/marks/titles.json），按影片标记。Trakt 同步未接入。")
        }
    }
}

@Composable
private fun LocalRow(title: String, subtitle: String) {
    SwitchRow(title, true, enabled = false, subtitle = subtitle) {}
}

@Composable
private fun GrayRow(title: String, reason: String) {
    SwitchRow(title, false, enabled = false, subtitle = reason) {}
}

@Composable
private fun AggregateContent(
    records: List<SourceRecord>,
    onControl: (SourceRecord, SourceRepository.Control, Boolean) -> Unit,
    onOpenSources: () -> Unit,
) {
    val visible = records.filter { !it.hidden }
    SectionCard("资源库") {
        if (visible.isEmpty()) {
            ValueRow("还没有资源库", "", dim = true)
        } else {
            visible.forEachIndexed { index, record ->
                if (index > 0) RowDivider()
                SwitchRow(record.name, record.aggregateEnabled,
                    subtitle = if (record.isSmb) "SMB 网络共享" else "Emby 媒体服务器",
                ) { onControl(record, SourceRepository.Control.AGGREGATE, it) }
            }
        }
    }
    Footnote("控制该资源库内容是否加入首页聚合。顺序固定，拖拽排序未接入。添加/编辑/隐藏资源库在资源库管理页进行。")
    Spacer(Modifier.height(14.dp))
    LinkRow("媒体服务器管理", subtitle = "添加、编辑与隐藏资源库", onClick = onOpenSources)
}

@Composable
private fun SearchContent(
    records: List<SourceRecord>,
    onControl: (SourceRecord, SourceRepository.Control, Boolean) -> Unit,
) {
    SectionCard("搜索来源") {
        GrayRow("TMDB", "TMDB 提供评分与首页推荐，不参与聚合搜索")
        RowDivider()
        GrayRow("网络硬盘", "WebDAV 未接入聚合搜索")
        records.filter { !it.hidden }.forEach { record ->
            RowDivider()
            if (record.isSmb) GrayRow(record.name, "SMB 不参与聚合搜索")
            else SwitchRow(record.name, record.aggregateSearchEnabled, subtitle = "Emby 媒体服务器",
            ) { onControl(record, SourceRepository.Control.AGGREGATE_SEARCH, it) }
        }
        RowDivider()
        GrayRow("AI 搜索", "AI 搜索未接入")
    }
    Footnote("控制该资源库是否参与跨服务器搜索。SMB 不参与聚合搜索；TMDB 提供评分与首页推荐，不参与搜索；AI 搜索未接入。")
}

@Composable
private fun MetaContent(records: List<SourceRecord>) {
    SectionCard("信息来源") {
        LocalRow("TMDB", "固定简体中文")
        records.filter { !it.hidden }.forEach { record ->
            RowDivider()
            GrayRow(record.name, "元数据源优先级未接入")
        }
    }
    Footnote("影片信息来自 TMDB，固定简体中文。按资源库区分元数据源与优先级尚未接入。")
}

@Composable
private fun MarksContent() {
    SectionCard("标记来源") {
        listOf("本机手动标记", "TheOtherDB", "Emby 章节", "TheIntroDB", "IntroDB", "PublicMetaDB")
            .forEachIndexed { index, name ->
                if (index > 0) RowDivider()
                GrayRow(name, "片头/片尾标记尚未接入")
            }
    }
    Footnote("片头/片尾与片段标记尚未接入，播放器内「跳过片头/片尾」同样置灰。接入后可按来源自动跳过。")
}

@Composable
private fun LocalTraktContent(localSubtitle: String, footnote: String) {
    SectionCard("数据来源") {
        LocalRow("本机", localSubtitle)
        RowDivider()
        GrayRow("Trakt", "Trakt 未接入")
    }
    Footnote(footnote)
}