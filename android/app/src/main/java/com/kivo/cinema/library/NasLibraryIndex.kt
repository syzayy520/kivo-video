package com.kivo.cinema.library

import android.content.Context
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore

/**
 * NAS 片库索引的「可用条目」读取：读本地索引，再剔除已删除 / 已隐藏的源。
 *
 * 首页分类行（refreshNasLibrary）与「查看全部」分类页共用这一份口径 —— 源被删 / 隐藏后，
 * 索引里的旧条目必须作废，否则会挂出点开就失败的卡片。只读磁盘，不扫描、不碰 UI；须在 IO 线程调用。
 */
object NasLibraryIndex {
    fun aliveVideos(context: Context, store: NasIndexStore = NasIndexStore(context)): List<NasVideo> {
        val videos = store.read()
        val alive = runCatching {
            SourceRepository(SourceStore(context)).read().getOrNull()?.filter { !it.hidden }?.map { it.id }?.toSet()
        }.getOrNull() ?: return videos
        return videos.filter { it.sourceId in alive }
    }
}
