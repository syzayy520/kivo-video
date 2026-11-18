package com.kivo.cinema.smb.scan

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * NAS 扫描准入规则测试。
 *
 * 判错会让片库变脏（把字幕/系统目录扫进来）或漏片（把真视频当噪音丢掉），
 * 且表现只是"首页少了/多了东西"，线上很难发现，所以必须锁住。
 */
class VideoFileFilterTest {

    @Test
    fun `常见视频容器都算影片`() {
        listOf("a.mkv", "b.MP4", "c.ts", "d.m2ts", "e.avi", "f.rmvb").forEach {
            assertTrue("应识别为视频: $it", VideoFileFilter.isVideo(it))
        }
    }

    @Test
    fun `图片与字幕不算影片`() {
        assertFalse(VideoFileFilter.isVideo("poster.jpg"))
        assertFalse(VideoFileFilter.isVideo("movie.nfo"))
        assertFalse(VideoFileFilter.isVideo("chs.srt"))
    }

    @Test
    fun `图片识别覆盖常见扩展名`() {
        assertTrue(VideoFileFilter.isImage("poster.jpg"))
        assertTrue(VideoFileFilter.isImage("COVER.PNG"))
        assertTrue(VideoFileFilter.isImage("x.webp"))
        assertFalse(VideoFileFilter.isImage("x.mkv"))
    }

    @Test
    fun `系统与非影视目录整棵跳过`() {
        listOf("#recycle", "字幕", "学习", "ipa备份", "BT源", "临时", "@eaDir").forEach {
            assertTrue("应跳过目录: $it", VideoFileFilter.skipDir(it))
        }
        assertFalse(VideoFileFilter.skipDir("电影"))
        assertFalse(VideoFileFilter.skipDir("剧集"))
    }

    @Test
    fun `样片与预告算噪音`() {
        assertTrue(VideoFileFilter.isNoise("movie-sample.mkv"))
        assertTrue(VideoFileFilter.isNoise("trailer.mp4"))
        assertTrue(VideoFileFilter.isNoise(".hidden.mkv"))
        assertFalse(VideoFileFilter.isNoise("正片.mkv"))
    }

    @Test
    fun `封面优先挑海报类文件名`() {
        // poster 优先级压过更大的无关图
        assertEquals(
            "poster.jpg",
            VideoFileFilter.pickPoster(listOf("a.jpg" to 900L, "poster.jpg" to 100L)),
        )
        assertEquals(
            "cover.jpg",
            VideoFileFilter.pickPoster(listOf("cover.jpg" to 100L, "fanart.jpg" to 900L)),
        )
    }

    @Test
    fun `没有海报类文件名时取最大一张`() {
        assertEquals(
            "big.jpg",
            VideoFileFilter.pickPoster(listOf("small.jpg" to 10L, "big.jpg" to 500L)),
        )
        assertNull(VideoFileFilter.pickPoster(emptyList()))
    }
}
