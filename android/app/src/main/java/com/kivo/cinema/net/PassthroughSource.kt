package com.kivo.cinema.net

/**
 * **直通源**：不提供任何字节，只告诉代理"这一路去那个地址取"。
 *
 * ## 为什么需要它
 *
 * 我们的取数层（`ByteSource`）建立在**随机读**契约上：有 `size()`、能 `readAt(offset)`。
 * 这条契约服务的是"源是一个完整文件"的情形 —— 直连流、SMB、本地文件都满足。
 *
 * 但 **Emby 的转码流不满足**（真机实测 2026-11-15 18:2x）：
 *
 * ```
 * [play] origin open fail @4194304: 服务器忽略了范围请求，无法定位到指定位置
 * PROXY size fail: IOException
 * [ffmpeg] http: HTTP error 503 Service Unavailable
 * ```
 *
 * 转码流是**顺序输出、无 Content-Length、不支持 Range** 的管道，
 * 而我们按 Range 去要 4 MiB 起步的分段 ⇒ 直接失败。
 *
 * ## 它怎么工作
 *
 * 代理收到对直通源的请求时，**回 302 让内核自己连源站**（见 `LocalRangeResponder`）。
 * 于是：
 * · 清单与分片都由 mpv/ffmpeg 直接向源站请求，不经过我们的随机读层；
 * · HLS 的**分片天生可随机请求** ⇒ seek 依旧可用（这不是"不可 seek 的流"）；
 * · 我们放弃的只是"分段取数统计/预热"—— 转码场景本来也不需要它们。
 *
 * ## 为什么是 HLS 而不是 mp4 直出
 *
 * `stream.mp4?static=false` 也能拿到 200，但它是**单条不可定位的管道**：
 * 一 seek 就得重新起转码，且拿不到总长。HLS 用 2 秒分片，seek 只是换一个分片请求。
 * 真机验证清单：`720x302 / 1.95 Mbps 峰值 / 1.63 Mbps 平均` ⇒ 0.2 MB/s，
 * 而手机侧实测 0.87–1.75 MB/s ⇒ **带宽足够**（这是 283 里唯一可行的降码率路径）。
 */
class PassthroughSource(
    /** 目标地址（HLS 清单或任何"让内核自己连"的 URL）。 */
    val targetUrl: String,
) : ByteSource {

    /**
     * 把内核发来的**相对请求**解析成源站地址。
     *
     * 【为什么会收到相对路径】内核以为它在读 `http://127.0.0.1:port/1`；
     * HLS 清单里的子地址是相对写法（`main.m3u8?MediaSourceId=…`、`main-1.ts?…`），
     * 于是这些请求原样打回代理：`GET /main.m3u8?…`。代理必须把它们映射回源站
     * ⇒ 规则与 HTTP 相对 URL 语义一致：**清单所在目录 + 请求文件名 + 请求自带 query**。
     *
     *   master  = `https://h/emby/Videos/252137/master.m3u8?A`
     *   收到    = `/main.m3u8?B`   ⇒   `https://h/emby/Videos/252137/main.m3u8?B`
     *
     * query 用**请求自带的那份**（清单里写什么就传什么），不用 master 的 ——
     * 分片清单的参数和 master 的参数本来就不是一份。
     *
     * @param targetPath 代理收到的原始 path+query，如 `/main.m3u8?MediaSourceId=…`
     */
    fun resolve(targetPath: String): String {
        val dir = targetUrl.substringBefore('?').substringBeforeLast('/') + '/'
        val fileWithQuery = targetPath.trim('/')
        return dir + fileWithQuery
    }

    /**
     * 直通源**没有**总长概念 —— 这是它的本质，不是缺陷。
     *
     * 抛异常而不是返回 0/-1：调用方一旦走到这里就说明分流错了（本该 302 却没走），
     * 静默返回一个假值会让内核以为"这是个 0 字节文件"，现象变成更难查的"打开即结束"。
     */
    override fun size(): Long =
        throw UnsupportedOperationException("直通源不提供大小：应由代理回 302，不该走到这里")

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int =
        throw UnsupportedOperationException("直通源不提供随机读：应由代理回 302，不该走到这里")

    override fun close() = Unit
}
