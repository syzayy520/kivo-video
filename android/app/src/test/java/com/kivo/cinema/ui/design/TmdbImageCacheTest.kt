package com.kivo.cinema.ui.design

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.After
import org.junit.Test
import java.io.File
import java.lang.reflect.Method
import java.nio.file.Files

/**
 * TmdbImageCache 三条语义锁（设计 §5.3 / §6），全部在纯 JVM 上跑：
 * 1. 命中：put 后 get 取回同一实例，clearMemory 后不再命中；
 * 2. 失败不落盘：失败请求返回 null 且磁盘目录不产生文件；
 * 3. 唯一 URL：磁盘 key 为 URL 的 SHA-256，仅由 URL 决定（token 不拼进 key）。
 *
 * 内部成员用反射访问——不引入 Robolectric/Mockito 依赖，也不为测试改生产可见性。
 */
class TmdbImageCacheTest {

    private val tempDir: File = Files.createTempDirectory("tmdb-img-cache-test").toFile()

    @After
    fun tearDown() {
        clearMemoryReal()
        TmdbImageCache.diskDir = null
        tempDir.deleteRecursively()
    }

    @Test
    fun `命中_put后get取回同一实例_clear后不再命中`() {
        // get() 返回类型 Bitmap?，内部有 checkcast ⇒ 用 Unsafe 分配真 Bitmap 实例，
        // 全程只比引用、不调用任何 Android 方法（JVM 单测无 Robolectric）。
        val bmp = allocateBitmap()
        clearMemoryReal()
        TmdbImageCache.put("https://img/hit", bmp)
        assertSame("命中应取回同一实例", bmp, TmdbImageCache.get("https://img/hit"))
        clearMemoryReal()
        assertNull("clearMemory 后不应命中", TmdbImageCache.get("https://img/hit"))
    }

    @Test
    fun `失败不落盘_失败请求返回null且磁盘无文件`() {
        TmdbImageCache.diskDir = tempDir
        clearMemoryReal()
        // 指向本机未监听端口：连接必失败，不依赖外网。load 内部对 IO 失败重试一次。
        val url = "http://127.0.0.1:1/__kivo_never__/fail.png"
        assertNull("失败请求应返回 null", TmdbImageCache.load(url))
        val target = invokeDiskFile(url)
        assertEquals("失败不应在目标路径留下文件", false, target?.exists())
        val imgDir = File(tempDir, "tmdb-images-v2")
        val written = imgDir.listFiles()?.filter { it.isFile } ?: emptyList()
        assertEquals("失败不应写入任何图片文件", 0, written.size)
    }

    @Test
    fun `唯一URL_key为URL的SHA256且同一URL稳定`() {
        TmdbImageCache.diskDir = tempDir
        val urlA = "https://img/poster/abc?itemId=1&tag=v2"
        val urlB = "https://img/poster/abc?itemId=1&tag=v3"
        val keyA = invokeDiskFile(urlA)?.name
        val keyB = invokeDiskFile(urlB)?.name
        assertEquals("同一 URL key 必须稳定", keyA, invokeDiskFile(urlA)?.name)
        assertNotEquals("不同 URL 必须不同 key（唯一标识靠 URL）", keyA, keyB)
        assertEquals("SHA-256 hex 长度 64", 64, keyA?.length)
        assertEquals("仅 0-9a-f", true, keyA?.all { it in '0'..'9' || it in 'a'..'f' })
    }

    // ── 反射工具：不改生产可见性 ──
    private fun invokeDiskFile(url: String): File? =
        method("diskFile", String::class.java).invoke(TmdbImageCache, url) as File?

    private fun clearMemoryReal() = TmdbImageCache.clearMemory()

    private fun method(name: String, vararg params: Class<*>): Method =
        TmdbImageCache::class.java.getDeclaredMethod(name, *params).apply { isAccessible = true }

    /** Unsafe 分配一个未初始化 Bitmap：只作缓存键值的引用载体，不触碰任何 Android 方法。 */
    private fun allocateBitmap(): android.graphics.Bitmap {
        val unsafeClass = Class.forName("sun.misc.Unsafe")
        val field = unsafeClass.getDeclaredField("theUnsafe").apply { isAccessible = true }
        val unsafe = field.get(null)
        return unsafeClass.getMethod("allocateInstance", Class::class.java)
            .invoke(unsafe, android.graphics.Bitmap::class.java) as android.graphics.Bitmap
    }
}
