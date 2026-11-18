package com.kivo.cinema.library

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 片库聚合的回归测试。
 *
 * 为什么值得测：这层的错误**只表现为"首页多几张卡 / 少几张卡"**，真机上很难发现是 bug
 * （用户只会觉得"怎么重复了"或"怎么少了一部"）。所以判据必须钉在单测里。
 *
 * 用例全部来自真机 NAS 的真实目录结构（12T 共享）。
 */
class NasLibraryGroupingTest {

    private fun v(path: String, cat: NasCategory = NasCategory.Tv) =
        NasVideo("src", "12T", path, 1000L, 1_000L, cat)

    @Test
    fun `同目录多集聚合成一部剧，不再一集一张卡`() {
        val items = NasLibraryGrouping.group(
            listOf(
                v("剧集/繁花/繁花.第01集.mkv"),
                v("剧集/繁花/繁花.第02集.mkv"),
                v("剧集/繁花/繁花.第03集.mkv"),
            ),
        )
        assertEquals(1, items.size)
        val s = (items[0] as NasLibraryGrouping.Item.Series).series
        assertEquals(3, s.count)
        assertEquals("繁花", s.title)
        assertEquals(1, s.seasonCount)
    }

    @Test
    fun `单个文件保持单片`() {
        val items = NasLibraryGrouping.group(listOf(v("电影/某电影.2024.mkv", NasCategory.Movie)))
        assertEquals(1, items.size)
        assertTrue(items[0] is NasLibraryGrouping.Item.Single)
    }

    /** 电影目录常见"正片 + 花絮"，两者都没有集号 ⇒ 不能被并成"2 集的剧"。 */
    @Test
    fun `电影正片加花絮不并成剧`() {
        val items = NasLibraryGrouping.group(
            listOf(
                v("电影/某电影/正片.mkv", NasCategory.Movie),
                v("电影/某电影/花絮.mkv", NasCategory.Movie),
            ),
        )
        assertEquals(2, items.size)
        assertTrue(items.all { it is NasLibraryGrouping.Item.Single })
    }

    /** 真机结构：American.Horror.Story.S01/S02 各自一个目录 ⇒ 必须合成一部 2 季的剧。 */
    @Test
    fun `多季目录按剧名合并成一部剧`() {
        val items = NasLibraryGrouping.group(
            listOf(
                v("剧集/Some.Show.S01.1080p/Some.Show.S01E01.mkv"),
                v("剧集/Some.Show.S01.1080p/Some.Show.S01E02.mkv"),
                v("剧集/Some.Show.S02.1080p/Some.Show.S02E01.mkv"),
                v("剧集/Some.Show.S02.1080p/Some.Show.S02E02.mkv"),
            ),
        )
        assertEquals(1, items.size)
        val s = (items[0] as NasLibraryGrouping.Item.Series).series
        assertEquals(2, s.seasonCount)
        assertEquals(4, s.count)
        assertEquals("Some Show", s.title)
        assertEquals(listOf(1, 2), s.seasons.map { it.index })
    }

    /** 综艺常用"日期当集号"（`节目名.20240101.mp4`），认不出就退化成"一期一张卡"。 */
    @Test
    fun `综艺日期期号被识别并聚合`() {
        val items = NasLibraryGrouping.group(
            listOf(
                v("综艺/某节目/某节目.20240101.mp4", NasCategory.Variety),
                v("综艺/某节目/某节目.20240108.mp4", NasCategory.Variety),
            ),
        )
        assertEquals(1, items.size)
        assertEquals(2, (items[0] as NasLibraryGrouping.Item.Series).series.count)
    }

    /** 动漫常见 `EP01`。 */
    @Test
    fun `动漫EP集号被识别`() {
        val items = NasLibraryGrouping.group(
            listOf(
                v("动漫/某番/[Group] Show - EP01 [1080p].mkv", NasCategory.Anime),
                v("动漫/某番/[Group] Show - EP02 [1080p].mkv", NasCategory.Anime),
            ),
        )
        assertEquals(1, items.size)
        assertEquals(2, (items[0] as NasLibraryGrouping.Item.Series).series.count)
    }

    /** 跨分类不能串：电影目录里的集号文件仍归电影分类（分类由根目录决定，聚合不改变它）。 */
    @Test
    fun `聚合不改变条目所属分类`() {
        val items = NasLibraryGrouping.group(
            listOf(
                v("纪录片/某纪录片/某纪录片.第01集.mkv", NasCategory.Documentary),
                v("纪录片/某纪录片/某纪录片.第02集.mkv", NasCategory.Documentary),
            ),
        )
        assertEquals(NasCategory.Documentary, items[0].category)
    }

    @Test
    fun `剧键可在详情页还原坐标`() {
        val key = "smb-series://src/12T/剧集|Some Show"
        assertTrue(NasSeries.isSeriesKey(key))
        val c = NasSeries.parseKey(key)!!
        assertEquals("src", c.sourceId)
        assertEquals("12T", c.share)
        assertEquals("剧集", c.rootDir)
        assertEquals("Some Show", c.title)
    }
}
