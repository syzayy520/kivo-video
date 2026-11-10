package com.kivo.cinema.smb

/**
 * 共享枚举（243 D1）：一次性的 IPC$ 连接，用完即弃。
 *
 * 协议要求：`NetrShareEnum` 只能在连到 `IPC$` 时调用，所以不能复用 [SmbSession]
 * 的连接（那个连的是具体共享）。这里自己建 context、自己销毁，
 * 失败不影响用户已保存的源。
 *
 * 只返回可浏览的磁盘共享（过滤 IPC$/打印/设备/以 `$` 结尾的隐藏共享）；
 * 若服务器一个都没返回，调用方要给出"手动输入共享名"的兜底。
 */
class SmbShareLister(
    private val host: String,
    private val port: Int = SmbAddress.DEFAULT_PORT,
    private val credential: SmbCredential = SmbCredential.ANONYMOUS,
) {

    fun list(): List<SmbShare> {
        val ctx = Smb2Native.nativeCreate()
        if (ctx == 0L) {
            throw SmbError.stageFailure(SmbStage.CONNECT, "无法初始化 SMB 会话")
        }
        try {
            val rc = Smb2Native.nativeConnect(
                ctx, host, port, IPC_SHARE,
                credential.username, credential.password, credential.domain,
            )
            if (rc != 0) {
                throw SmbError.translate(
                    SmbStage.CONNECT, Smb2Native.nativeNtStatus(ctx),
                    Smb2Native.nativeGetError(ctx), credential.anonymous,
                )
            }
            val json = Smb2Native.nativeListShares(ctx) ?: throw SmbError.translate(
                SmbStage.SHARE, Smb2Native.nativeNtStatus(ctx),
                Smb2Native.nativeGetError(ctx), credential.anonymous,
            )
            val array = org.json.JSONArray(json)
            val shares = ArrayList<SmbShare>(array.length())
            for (i in 0 until array.length()) {
                val item = array.getJSONObject(i)
                shares.add(
                    SmbShare(
                        name = item.getString("name"),
                        remark = item.optString("remark", ""),
                        type = item.optInt("type", 0),
                    )
                )
            }
            return shares.filter { it.isBrowsable }
                .sortedWith(compareBy<SmbShare> { it.isHidden }.thenBy { it.name.lowercase() })
        } finally {
            Smb2Native.nativeDestroy(ctx)
        }
    }

    private companion object {
        const val IPC_SHARE = "IPC$"
    }
}
