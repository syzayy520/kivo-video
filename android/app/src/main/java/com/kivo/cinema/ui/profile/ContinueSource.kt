package com.kivo.cinema.ui.profile

import android.content.Context
import com.kivo.cinema.library.ContinueWatching
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.ui.profile.model.WatchEntry
import com.kivo.cinema.ui.profile.thumbs.VideoThumbnails

/**
 * 「我的」看板的在看数据源（PROF-03）：与发现页/在看覆盖层**同一份**对齐结果，
 * 不再直接用 [WatchStore.recentKeys]（那会把网络 key 当条目，与覆盖层的空态互相矛盾）。
 *
 * 对齐不上的条目整条丢弃；片名由 [ProfileNames] 解析，任何情况下都不把 URL 上屏。
 *
 * 阻塞调用（MediaStore + 取帧）：请在 `Dispatchers.IO` 上调用。
 */
object ContinueSource {

    /** 只为前 6 条生成缩略图：20 条各取一帧会把页面首帧拖到秒级。 */
    private const val THUMB_BUDGET = 6

    private const val WATCHED_PREFIX = "看到 "

    private const val MILLIS_MINUTE = 60
    private const val MILLIS_HOUR = 3600
    private const val FULL_PROGRESS_PERCENT = 100

    /**
     * @param aligned Shell 侧已对齐的列表（`HomeShellState.continueItems`）；
     *   传 null 时按同一对齐规则现算（二者结果一致）。
     */
    fun continueEntries(
        context: Context,
        aligned: List<Pair<MediaItem, WatchStore.Progress>>? = null,
    ): List<WatchEntry> {
        val app = context.applicationContext
        val pairs = aligned ?: ContinueWatching.aligned(
            app,
            WatchStore(app),
            ContinueWatching.DEFAULT_LIMIT,
        )
        return pairs.mapIndexed { index, (item, progress) ->
            WatchEntry(
                title = ProfileNames.display(item.path, item.title),
                subtitle = WATCHED_PREFIX + seenText(progress.positionMs),
                posterPath = if (index < THUMB_BUDGET) {
                    VideoThumbnails.framePath(app, item.path)
                } else null,
                progressPercent = percentOf(progress),
            )
        }
    }

    private fun percentOf(progress: WatchStore.Progress): Int? {
        if (progress.durationMs <= 0L) return null
        return ((progress.positionMs * FULL_PROGRESS_PERCENT) / progress.durationMs)
            .toInt().coerceIn(0, FULL_PROGRESS_PERCENT)
    }

    /** 与发现页 `fmtSeen` 同口径；在数据层自持，避免依赖 UI 包。 */
    private fun seenText(positionMs: Long): String {
        val seconds = positionMs / 1000
        return if (seconds >= MILLIS_HOUR) {
            "%d:%02d:%02d".format(
                seconds / MILLIS_HOUR,
                seconds / MILLIS_MINUTE % MILLIS_MINUTE,
                seconds % MILLIS_MINUTE,
            )
        } else {
            "%d:%02d".format(seconds / MILLIS_MINUTE, seconds % MILLIS_MINUTE)
        }
    }
}