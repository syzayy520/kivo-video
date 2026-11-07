package com.kivo.cinema.smb

import jcifs.CIFSContext
import jcifs.CIFSException
import jcifs.config.PropertyConfiguration
import jcifs.context.BaseContext
import jcifs.smb.NtlmPasswordAuthenticator
import jcifs.smb.SmbFile
import jcifs.smb.SmbRandomAccessFile
import java.util.Properties

/**
 * SMB 协议唯一访问点（220 设计稿 D1）：jcifs-ng 封装。
 * API 面逐项映射 SenPlayer SMBClient（实证 S2）：
 *   create→[connect] · invoke(浏览)→[list] · readBlock→[readAt] ·
 *   fileWrite/writeBlock→[writeAt](预留) · cancelCore→[cancel] · destroy→[close]
 *
 * 错误纪律（G9）：所有失败抛 [SmbSourceException]，message 带阶段与目标
 * （connect/browse/read + host/share/path），上层直接展示，不吞不猜。
 *
 * 线程模型：一个 [SmbSource] 绑定一个 CIFSContext/SmbFile 树；
 * readAt 由播放线程串行调（LocalRangeProxy 单请求队列保证）。
 */
class SmbSource(private val config: SmbConfig, private val extraProps: Map<String, String> = emptyMap()) : AutoCloseable {

    /** 随机读源接口（220 设计稿 D2）：LocalRangeProxy 只认这个，221/224 各自实现。 */
    interface ByteSource : AutoCloseable {
        fun size(): Long
        /** 读满 [buf] 的 len 字节，返回实际读数；EOF 返回 -1。 */
        fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int
    }

    class SmbSourceException(stage: String, target: String, cause: Throwable? = null) :
        Exception("[$stage] $target: ${cause?.message ?: "unknown"}", cause)

    private var ctx: CIFSContext? = null
    private var root: SmbFile? = null       // smb://host:port/share/path/
    private var raf: SmbRandomAccessFile? = null

    /** 连接并打开共享（阶段名 connect；G9 reason 带主机与共享）。 */
    fun connect() {
        try {
            val props = Properties().apply {
                // 只说 SMB2/3（设计稿明确不做 SMB1）；超时别拖死 UI
                setProperty("jcifs.smb.client.minVersion", "SMB202")
                setProperty("jcifs.smb.client.maxVersion", "SMB311")
                // 只走 DNS/IP：不做 NetBIOS 名字解析（真机实证 0.0.0.0<00> 查询拖失败）
                setProperty("jcifs.resolveOrder", "DNS")
                // rawNTLM 降级为矩阵变体（SmbSource 构造参数注入）——
                // 基线保持 jcifs 默认（SPNEGO），与 PC 成功路径完全一致
                for ((k, v) in extraProps) setProperty(k, v)
                setProperty("jcifs.smb.client.connTimeout", "8000")
                setProperty("jcifs.smb.client.soTimeout", "12000")
                setProperty("jcifs.smb.client.responseTimeout", "12000")
            }
            val base = BaseContext(PropertyConfiguration(props))
            val auth = if (config.username.isEmpty()) {
                // 空用户名 = GUEST（对齐 PC 成功路径：guest 用户名 + NTLMv2；
                // PC 上 GUEST/NULL 都通，手机上此前测的都拒——重测无 rawNTLM 干扰下的 GUEST）
                NtlmPasswordAuthenticator(NtlmPasswordAuthenticator.AuthenticationType.GUEST)
            } else {
                NtlmPasswordAuthenticator(config.domain, config.username, config.password)
            }
            ctx = base.withCredentials(auth)
            val url = smbUrl() + config.path.trim('/')
            val f = SmbFile(if (url.endsWith("/")) url else "$url/", ctx)
            f.connect()
            root = f
        } catch (e: Exception) {
            throw SmbSourceException(
                "connect", smbUrl() + config.path.trim('/'), e
            )
        }
    }

    /** 浏览目录（阶段名 browse）。目录末尾必须带 "/"（jcifs 语义），由这里负责。 */
    fun list(subPath: String = config.path): List<Entry> {
        val root = this.root ?: throw SmbSourceException("browse", "not connected")
        return try {
            val dir = if (subPath.isEmpty()) root else SmbFile(smbUrl() + subPath.trim('/') + "/", ctx)
            dir.listFiles().map { child ->
                val name = child.name.trimEnd('/')
                Entry(
                    name = name,
                    isDirectory = child.isDirectory,
                    size = if (child.isDirectory) 0 else child.length(),
                )
            }.sortedWith(compareByDescending<Entry> { it.isDirectory }.thenBy { it.name.lowercase() })
        } catch (e: Exception) {
            throw SmbSourceException("browse", subPath.ifEmpty { config.share }, e)
        }
    }

    /** 打开共享根（或 path 指向的文件）做随机读（阶段名 open）。 */
    fun openForRead(fileSubPath: String): SmbByteSource {
        if (raf != null) {
            raf?.close()
            raf = null
        }
        return try {
            val url = smbUrl() + fileSubPath
            val f = SmbFile(url, ctx)
            val access = SmbRandomAccessFile(f, "r")
            val s = SmbByteSource(access)
            raf = access
            s
        } catch (e: Exception) {
            throw SmbSourceException("open", smbUrl() + fileSubPath, e)
        }
    }

    fun cancel() {
        runCatching { root?.close() }
    }

    override fun close() {
        runCatching { raf?.close() }
        runCatching { root?.close() }
        raf = null
        root = null
    }

    /** share 根 URL（不带 config.path —— path 只在 list/openForRead 的参数里拼，
     *  真机实证：两边都拼会出现 视频/电影/电影/ 双重路径）。 */
    private fun smbUrl(): String =
        "smb://${config.host}:${config.port}/${config.share}/"

    data class Entry(val name: String, val isDirectory: Boolean, val size: Long)

    /** SMB 随机读源（SenPlayer readBlock 的对位）。 */
    inner class SmbByteSource(private val access: SmbRandomAccessFile) : ByteSource {
        override fun size(): Long = try {
            access.length()
        } catch (e: Exception) {
            throw SmbSourceException("size", config.share + config.path, e)
        }

        override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int = try {
            access.seek(offset)
            access.read(buf, off, len)
        } catch (e: Exception) {
            throw SmbSourceException("read", "${config.share}@$offset+$len", e)
        }

        override fun close() {
            // raf 生命周期归 SmbSource 管（openForRead 复用同一个实例）
        }
    }
}
