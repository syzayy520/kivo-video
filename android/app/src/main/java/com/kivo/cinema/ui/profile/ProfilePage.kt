package com.kivo.cinema.ui.profile

import android.content.Intent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.calendar.CalendarActivity
import com.kivo.cinema.ui.profile.calendar.WeekStrip
import com.kivo.cinema.ui.profile.charts.GenreDonut
import com.kivo.cinema.ui.profile.charts.MonthlyChart
import com.kivo.cinema.ui.profile.charts.TopShowsCard
import com.kivo.cinema.ui.profile.charts.TrendCard
import com.kivo.cinema.ui.profile.chrome.ManageModulesPill
import com.kivo.cinema.ui.profile.chrome.ProfileEnvironmentHeader
import com.kivo.cinema.ui.profile.lists.CollectionSection
import com.kivo.cinema.ui.profile.lists.WatchlistSection
import com.kivo.cinema.ui.profile.lists.WatchingSection
import com.kivo.cinema.ui.profile.model.ProfileDashboard

import com.kivo.cinema.ui.profile.model.TrendStat
import com.kivo.cinema.ui.profile.model.WatchEntry
import com.kivo.cinema.ui.profile.modules.ModuleManagerSheet
import com.kivo.cinema.ui.profile.modules.ProfileModulePrefs
import com.kivo.cinema.ui.profile.overview.HistoryHeatmap
import com.kivo.cinema.ui.profile.overview.TodaySummaryCard
import com.kivo.cinema.ui.shell.ShellInsets

/**
 * 「我的」页 · **B2 版复刻编排**（基准 `docs/audit/golden/kivo/profile-1..5.png`）。
 *
 * 本文件只做编排：不取数、不补假值、不改子组件形状；单列 [LazyColumn] 的 item 顺序即截图顺序：
 *
 * ① [ProfileEnvironmentHeader]（`chrome/ProfileChrome.kt`：左「本地」26sp Bold + 右`[日历] [编辑]`）
 * ② [WeekStrip] ③ [TodaySummaryCard] ④「历史观看」节标题 + [HistoryHeatmap]
 * ⑤ [WatchlistSection] ⑥ [CollectionSection] + [WatchingSection] 双卡一行（各 `weight(1f)`，间距 10dp）
 * ⑦ [MonthlyChart] ⑧ [TopShowsCard] ⑨「类型分布」节标题 + [GenreDonut] ⑩ `dashboard.trends` 逐条 [TrendCard]
 * ⑪「管理模块」白胶囊 → [ModuleManagerSheet]
 *
 * 「类型分布」标题由本页提供（key `genre-title`）而不是 [GenreDonut] 内建：基准 `profile-4.png`
 * 的标题在卡片外，与「历史观看」同款 16sp Bold / 左右 13dp。
 *
 * 尺寸依赖（◐ 推断，实拍见基准图）：各区块的水平 13dp 内边距由**子组件自带**（契约签名不含
 * padding 参数），本页只提供 10dp 的垂直间距与底部 [ShellInsets.NavReservedHeight] 预留；
 * 若子组件未自带内边距，页面上会表现为「贴边」，由 R5 同屏对比打回。
 *
 * 旧原创版组件（`ProfileHeader` / `ContinueHero` / `CollectionsRow` / `WeeklyReport` /
 * `WeeklyReportCharts`）**不再被调用**，文件保留待 P4 清理；`onEdit` / `onOpenWatching` /
 * `onOpenItem` 的行为与 B2 版一致。
 *
 * @param onManageModules 外部通知钩子：页面自身打开 [ModuleManagerSheet]，同时向 Shell 抛一次
 *   事件（当前 Shell 传空实现）。两处都做是刻意的 —— 页面不依赖 Shell 才能自洽，Shell 后续若要
 *   埋点/埋深链不必改本页签名。
 */
@Composable
fun ProfilePage(
    dashboard: ProfileDashboard,
    onEditEnvironment: () -> Unit,
    onOpenWatching: () -> Unit,
    onManageModules: () -> Unit,
    onOpenItem: (String) -> Unit,
) {
    // 周历选中态存日期字符串（CalendarDay 不是 Saveable，日期键是稳定身份）。
    var selectedDate by rememberSaveable { mutableStateOf(dashboard.selectedDay?.date) }
    var showModules by rememberSaveable { mutableStateOf(false) }
    // D10：真值由 ProfileModulePrefs 持有（启动期 bind），本页只观察；切换写盘后自动重组。
    val enabledModules by ProfileModulePrefs.enabled

    val selectedDay = dashboard.calendar.firstOrNull { it.date == selectedDate } ?: dashboard.selectedDay
    val openEntry: (WatchEntry) -> Unit = { onOpenItem(it.title) }
    // 日历是独立 Activity（页面外壳见 CalendarActivity）：本页只负责带日期启动，不持有日历状态。
    val context = LocalContext.current
    val openCalendar: (String?) -> Unit = { date ->
        val target = Intent(context, CalendarActivity::class.java)
        if (date != null) target.putExtra(CalendarActivity.EXTRA_DATE, date)
        context.startActivity(target)
    }

    // 渐变底由本页负责：LazyColumn 自身不画背景，滚动时底色不跟着位移。
    Box(modifier = Modifier.fillMaxSize().background(ProfilePageBackdrop)) {
        LazyColumn(
            modifier = Modifier.fillMaxSize(),
            contentPadding = PaddingValues(bottom = ShellInsets.NavReservedHeight),
            verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
        ) {
            item(key = "environment") {
                ProfileEnvironmentHeader(
                    title = dashboard.environmentName,
                    // 无日期进日历 = 落在「今天」；入口在环境头，不代表周历里的某一天。
                    onOpenCalendar = { openCalendar(null) },
                    onEdit = onEditEnvironment,
                )
            }
            if (ProfileModulePrefs.visibleBlock("week")) item(key = "week") {
                WeekStrip(
                    days = dashboard.calendar,
                    selected = selectedDay,
                    // 先落本地选中态（返回本页仍是这一天），再进日历并定位到该日。
                    onSelect = {
                        selectedDate = it.date
                        openCalendar(it.date)
                    },
                )
            }
            if (ProfileModulePrefs.visibleBlock("today")) item(key = "today") { TodaySummaryCard(dashboard.today) }
            if (ProfileModulePrefs.visibleBlock("history-title")) item(key = "history-title") { SectionTitle("历史观看") }
            if (ProfileModulePrefs.visibleBlock("history")) item(key = "history") { HistoryHeatmap(cells = dashboard.history) }
            if (ProfileModulePrefs.visibleBlock("watchlist")) item(key = "watchlist") {
                WatchlistSection(
                    entries = dashboard.watchlist,
                    onOpen = openEntry,
                    // 空态卡是纯引导文案（「在详情页加入待看」），这里没有跳转目标，显式 no-op。
                    onAdd = {},
                )
            }
            item(key = "collections-watching") {
                val showCollection = ProfileModulePrefs.visibleBlock("collections")
                val showWatching = ProfileModulePrefs.visibleBlock("watching")
                if (showCollection || showWatching) {
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(horizontal = DesignTokens.screenPad.dp),
                        horizontalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
                    ) {
                        if (showCollection) Box(Modifier.weight(1f)) {
                            CollectionSection(entries = dashboard.collections, onOpen = openEntry)
                        }
                        if (showWatching) Box(Modifier.weight(1f)) {
                            WatchingSection(
                                entries = dashboard.watching,
                                onOpen = openEntry,
                                onMore = onOpenWatching,
                            )
                        }
                    }
                }
            }
            if (ProfileModulePrefs.visibleBlock("monthly")) item(key = "monthly") {
                MonthlyChart(
                    label = dashboard.monthLabel,
                    points = dashboard.monthPoints,
                    hours = dashboard.monthHours,
                    delta = dashboard.monthDelta,
                )
            }
            item(key = "top-shows") { TopShowsCard(dashboard.topShows) }
            if (ProfileModulePrefs.visibleBlock("genre-title")) item(key = "genre-title") { SectionTitle("类型分布") }
            if (ProfileModulePrefs.visibleBlock("genres")) item(key = "genres") {
                GenreDonut(
                    slices = dashboard.genres,
                    // 环心总时长取本月（基准图 1.7h 与「本月观看」同值）；部/集数只有今日口径可用。
                    totalHours = dashboard.monthHours,
                    movieCount = dashboard.today.movies,
                    episodeCount = dashboard.today.episodes,
                )
            }
            trendsItems(dashboard.trends)
            item(key = "manage") {
                ManageModulesPill(
                    onClick = {
                        onManageModules()
                        showModules = true
                    },
                    modifier = Modifier.padding(top = ManagePillTopGap),
                )
            }
        }

        if (showModules) {
            ModuleManagerSheet(
                enabled = enabledModules,
                onToggle = { module, checked -> ProfileModulePrefs.setEnabled(module, checked) },
                onDismiss = { showModules = false },
            )
        }
    }
}

/** 趋势卡逐条入列（数量由数据决定，不固定 2 条）：key 带序号，避免同名统计项撞 key。 */
private fun androidx.compose.foundation.lazy.LazyListScope.trendsItems(trends: List<TrendStat>) {
    trends.forEachIndexed { index, stat ->
        if (ProfileModulePrefs.visibleTrend(index)) {
            item(key = "trend-$index-${stat.name}") { TrendCard(stat) }
        }
    }
}


/** ④ 小节标题：「历史观看」16sp Bold，左右 13dp；与热力图卡片之间靠 LazyColumn 的 10dp 间距。 */
@Composable
private fun SectionTitle(title: String) {
    Text(
        text = title,
        fontSize = SectionTitleSize,
        fontWeight = FontWeight.Bold,
        color = DesignTokens.textMain,
        modifier = Modifier.fillMaxWidth().padding(horizontal = DesignTokens.screenPad.dp),
    )
}


private val SectionTitleSize = 16.sp
private val ManagePillTopGap = 8.dp

// ---- Preview：完整样例数据见 ProfilePagePreviewData.kt（不放在页面文件里，守 300 行上限）。 ----

@Preview(name = "我的 · 完整样例", widthDp = 390, heightDp = 900, showBackground = true)
@Composable
private fun PreviewProfileFull() {
    ProfilePage(
        dashboard = previewDashboard(),
        onEditEnvironment = {},
        onOpenWatching = {},
        onManageModules = {},
        onOpenItem = {},
    )
}