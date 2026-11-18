package com.kivo.cinema.smb.scan

import com.kivo.cinema.smb.SmbSession

/**
 * 单个共享内的片库扫描：BFS 遍历，带**深度/数量上限**与逐目录容错。
 *
 * 单一职责：只做"从一个已连接的共享里把影片路径找出来"，不建连接、不写索引、不碰 UI。
 * 阻塞调用（每层都走 SMB 列目录）：必须在 IO 线程执行。
 *
 * 容错口径：某个子目录 list 失败（权限/网络抖动）只跳过该目录，不中断整次扫描——
 * 宁可少几部片，也不让首页因为一个坏目录整块空掉。
 */
class SmbLibraryScanner(private val session: SmbSession) {

    data class Found(val path: String, val size: Long)

    /**
     * @param roots 共享内起始目录（如 `["电影","剧集"]`；空串 = 共享根）
     * @param maxDepth 相对 roots 的最大下探层数（真机实测 电影/片名目录/文件 = 2，留 4 兜底）
     * @param maxFiles 总条数上限，防止剧集目录把扫描拖成分钟级
     */
    fun scan(
        roots: List<String>,
        maxDepth: Int = 4,
        maxFiles: Int = 3000,
        onProgress: (found: Int) -> Unit = {},
    ): List<Found> {
        val out = ArrayList<Found>(256)
        val queue = ArrayDeque<Pair<String, Int>>()
        roots.forEach { queue.add(it.trim('/') to 0) }

        while (queue.isNotEmpty() && out.size < maxFiles) {
            val (dir, depth) = queue.removeFirst()
            val entries = runCatching { session.list(dir) }.getOrNull() ?: continue
            for (entry in entries) {
                val child = if (dir.isEmpty()) entry.name else "$dir/${entry.name}"
                if (entry.isDirectory) {
                    if (depth < maxDepth && !VideoFileFilter.skipDir(entry.name)) {
                        queue.add(child to depth + 1)
                    }
                } else if (VideoFileFilter.isVideo(entry.name) && !VideoFileFilter.isNoise(entry.name)) {
                    out.add(Found(child, entry.size))
                    if (out.size >= maxFiles) break
                }
            }
            onProgress(out.size)
        }
        return out
    }
}
