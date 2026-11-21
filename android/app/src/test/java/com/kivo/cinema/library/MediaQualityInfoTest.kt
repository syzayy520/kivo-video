package com.kivo.cinema.library

import org.junit.Assert.assertEquals
import org.junit.Test

class MediaQualityInfoTest {

    @Test
    fun `真实 NAS 文件名 - 1080p 普通片`() {
        val q = MediaQualityInfo.from(
            "[犯罪都市4].The.Roundup.Punishment.2024.Hami.WEB-DL.1080p.H264.AAC-CMCTV.mkv"
        )
        assertEquals("1080P", q.resolution)
        assertEquals("SDR", q.dynamicRange)
        assertEquals(false, q.hasHdr)
    }

    @Test
    fun `4K Dolby Vision REMUX`() {
        val q = MediaQualityInfo.from("A.Beautiful.Mind.2001.2160p.BluRay.REMUX.Dolby.Vision.HDR10.mkv")
        assertEquals("4K", q.resolution)
        assertEquals("DOLBY VISION", q.dynamicRange)
        assertEquals(true, q.hasHdr)
    }

    @Test
    fun `HDR10 720p`() {
        val q = MediaQualityInfo.from("Some.Show.S01E01.720p.HDR10.x265.mkv")
        assertEquals("720P", q.resolution)
        assertEquals("HDR10", q.dynamicRange)
        assertEquals(true, q.hasHdr)
    }

    @Test
    fun `HDR10+ 别名`() {
        val q = MediaQualityInfo.from("Movie.2020.2160p.HDR10plus.WEB.mkv")
        assertEquals("4K", q.resolution)
        assertEquals("HDR10+", q.dynamicRange)
        assertEquals(true, q.hasHdr)
    }

    @Test
    fun `无标签 - 分辨率空 动态范围回落 SDR 不伪造分辨率`() {
        val q = MediaQualityInfo.from("Movie.mkv")
        assertEquals("", q.resolution)
        assertEquals("SDR", q.dynamicRange)
        assertEquals(false, q.hasHdr)
    }

    @Test
    fun `编码名不当分辨率`() {
        val q = MediaQualityInfo.from("Movie.x264.aac.mkv")
        assertEquals("", q.resolution)
    }
}
