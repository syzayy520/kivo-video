package com.kivo.cinema.library

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 「在看」聚合的回归测试。
 *
 * 为什么值得测：聚合错误的唯一表现是"列表里同一部剧重复出现几次"或"两部剧被并成一条"，
 * 真机上一眼看不出来（用户只会觉得乱）。判据必须钉在单测里。
 */
class WatchingGroupingTest {

    private fun smb(path: String, at: Long = 1_000L) = HomeEntry(
        key = "smb://src/12T/$path",
        title = path.substringAfterLast('/'),
        sourceLabel = "NAS",
        addedSec = 0L,
        positionMs = 60_000L,
        durationMs = 600_000L,
        kind = HomeEntry.Kind.Smb,
        lastPlayedMs = at,
    )

    private fun emby(id: String, title: String, at: Long = 1_000L) = HomeEntry(
        key = "emby:s1:$id",
        title = title,
        sourceLabel = "Emby",
        addedSec = 0L,
        positionMs = 60_000L,
        durationMs = 600_000L,
        kind = HomeEntry.Kind.Emby,
        lastPlayedMs = at,
        embyItemId = id,
        embySourceId = "s1",
    )

    @Test
    fun `同一部剧的多集合成一行，代表取最近看的`() {
        val rows = WatchingGrouping.rows(
            listOf(
                smb("剧集/Some.Show.S01.1080p/Some.Show.S01E03.mkv", at = 3_000),
                smb("剧集/Some.Show.S01.1080p/Some.Show.S01E02.mkv", at = 2_000),
                smb("剧集/Some.Show.S01.1080p/Some.Show.S01E01.mkv", at = 1_000),
            ),
        )
        assertEquals(1, rows.size)
        val row = rows[0]
        assertEquals("Some Show", row.title)
        assertEquals(3, row.entryCount)
        assertTrue(row.isSeries)
        // 代表 = 最近看的 E03
        assertTrue(row.latest.key.endsWith("S01E03.mkv"))
        assertEquals("S01E03", row.episodeLabel)
    }

    @Test
    fun `多季目录也能合成一行（跨季合并且与片库同口径）`() {
        val rows = WatchingGrouping.rows(
            listOf(
                smb("剧集/Some.Show.S02.1080p/Some.Show.S02E01.mkv", at = 2_000),
                smb("剧集/Some.Show.S01.1080p/Some.Show.S01E01.mkv", at = 1_000),
            ),
        )
        assertEquals(1, rows.size)
        assertEquals(2, rows[0].entryCount)
    }

    @Test
    fun `不同剧各自成行`() {
        val rows = WatchingGrouping.rows(
            listOf(
                smb("剧集/Show.A/Show.A.S01E01.mkv", at = 2_000),
                smb("剧集/Show.B/Show.B.S01E01.mkv", at = 1_000),
            ),
        )
        assertEquals(2, rows.size)
        assertFalse(rows[0].isSeries)
    }

    /** 电影没有集号 ⇒ 每条独立，绝不能被"父目录 + 片名"误并。 */
    @Test
    fun `电影各自成行不聚合`() {
        val rows = WatchingGrouping.rows(
            listOf(
                smb("电影/Movie.One.2024.mkv", at = 2_000),
                smb("电影/Movie.Two.2023.mkv", at = 1_000),
            ),
        )
        assertEquals(2, rows.size)
        assertTrue(rows.all { !it.isSeries })
    }

    /** 同一目录下的"正片 + 花絮"都没有集号 ⇒ 也必须各自成行。 */
    @Test
    fun `同目录无集号的两条不聚合`() {
        val rows = WatchingGrouping.rows(
            listOf(
                smb("电影/Movie/正片.mkv", at = 2_000),
                smb("电影/Movie/花絮.mkv", at = 1_000),
            ),
        )
        assertEquals(2, rows.size)
    }

    @Test
    fun `Emby 条目保持独立不参与聚合`() {
        val rows = WatchingGrouping.rows(
            listOf(
                emby("1", "第 3 集", at = 2_000),
                emby("2", "第 2 集", at = 1_000),
            ),
        )
        assertEquals(2, rows.size)
    }

    @Test
    fun `中文集号也被识别`() {
        val rows = WatchingGrouping.rows(
            listOf(
                smb("剧集/繁花/繁花.第03集.mkv", at = 2_000),
                smb("剧集/繁花/繁花.第02集.mkv", at = 1_000),
            ),
        )
        assertEquals(1, rows.size)
        assertEquals(2, rows[0].entryCount)
        assertEquals("第03集", rows[0].episodeLabel)
    }

    @Test
    fun `输出按最近观看倒序`() {
        val rows = WatchingGrouping.rows(
            listOf(
                smb("剧集/Show.A/Show.A.S01E01.mkv", at = 1_000),
                smb("剧集/Show.B/Show.B.S01E01.mkv", at = 9_000),
            ),
        )
        assertEquals(2, rows.size)
        assertTrue(rows[0].latest.key.contains("Show.B"))
    }
}
