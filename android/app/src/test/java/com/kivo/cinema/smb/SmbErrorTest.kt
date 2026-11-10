package com.kivo.cinema.smb

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 读失败归因（250 S1）。
 *
 * 核心是**别把两种负数当成一种**：libsmb2 的 `smb2_pread` 失败时返回
 * `cb_data->status`（NT status，高位 0xC0，作为 int32 是负数）或 `-errno`。
 * 上一版把两者都压成 `-1`，再拿 `smb2_get_nterror()` 解释 —— 而超时路径
 * **没有回复报文**，那个值是上一次回复留下的，于是"这次超时"被译成"上次的权限错误"。
 */
class SmbErrorTest {

    /** STATUS_IO_TIMEOUT = 0xC0000121，超出 Int 正数范围，作为 int32 是负数。 */
    private val ioTimeout = 0xC0000121L.toInt()
    private val accessDenied = 0xC0000022L.toInt()

    @Test
    fun `non negative codes are successes`() {
        assertNull(SmbError.fromReadCode(0))
        assertNull(SmbError.fromReadCode(4096))
    }

    @Test
    fun `timeout is reported as a timeout not as the previous error`() {
        val failure = SmbError.fromReadCode(ioTimeout)
        assertNotNull(failure)
        assertEquals(SmbStage.READ, failure!!.stage)
        assertEquals("服务器没有响应（超时），请检查网络或服务器状态", failure.reason)
        assertTrue("detail must carry the raw status for evidence", failure.detail.contains("c0000121"))
    }

    @Test
    fun `access denied at the read stage is explained by permissions`() {
        val failure = SmbError.fromReadCode(accessDenied)!!
        assertEquals(SmbStage.READ, failure.stage)
        assertEquals("当前账号没有访问权限", failure.reason)
    }

    @Test
    fun `anonymous access denied asks for credentials`() {
        val failure = SmbError.fromReadCode(accessDenied, anonymous = true)!!
        assertEquals("需要账号密码才能访问", failure.reason)
    }

    @Test
    fun `errno codes are explained as errno and keep the number`() {
        val io = SmbError.fromReadCode(-5)!!
        assertEquals("读取中断，请检查网络后重试", io.reason)
        assertTrue(io.detail.contains("errno=5"))

        assertEquals("连接被服务器断开", SmbError.fromReadCode(-104)!!.reason)
        assertEquals("连接超时，网络可能不稳定", SmbError.fromReadCode(-110)!!.reason)
        assertEquals("网络不可达，请检查 Wi-Fi 或网线", SmbError.fromReadCode(-101)!!.reason)
        assertEquals("文件不存在，可能已被移动或删除", SmbError.fromReadCode(-2)!!.reason)
    }

    @Test
    fun `unknown errno keeps the number for forensics`() {
        val failure = SmbError.fromReadCode(-9999)!!
        assertTrue(failure.reason.contains("9999"))
        assertTrue(failure.detail.contains("errno=9999"))
    }

    @Test
    fun `unknown nt status falls back to the read stage text`() {
        val failure = SmbError.fromReadCode(0xC0009999L.toInt())!!
        assertEquals(SmbStage.READ, failure.stage)
        assertTrue(failure.reason.isNotBlank())
    }
}
