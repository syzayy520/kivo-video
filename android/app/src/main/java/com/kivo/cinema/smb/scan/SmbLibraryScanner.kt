package com.kivo.cinema.smb.scan

import com.kivo.cinema.library.NasCategory
import com.kivo.cinema.smb.SmbSession

/**
 * 单个共享内的片库扫描：BFS 遍历，带**深度/数量上限**与逐目录容错。
 *
 * 单一职责：只做"从一个已连接的共享里把影片路径找出来（顺带找同目录封面）"，
 * 不建连接、不写索引、不碰 UI、不联网。阻塞调用（每层都走 SMB 列目录）：必须在 IO 线程执行。
 *
 * 容错口径：某个子目录 list 失败（权限/网络抖动）只跳过该目录，不中断整次扫描——
 * 宁可少几部片，也不让首页因为一个坏目录整块空掉。
 *
 * 【封面关联规则】只有该目录**恰好 1 个视频**时才把同目录图片当作它的封面。
 * 多视频目录共用一个 poster 会造成张冠李戴，宁可不关联（让上层兜底 TMDB）。
 */
class SmbLibraryScanner(private val session: SmbSession) {

    data class Found(
        val path: String,
        val size: Long,
        val category: NasCategory,
        val posterPath: String? = null,
    )

    /** 一根待扫目录：共享内路径 + 它代表的分类（分类由根目录名决定，向下继承）。 */
    data class Root(val path: String, val category: NasCategory)

    fun scan(
        roots: List<Root>,
        maxDepth: Int = 4,
        maxFiles: Int = 3000,
        onProgress: (found: Int) -> Unit = {},
    ): List<Found> {
        val out = ArrayList<Found>(256)
        val queue = ArrayDeque<Triple<String, Int, NasCategory>>()
        roots.forEach { queue.add(Triple(it.path.trim('/'), 0, it.category)) }

        while (queue.isNotEmpty() && out.size < maxFiles) {
            val (dir, depth, category) = queue.removeFirst()
            val entries = runCatching { session.list(dir) }.getOrNull() ?: continue

            val videos = entries.filter {
                !it.isDirectory && VideoFileFilter.isVideo(it.name) && !VideoFileFilter.isNoise(it.name)
            }
            val posterPath = if (videos.size == 1) {
                val images = entries.filter { !it.isDirectory && VideoFileFilter.isImage(it.name) }
                    .map { it.name to it.size }
                VideoFileFilter.pickPoster(images)?.let { if (dir.isEmpty()) it else "$dir/$it" }
            } else null

            for (v in videos) {
                if (out.size >= maxFiles) break
                val child = if (dir.isEmpty()) v.name else "$dir/${v.name}"
                out.add(Found(child, v.size, category, posterPath))
            }
            for (e in entries) {
                if (!e.isDirectory) continue
                if (depth >= maxDepth || VideoFileFilter.skipDir(e.name)) continue
                val child = if (dir.isEmpty()) e.name else "$dir/${e.name}"
                queue.add(Triple(child, depth + 1, category))
            }
            onProgress(out.size)
        }
        return out
    }
}
