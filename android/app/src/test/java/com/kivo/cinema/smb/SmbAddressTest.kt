package com.kivo.cinema.smb

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 地址解析与 authority 形态（250 S1）。
 *
 * 这里的每一项都对应一次真实的"错一次就整条链路不通"：
 * IPv6 的方括号是 libsmb2 `socket.c` 的硬要求（它只在首字符是 `[` 时按 IPv6 解析），
 * UNC 的两个反斜杠是根不是分隔符，多剥一个就少了主机。
 *
 * 之前这些只有 instrumented 覆盖，要插设备跑 40 秒，所以从来没人为了"IPv6 拼串对不对"
 * 去跑一次。现在它们是毫秒级的。
 */
class SmbAddressTest {

    @Test
    fun `smb scheme with share and nested path`() {
        val address = SmbAddress.parse("smb://192.168.5.1/视频/电影/2024")
        assertEquals("192.168.5.1", address.host)
        assertEquals(445, address.port)
        assertEquals("视频", address.share)
        assertEquals("电影/2024", address.path)
        assertFalse(address.needsShare)
    }

    @Test
    fun `unc form keeps host share and path`() {
        val address = SmbAddress.parse("""\\nas\media\movies""")
        assertEquals("nas", address.host)
        assertEquals("media", address.share)
        assertEquals("movies", address.path)
    }

    @Test
    fun `custom port is parsed`() {
        val address = SmbAddress.parse("nas.local:1445/share")
        assertEquals("nas.local", address.host)
        assertEquals(1445, address.port)
        assertEquals("share", address.share)
    }

    @Test
    fun `missing share is flagged`() {
        val address = SmbAddress.parse("smb://192.168.5.1")
        assertTrue(address.needsShare)
        assertTrue(address.share.isEmpty())
    }

    @Test
    fun `bracketed ipv6 with port`() {
        val address = SmbAddress.parse("[::1]:1445/share")
        assertEquals("::1", address.host)
        assertEquals(1445, address.port)
        assertEquals("share", address.share)
        assertEquals("[::1]:1445", address.authority)
    }

    @Test
    fun `ipv4 authority carries the port without brackets`() {
        assertEquals("192.168.5.1:445", SmbAddress("192.168.5.1").authority)
    }

    @Test
    fun `ipv6 authority is always bracketed`() {
        assertEquals("[::1]:445", SmbAddress("::1").authority)
        assertEquals("[fe80::1]:445", SmbAddress("fe80::1").authority)
    }

    @Test
    fun `link local zone id survives parsing`() {
        val address = SmbAddress("fe80::1%wlan0")
        assertEquals("fe80::1%wlan0", address.host)
        assertEquals("[fe80::1%wlan0]:445", address.authority)
    }

    @Test
    fun `uri omits the default port while authority always keeps it`() {
        val address = SmbAddress(host = "nas", share = "media")
        assertEquals("smb://nas/media", address.toUri())
        assertEquals("nas:445", address.authority)
    }

    @Test
    fun `trailing slashes do not leak into the path`() {
        val address = SmbAddress.parse("smb://nas/media/movies/")
        assertEquals("media", address.share)
        assertEquals("movies", address.path)
        assertEquals("media/movies/clip.mkv", address.filePath("clip.mkv"))
    }

    @Test(expected = IllegalArgumentException::class)
    fun `blank input is rejected`() {
        SmbAddress.parse("   ")
    }

    @Test(expected = IllegalArgumentException::class)
    fun `out of range port is rejected`() {
        SmbAddress.parse("nas:70000/share")
    }
}
