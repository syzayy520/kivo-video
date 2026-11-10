package com.kivo.cinema.smb

/**
 * SMB 控制连接（243 D4）：负责"连到某个共享"与"列目录"。
 *
 * 一个实例 = 一个 libsmb2 context = 一条 TCP 连接。libsmb2 的 context 不是线程安全的，
 * 所以这里用 [lock] 串行化所有调用；浏览与播放**各用各的实例**（播放见 [SmbReader]），
 * 否则一边翻目录一边播片会互相阻塞。
 *
 * 只做控制面：不开文件、不读字节。地址里 `share` 为空时不能连接——先 [SmbShareLister]。
 */
class SmbSession(
    private val address: SmbAddress,
    private val credential: SmbCredential,
) : AutoCloseable {

    private val lock = Any()
    private var ctx: Long = 0

    val isConnected: Boolean get() = ctx != 0L

    fun connect() = synchronized(lock) {
        if (ctx != 0L) return@synchronized
        if (address.needsShare) {
            throw SmbError.stageFailure(SmbStage.SHARE, "请先选择要访问的共享")
        }
        val created = Smb2Native.nativeCreate()
        if (created == 0L) {
            throw SmbError.stageFailure(SmbStage.CONNECT, "无法初始化 SMB 会话")
        }
        ctx = created
        val rc = Smb2Native.nativeConnect(
            created, address.authority, address.share,
            credential.username, credential.password, credential.domain,
        )
        if (rc != 0) {
            val failure = translate(SmbStage.CONNECT)
            close()
            throw failure
        }
    }

    /** 列出 [subPath]（相对共享根；空 = 共享根）。目录在前，同类按名称排序。 */
    fun list(subPath: String = address.path): List<SmbEntry> = synchronized(lock) {
        check(ctx != 0L) { "SMB 会话尚未连接" }
        val json = Smb2Native.nativeList(ctx, subPath.trim('/'))
            ?: throw translate(SmbStage.BROWSE)
        val array = org.json.JSONArray(json)
        val entries = ArrayList<SmbEntry>(array.length())
        for (i in 0 until array.length()) {
            val item = array.getJSONObject(i)
            entries.add(
                SmbEntry(
                    name = item.getString("name"),
                    isDirectory = item.getBoolean("dir"),
                    size = item.optLong("size", 0L),
                )
            )
        }
        return@synchronized entries.sortedWith(
            compareByDescending<SmbEntry> { it.isDirectory }.thenBy { it.name.lowercase() }
        )
    }

    /** 打开一条独立的播放数据连接（同地址同凭据，但自己的 context）。 */
    fun openReader(filePath: String): SmbReader {
        val reader = SmbReader(address, credential, filePath)
        reader.open()
        return reader
    }

    override fun close() = synchronized(lock) {
        val current = ctx
        ctx = 0
        if (current != 0L) Smb2Native.nativeDestroy(current)
        Unit
    }

    private fun translate(stage: SmbStage): SmbFailure = SmbError.translate(
        stage,
        Smb2Native.nativeNtStatus(ctx),
        Smb2Native.nativeGetError(ctx),
        credential.anonymous,
    )
}
