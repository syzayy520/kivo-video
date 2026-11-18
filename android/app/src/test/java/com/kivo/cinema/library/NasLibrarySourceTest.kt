package com.kivo.cinema.library

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 首页分类行切分测试。
 *
 * 这是"首页到底显示哪几行、每行几部"的唯一决策点：
 * 过滤写错 → 关掉的分类还在显示；切分写错 → 一部片出现在两行，或空行占位。
 */
class NasLibrarySourceTest {

    private fun video(category: NasCategory, name: String, poster: String? = null) = NasVideo(
        sourceId = "s1", share = "12T", path = "${category.label}/$name",
        size = 1024L, addedMs = 0L, category = category, posterPath = poster,
    )

    @Test
    fun `只输出可见且有内容的分类`() {
        val videos = listOf(
            video(NasCategory.Movie, "A.mkv"),
            video(NasCategory.Tv, "B.S01E01.mkv"),
            video(NasCategory.Anime, "C.mkv"),
        )
        // 只开电影与剧集
        val sections = NasLibrarySource.sections(videos) { it == NasCategory.Movie || it == NasCategory.Tv }
        assertEquals(2, sections.size)
        assertEquals(NasCategory.Movie, sections[0].category)
        assertEquals(NasCategory.Tv, sections[1].category)
    }

    @Test
    fun `顺序恒为 ordered 而非输入顺序`() {
        val videos = listOf(
            video(NasCategory.Anime, "C.mkv"),
            video(NasCategory.Movie, "A.mkv"),
        )
        val sections = NasLibrarySource.sections(videos) { true }
        assertEquals(listOf(NasCategory.Movie, NasCategory.Anime), sections.map { it.category })
    }

    @Test
    fun `没有内容的分类不产出空行`() {
        val videos = listOf(video(NasCategory.Movie, "A.mkv"))
        val sections = NasLibrarySource.sections(videos) { true }
        assertEquals(1, sections.size)
        assertEquals(NasCategory.Movie, sections[0].category)
    }

    @Test
    fun `limit 截断每行条目数`() {
        val videos = (1..20).map { video(NasCategory.Movie, "M$it.mkv") }
        val sections = NasLibrarySource.sections(videos, limit = 5) { true }
        assertEquals(5, sections.single().entries.size)
    }

    @Test
    fun `书籍条目带上续播键与库内封面`() {
        val videos = listOf(video(NasCategory.Movie, "A Beautiful Mind.2001.mkv", poster = "电影/A/poster.jpg"))
        val entry = NasLibrarySource.sections(videos) { true }.single().entries.single()
        assertTrue(entry.key.startsWith("smb://s1/12T/"))
        assertEquals("电影/A/poster.jpg", entry.smbPosterPath)
        assertEquals("A Beautiful Mind", entry.title)
        assertEquals(2001, entry.year)
    }

    @Test
    fun `空索引不崩`() {
        assertTrue(NasLibrarySource.sections(emptyList()) { true }.isEmpty())
    }
}
