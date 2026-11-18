package com.kivo.cinema.library

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/**
 * SMB 续播键解析测试。
 *
 * 这个键同时用于「续播」「首页点开播放」「封面读图」，解析错会导致：
 * 播放到错误的片、封面读到别的目录、或首页卡片点不开。
 */
class SmbContinueKeyTest {

    @Test
    fun `标准键解析出源 共享 路径`() {
        val p = SmbContinueKey.parse("smb://src-1/12T/电影/某片/某片.mkv")
        assertEquals("src-1", p?.sourceId)
        assertEquals("12T", p?.share)
        assertEquals("电影/某片/某片.mkv", p?.path)
    }

    @Test
    fun `片名取文件名去扩展名`() {
        val p = SmbContinueKey.parse("smb://s/12T/电影/x/A.Beautiful.Mind.2001.mkv")
        assertEquals("A.Beautiful.Mind.2001.mkv", p?.fileName)
        assertEquals("A.Beautiful.Mind.2001", p?.title)
    }

    @Test
    fun `共享为空时仍可解析路径`() {
        val p = SmbContinueKey.parse("smb://s//电影/x.mkv")
        assertEquals("s", p?.sourceId)
        assertEquals("", p?.share)
        assertEquals("电影/x.mkv", p?.path)
    }

    @Test
    fun `非 smb 键与缺路径一律 null`() {
        assertNull(SmbContinueKey.parse("https://host/a.mkv"))
        assertNull(SmbContinueKey.parse("smb://only-source"))
        assertNull(SmbContinueKey.parse(""))
    }
}
