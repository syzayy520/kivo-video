package com.kivo.cinema.ui.design.home

import android.content.Context
import android.content.Intent
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.NasSeries
import com.kivo.cinema.library.SmbContinueKey
import com.kivo.cinema.ui.detail.MovieDetailActivity

/**
 * NAS 片库条目 → 详情页 Intent（首页分类行与「查看全部」分类页共用一份路由）。
 *
 * 剧（`smb-series://` 键）进「详情 + 选集」，单片进单片详情；键解析失败返回 null，由调用方提示原因。
 */
internal fun nasEntryIntent(context: Context, entry: HomeEntry): Intent? {
    NasSeries.parseKey(entry.key)?.let { c ->
        return MovieDetailActivity.smbIntent(context, c.sourceId, c.share, c.rootDir, isSeries = true, title = c.title)
    }
    val p = SmbContinueKey.parse(entry.key) ?: return null
    return MovieDetailActivity.smbIntent(context, p.sourceId, p.share, p.path, isSeries = false)
}
