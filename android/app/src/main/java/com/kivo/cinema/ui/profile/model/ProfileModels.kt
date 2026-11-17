package com.kivo.cinema.ui.profile.model

/**
 * 「我的」看板数据模型（B2-T1 冻结契约）。
 *
 * 这些字段是 UI 层唯一可消费的形状：图表与列表不得自行扩展字段。
 * 缺失的数据一律用 `null` / `0` / 空集合表达，绝不补假值（见 `ProfileStats` 的 KDoc）。
 */

/** 周日历一格。播出日程数据源接入前 [hasAiring] 恒 false（无数据 ≠ 有播出）。 */
data class CalendarDay(val label: String, val date: String, val hasAiring: Boolean, val isToday: Boolean)

/** 今日观看摘要。[hourBuckets] 恒 24 槽；无小时级采样时全 0。 */
data class TodaySummary(val hours: Float, val movies: Int, val episodes: Int, val hourBuckets: List<Int>)

/** 历史热力图一格：week 0 为最早一周，day 0 为周日，level 0..4。 */
data class HistoryCell(val week: Int, val day: Int, val level: Int)

/** 列表条目（待看 / 收藏 / 在看通用）。[progressPercent] 仅「在看」有值。 */
data class WatchEntry(val title: String, val subtitle: String, val posterPath: String?, val progressPercent: Int?)

data class MonthlyPoint(val day: Int, val minutes: Int)

data class TopShow(val title: String, val hours: Float)

data class GenreSlice(val name: String, val percent: Int)

data class TrendStat(val name: String, val hours: Float, val delta: Float)

/** 「我的」模块开关；声明顺序即渲染顺序，12 项与竞品模块清单一致。 */
enum class ProfileModule(val label: String, val defaultOn: Boolean) {
    Calendar("日历", true), Overview("观看概览", true), History("历史观看", true),
    Watchlist("待看", true), Collection("收藏", true), Watching("在看", true),
    Subscription("订阅", true), Download("下载", true), Monthly("本月观看", true),
    Genre("类型分布", true), Trend1("分类走势 1", true), Trend2("分类走势 2", true);
}

/**
 * 影院身份（「我的」头图数据，P1 冻结契约）。
 *
 * - [name] / [avatarText]：目前没有账号体系与头像上传，由聚合层给固定默认值（不是读出来的用户数据）；
 * - [sourceCount]：已配置的**可见**源数量（隐藏源不计，与源管理页一致）；源存储读不到时 0；
 * - [online]：**不是**服务器可达性。没有在线探测（禁止为头图发网络请求），
 *   现有健康判断 `SourceSummaries.Summary.online` 只存在于内存、且需要一次真实请求才能产生，
 *   这里拿不到 ⇒ 退化为「已配置至少一个可见源」；
 * - [weeklyHours] / [streakDays]：来自 [com.kivo.cinema.stats.WatchStore] 的按天累计。
 */
data class ProfileIdentity(
    val name: String,
    val avatarText: String,
    val sourceCount: Int,
    val online: Boolean,
    val weeklyHours: Float,
    val streakDays: Int,
)

/**
 * 一张用户片单。[covers] 是封面图路径（本地文件或 http），空列表表示没有可用封面。
 *
 * 目前没有片单存储 ⇒ 聚合层恒返回空集合（见 `ProfileStats.build`），不造假。
 */
data class CollectionCard(val id: String, val name: String, val count: Int, val covers: List<String>)

/**
 * 一条周报（P2 冻结契约，「我的」第 4 层「本周影院报告」）。
 *
 * - [headline]：结论句，由 `ProfileStats.weeklyHeadline` 纯函数生成（三条分支见该函数 KDoc）；
 * - [hours]：本周（周一→周日）小时，**不做一位小数舍入**（保留聚合精度），格式化交给 UI；
 * - [titles]：本周部数；[com.kivo.cinema.stats.WatchStore] 只有按天毫秒与续播位，
 *   没有按片时长/完播统计 ⇒ 恒 0，不伪造（KDoc 之外没有旁路能补出真值）；
 * - [streakDays]：与 [ProfileIdentity.streakDays] 同源同口径（从今天往前连续有记录的天数）；
 * - [dailyMinutes]：恒 7 项，周一→周日，单位分钟（整数截断）；本周未到的日期与无记录日为 0；
 * - [genres]：复用 [GenreSlice]；仓库没有类型数据源 ⇒ 当前恒空（由 UI 渲染空态）。
 */
data class WeeklyInsight(
    val headline: String,
    val hours: Float,
    val titles: Int,
    val streakDays: Int,
    val dailyMinutes: List<Int>,
    val genres: List<GenreSlice>,
)

/** 一次聚合产出的整页快照；各字段的来源与空值语义见 `ProfileStats.build`。 */
data class ProfileDashboard(
    /**
     * 旧「默认环境」标题。P1 起头图改用 [identity]，本字段已无 UI 消费方，
     * 保留只为不破坏既有构造点（P3 与旧 section 一起删除）；默认值与聚合层的口径一致。
     */
    val environmentName: String = "本地",
    val calendar: List<CalendarDay>,
    val selectedDay: CalendarDay?,
    val today: TodaySummary,
    val history: List<HistoryCell>,
    val watchlist: List<WatchEntry>,
    val collections: List<WatchEntry>,
    val watching: List<WatchEntry>,
    val monthLabel: String,
    val monthPoints: List<MonthlyPoint>,
    val monthHours: Float,
    val monthDelta: Float,
    val topShows: List<TopShow>,
    val genres: List<GenreSlice>,
    val trends: List<TrendStat>,
    val modules: List<ProfileModule>,
    /** 影院身份（P1 头图）。默认值与「无源、无观看」的空影院一致，旧构造点不破。 */
    val identity: ProfileIdentity = ProfileIdentity("我的影院", "K", 0, false, 0f, 0),
    /**
     * 用户片单。目前没有片单存储 ⇒ 恒为空（P2 接片单后再填）。
     *
     * 命名偏差（有意为之）：契约原文写的是 `collections`，但既有字段 [collections]
     * （`List<WatchEntry>`，旧「收藏」section）在被 `ProfilePage` 消费，
     * 而本轮禁止改 `ProfilePage`；同名同类型无法共存，故新字段用 [collectionCards]，
     * 待 P3 删除旧 section 的卡片再决定是否合并命名。
     */
    val collectionCards: List<CollectionCard> = emptyList(),
    /** 本周影院报告（P2）。默认值为「无数据」形状，旧构造点不破。 */
    val weekly: WeeklyInsight = WeeklyInsight("", 0f, 0, 0, emptyList(), emptyList()),
)