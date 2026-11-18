package com.kivo.cinema.library

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/**
 * 片名解析的回归测试。
 *
 * 为什么值得测：首页「本地片库」的片名与 TMDB 搜索词全靠它——
 * 季集没剥干净、中文名被英文名挤掉、标签没去干净，都会让封面搜不到（而且线上看不出来，只表现为"没图"）。
 * 这些用例来自真机 NAS 的真实文件名。
 */
class MediaTitleParserTest {

    @Test
    fun `方括号中文名优先，英文名单独留作搜索别名`() {
        val p = MediaTitleParser.parse(
            "[犯罪都市4].The.Roundup.Punishment.2024.Hami.WEB-DL.1080p.H264.AAC-CMCTV.mkv",
        )
        assertEquals("犯罪都市4", p.title)
        assertEquals(2024, p.year)
        assertEquals("The Roundup Punishment", p.english)
    }

    @Test
    fun `剧集剥掉季集标记`() {
        val p = MediaTitleParser.parse(
            "69两头勾.6ixtynin9.The.Series.S01E01.2023.1080p.NF.WEB-DL.H264.DDP5.1-Our.mkv",
        )
        assertEquals("69两头勾 6ixtynin9 The Series", p.title)
        assertEquals(2023, p.year)
    }

    @Test
    fun `英文片去片源与编码标签`() {
        val p = MediaTitleParser.parse("A.Beautiful.Mind.2001.BluRay.REMUX.mkv")
        assertEquals("A Beautiful Mind", p.title)
        assertEquals(2001, p.year)
        assertNull(p.english)
    }

    @Test
    fun `没有年份的中文名原样保留`() {
        val p = MediaTitleParser.parse("读书会.mkv")
        assertEquals("读书会", p.title)
        assertNull(p.year)
    }

    @Test
    fun `Season 与 1x03 形式同样剥离`() {
        assertEquals(
            "Some Show",
            MediaTitleParser.parse("Some.Show.Season.2.1080p.mkv").title,
        )
        assertEquals(
            "Another Show",
            MediaTitleParser.parse("Another.Show.1x03.720p.mkv").title,
        )
    }

    @Test
    fun `清洗后为空时退回原文件名，不产出空标题`() {
        val p = MediaTitleParser.parse("1080p.WEB-DL.mkv")
        assertEquals("1080p.WEB-DL", p.title)
    }
}
