package com.kivo.cinema.smb

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle

/**
 * 分层读尺寸扫描（251 前置诊断工具）。
 *
 * 起因：251 前置实验测出 libsmb2 单次 READ 的延迟与字节数几乎无关
 * （1 MiB 75.9 ms、4 MiB 74.2 ms、8 MiB 84.5 ms），按此推论"放大 chunk 就能提吞吐"。
 * 于是把 `SmbReadTuning.aheadChunk` 临时改成 4 MiB 实测 —— 结果通过 `SmbByteSource`
 * 的 64 KiB 读只有 3.9~5.2 MB/s，反而低于关预读的 5.2 MB/s。**该改动已回滚。**
 *
 * 这个测试把两层分开量，用来定位瓶颈：
 *   A. `SmbReader.pread` 直接读（绕过窗口、绕过预读）—— 裸 SMB + JNI 层
 *   B. `SmbByteSource.readAt` 64 KiB 顺序读（走窗口）—— 生产路径
 *
 * 结论（2026-11-10）：A 的 4 MiB 只有 10.4 MB/s，而同一台 NAS 用纯 C++ 程序
 * 直接调 libsmb2 是 53.9 MB/s。**瓶颈在 JNI 的大数组搬运，不在 SMB。**
 *
 * 参数：`-e smbHost ... -e smbShare ... -e smbPath ...`
 */
class SmbReadAheadInstrumentation : Instrumentation() {

    private var args: Bundle? = null
    private val report = mutableListOf<String>()

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        args = arguments
        start()
    }

    override fun onStart() {
        val host = args?.getString("smbHost") ?: "192.168.5.1"
        val share = args?.getString("smbShare") ?: "XiaoMi"
        val path = args?.getString("smbPath")
            ?: "视频/电影/The.Furious.2026.2160p.iTunes.WEB-DL.DDP.5.1.Atmos.HDR10+.H.265-CHDWEB.mkv"
        val credential = SmbCredential(
            username = args?.getString("smbUser") ?: "",
            password = args?.getString("smbPass") ?: "",
        )
        val address = SmbAddress(host, SmbAddress.DEFAULT_PORT, share)

        var reader: SmbReader? = null
        var source: SmbByteSource? = null
        try {
            // ---- A：裸 SMB 层，不同单次读大小 ----
            val opened = SmbReader(address, credential, path)
            reader = opened
            opened.open()
            val size = opened.size
            log("OPEN", "size=$size")
            val base = size / 4

            for (chunk in CHUNKS) {
                val rounds = BUDGET_BYTES / chunk
                val buf = ByteArray(chunk)
                var offset = base
                var ok = 0
                val began = System.nanoTime()
                for (i in 0 until rounds) {
                    val n = opened.pread(offset, buf, 0, chunk)
                    if (n <= 0) break
                    offset += n
                    ok += n
                }
                val seconds = (System.nanoTime() - began) / 1_000_000_000.0
                val mbps = if (seconds > 0) ok / 1024.0 / 1024.0 / seconds else 0.0
                log("RAW", "chunk=${chunk / 1024}K read=${ok / 1024 / 1024}MiB " +
                    "in ${"%.2f".format(seconds)}s -> ${"%.1f".format(mbps)} MB/s " +
                    "(${"%.1f".format(seconds * 1000 / (ok / chunk + 1))} ms/req)")
            }
            opened.close()
            reader = null

            // ---- B：生产路径（窗口），关预读 ----
            val viaSource = SmbByteSource.open(
                address, credential, path,
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(targetContext) },
            )
            source = viaSource
            viaSource.setPrefetchEnabled(false)
            val viaWindow = sequential(viaSource, size / 2)
            log("WINDOW", "64KiB sequential -> ${"%.1f".format(viaWindow)} MB/s")

            // ---- C：生产路径 + 预读 ----
            viaSource.setPrefetchEnabled(true)
            val withAhead = sequential(viaSource, size / 2 + size / 8)
            log("AHEAD", "64KiB sequential -> ${"%.1f".format(withAhead)} MB/s")
        } catch (e: Throwable) {
            log("FAIL", "${e.javaClass.simpleName}: ${e.message}")
        } finally {
            runCatching { reader?.close() }
            runCatching { source?.close() }
        }
        finish(Activity.RESULT_OK, Bundle().apply {
            putString("stream", report.joinToString("\n", postfix = "\n"))
        })
    }

    /** 以 fd 回调同粒度（64 KiB）顺序读，返回 MB/s。 */
    private fun sequential(source: SmbByteSource, startOffset: Long): Double {
        val chunk = 64 * 1024
        val buf = ByteArray(chunk)
        var offset = startOffset
        var got = 0L
        val began = System.nanoTime()
        while (got < BUDGET_BYTES && offset < source.size()) {
            val read = source.readAt(offset, buf, 0, chunk)
            if (read <= 0) break
            offset += read
            got += read
        }
        val seconds = (System.nanoTime() - began) / 1_000_000_000.0
        log("TRACE", "read=${got / 1024 / 1024} MiB in ${"%.2f".format(seconds)}s")
        return if (seconds > 0) got / 1024.0 / 1024.0 / seconds else 0.0
    }

    private fun log(tag: String, message: String) {
        report += "$tag $message"
        android.util.Log.i("KivoAhead", "[$tag] $message")
    }

    private companion object {
        const val BUDGET_BYTES = 24 shl 20
        val CHUNKS = intArrayOf(64 shl 10, 256 shl 10, 512 shl 10, 1 shl 20, 2 shl 20, 4 shl 20)
    }
}
