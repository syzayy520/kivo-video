package com.kivo.cinema.ui.design

/**
 * 共用时间格式化（236 设计稿 §三）。
 * 「我的」页已迁出到 `ui/profile/ProfilePage.kt`（B2 卡）；设置页在 `ui/design/settings/`；
 * 资源库页在 `SourcesScreen`；发现页在 `DiscoverPage.kt`；在看页在 `WatchingPage.kt`。
 */

/** 相对时间（t26/0083 帧：「3 天前 · 来源」前半）。WatchingPage/DiscoverPage 共用。 */
internal fun relDays(addedSec: Long): String {
    val days = (System.currentTimeMillis() / 1000 - addedSec) / 86400
    return when {
        days <= 0L -> "今天"
        days == 1L -> "昨天"
        days < 30 -> "" + days + " 天前"
        else -> formatDate(addedSec)
    }
}

internal fun fmtSeen(positionMs: Long): String {
    val s = positionMs / 1000
    return if (s >= 3600) "%d:%02d:%02d".format(s / 3600, s / 60 % 60, s % 60)
    else "%d:%02d".format(s / 60, s % 60)
}

internal fun formatDate(addedSec: Long): String {
    val d = java.util.Calendar.getInstance()
    d.timeInMillis = addedSec * 1000L
    return "%04d-%02d-%02d".format(
        d.get(java.util.Calendar.YEAR), d.get(java.util.Calendar.MONTH) + 1,
        d.get(java.util.Calendar.DAY_OF_MONTH))
}