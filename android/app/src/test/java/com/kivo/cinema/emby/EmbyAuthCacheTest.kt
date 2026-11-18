package com.kivo.cinema.emby

import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Test

/**
 * D370：会话缓存的失效语义。
 *
 * 这里锁的是两件**出错时只表现为"一直转圈/一直失败"**的事：
 * 1. 按 token 清除（401 兜底）—— 服务器吊销 token 后不能继续拿废 token 硬撑到 TTL；
 * 2. 凭据变更后不复用旧会话 —— 用户改了密码，旧 token 不该再被拿来用。
 *
 * 用 `127.0.0.1` 的假地址构造 [EmbyClient]（构造不发网络请求）。
 */
class EmbyAuthCacheTest {

    private val address = "http://127.0.0.1:8096"
    private fun client() = EmbyClient(address)

    @Test
    fun `按 token 清除后同一凭据查不到会话`() {
        val c = client()
        val auth = EmbyClient.Auth("tok-abc", "user-1")
        EmbyAuthCache.remember(address, "alice", "pw", c, auth)
        assertNotNull("记住后应命中", EmbyAuthCache.get(address, "alice", "pw"))

        EmbyAuthCache.invalidateByToken("tok-abc")

        assertNull("401 清除后不应再命中", EmbyAuthCache.get(address, "alice", "pw"))
    }

    @Test
    fun `清除不相关 token 不影响其它会话`() {
        val c = client()
        EmbyAuthCache.remember(address, "bob", "pw", c, EmbyClient.Auth("tok-other", "user-2"))
        EmbyAuthCache.invalidateByToken("tok-unrelated")
        assertNotNull("不同 token 不该被误删", EmbyAuthCache.get(address, "bob", "pw"))
    }

    @Test
    fun `凭据变化后旧会话不再命中`() {
        val c = client()
        EmbyAuthCache.remember(address, "carol", "old-pw", c, EmbyClient.Auth("tok-carol", "user-3"))
        assertNull("密码改了应重新登录", EmbyAuthCache.get(address, "carol", "new-pw"))
    }

    @Test
    fun `空 token 调用清除是安全的`() {
        val c = client()
        EmbyAuthCache.remember(address, "dave", "pw", c, EmbyClient.Auth("tok-dave", "user-4"))
        EmbyAuthCache.invalidateByToken("")
        assertNotNull("空 token 不该清掉任何会话", EmbyAuthCache.get(address, "dave", "pw"))
    }
}
