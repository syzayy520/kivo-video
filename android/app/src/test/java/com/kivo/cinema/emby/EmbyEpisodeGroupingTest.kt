package com.kivo.cinema.emby

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/**
 * D366：一次全量拉集的分季与**覆盖判定**。
 *
 * 这层逻辑值钱的点：服务器一旦分页截断，拿到的集会少 —— 少一集在 UI 上看不出来，
 * 但选集面板会缺集。所以"不完整就返回 null 让调用方退回逐季请求"必须被锁住。
 */
class EmbyEpisodeGroupingTest {

    private fun season(id: String, name: String, index: Int) = EmbyShows.Season(id, name, index)

    private fun episode(id: String, seasonIndex: Int, index: Int) = EmbyShows.Episode(
        id = id, name = "E$index", index = index, seasonIndex = seasonIndex,
        runtimeMinutes = 45, thumbUrl = "", played = false, positionTicks = 0L,
        airDate = "", overview = "",
    )

    @Test
    fun `多季全量按季号分组，顺序与季表一致`() {
        val seasons = listOf(season("s1", "第1季", 1), season("s2", "第2季", 2))
        val all = listOf(
            episode("e201", 2, 1), episode("e101", 1, 1), episode("e202", 2, 2),
        )
        val grouped = EmbyEpisodeGrouping.group(seasons, all)!!
        assertEquals(listOf("第1季", "第2季"), grouped.map { it.first.name })
        assertEquals(listOf("e101"), grouped[0].second.map { it.id })
        assertEquals(listOf("e201", "e202"), grouped[1].second.map { it.id })
    }

    @Test
    fun `某一季一集都没拿到时判定不完整，返回 null 由调用方回退`() {
        val seasons = listOf(season("s1", "第1季", 1), season("s2", "第2季", 2))
        val truncated = listOf(episode("e101", 1, 1))
        assertNull(EmbyEpisodeGrouping.group(seasons, truncated))
    }

    @Test
    fun `全量结果为空时同样判定不完整`() {
        val seasons = listOf(season("s1", "第1季", 1))
        assertNull(EmbyEpisodeGrouping.group(seasons, emptyList()))
    }

    @Test
    fun `季表为空时返回空分组而不是 null`() {
        assertEquals(emptyList<Pair<EmbyShows.Season, List<EmbyShows.Episode>>>(), 
            EmbyEpisodeGrouping.group(emptyList(), listOf(episode("e1", 1, 1))))
    }

    @Test
    fun `多余季号的集不影响覆盖判定`() {
        val seasons = listOf(season("s1", "第1季", 1))
        val all = listOf(episode("e101", 1, 1), episode("e001", 0, 1))
        val grouped = EmbyEpisodeGrouping.group(seasons, all)!!
        assertEquals(1, grouped.size)
        assertEquals(listOf("e101"), grouped[0].second.map { it.id })
    }
}
