package com.kivo.cinema.library

import android.content.Context
import android.provider.MediaStore as MS

/**
 * 本地视频来源（240 §9.2）。
 *
 * 唯一职责：从 MediaStore 取本地视频列表。
 * 不做匹配、不做格式化、不碰 UI —— 匹配见 [TitleMatch]，推导见 `ui/detail/MediaMatch`。
 */
object LocalVideoSource {

    private val PROJECTION = arrayOf(
        MS.Video.Media._ID,
        MS.Video.Media.DISPLAY_NAME,
        MS.Video.Media.DATA,
        MS.Video.Media.DURATION,
        MS.Video.Media.WIDTH,
        MS.Video.Media.HEIGHT,
        MS.Video.Media.DATE_ADDED,
    )

    /** 返回按加入时间倒序的本地视频；查询失败返回空表（调用方按「无资源」处理）。 */
    fun query(context: Context): List<MediaItem> {
        val out = mutableListOf<MediaItem>()
        runCatching {
            context.contentResolver.query(
                MS.Video.Media.EXTERNAL_CONTENT_URI, PROJECTION, null, null,
                "${MS.Video.Media.DATE_ADDED} DESC",
            )?.use { c ->
                while (c.moveToNext()) {
                    out.add(
                        MediaItem(
                            id = c.getLong(0),
                            title = c.getString(1) ?: "",
                            path = c.getString(2) ?: "",
                            durationMs = c.getLong(3),
                            width = c.getInt(4),
                            height = c.getInt(5),
                            addedSec = c.getLong(6),
                        )
                    )
                }
            }
        }
        return out
    }
}
