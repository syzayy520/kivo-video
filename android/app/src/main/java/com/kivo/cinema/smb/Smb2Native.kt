package com.kivo.cinema.smb

/**
 * libsmb2 的 Kotlin 绑定（220 设计稿 D1 定案实验）。
 * 与 [SmbSource]（jcifs）同一职责面：connect/list/open/readAt/size/close，
 * 供 spike 与后续 LocalRangeProxy 的 ByteSource 使用。
 *
 * 错误纪律（G9）：native 返回负值/空时，调 [lastError] 取 libsmb2 的错误文本组装 reason。
 */
object Smb2Native {

    init {
        System.loadLibrary("kivo_player")
    }

    /** 打开的文件句柄 + 大小（smb2_open + fstat 一次完成）。 */
    class Opened(val handle: Long, val size: Long)

    external fun nativeCreate(): Long
    external fun nativeDestroy(ctx: Long)
    external fun nativeConnect(ctx: Long, host: String, port: Int, share: String,
                               user: String, pass: String, domain: String): Int
    /** 返回 JSON 数组 [name,dir,size]；失败返回 null（读 nativeGetError）。 */
    external fun nativeList(ctx: Long, path: String): String?
    external fun nativeOpen(ctx: Long, path: String): Long
    external fun nativeSize(ctx: Long, fh: Long): Long
    external fun nativeReadAt(ctx: Long, fh: Long, offset: Long, len: Int): ByteArray?
    external fun nativeCloseFh(ctx: Long, fh: Long): Int
    external fun nativeGetError(ctx: Long): String

    class Smb2Exception(val stage: String, val target: String, detail: String) :
        Exception("[$stage] $target: $detail")

    /** libsmb2 后端：与 jcifs 版 SmbSource 同职责面。 */
    class Source(private val config: SmbConfig) : AutoCloseable {
        private var ctx: Long = 0
        private var opened: Opened? = null

        fun connect() {
            ctx = nativeCreate()
            if (ctx == 0L) throw Smb2Exception("create", config.host, "smb2_init_context failed")
            val rc = nativeConnect(ctx, config.host, config.port, config.share,
                config.username, config.password, config.domain)
            if (rc != 0) {
                val err = lastError()
                close()
                throw Smb2Exception("connect", "${config.host}:${config.port}/${config.share}", err)
            }
        }

        fun list(subPath: String = config.path): List<SmbSource.Entry> {
            val json = nativeList(ctx, subPath.trim('/'))
                ?: throw Smb2Exception("browse", subPath, lastError())
            val arr = org.json.JSONArray(json)
            val out = ArrayList<SmbSource.Entry>(arr.length())
            for (i in 0 until arr.length()) {
                val o = arr.getJSONObject(i)
                out.add(SmbSource.Entry(o.getString("name"), o.getBoolean("dir"), o.getLong("size")))
            }
            return out
        }

        fun openForRead(fileSubPath: String): NativeByteSource {
            val rel = fileSubPath.trim('/')
            val fh = nativeOpen(ctx, rel)
            if (fh == 0L) throw Smb2Exception("open", rel, lastError())
            val size = nativeSize(ctx, fh)
            if (size < 0) {
                nativeCloseFh(ctx, fh)
                throw Smb2Exception("size", rel, lastError())
            }
            return NativeByteSource(fh, size)
        }

        fun lastError(): String = if (ctx != 0L) nativeGetError(ctx) else "context destroyed"

        inner class NativeByteSource(private val fh: Long, private val sizeBytes: Long) :
            SmbSource.ByteSource {
            override fun size(): Long = sizeBytes
            override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
                val data = nativeReadAt(ctx, fh, offset, len)
                    ?: throw Smb2Exception("read", "${config.share}@$offset+$len", lastError())
                System.arraycopy(data, 0, buf, off, data.size)
                return data.size
            }
            override fun close() {
                nativeCloseFh(ctx, fh)
            }
        }

        override fun close() {
            opened = null
            if (ctx != 0L) {
                nativeDestroy(ctx)
                ctx = 0
            }
        }
    }
}
