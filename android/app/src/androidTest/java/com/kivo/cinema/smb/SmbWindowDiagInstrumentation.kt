package com.kivo.cinema.smb

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle

/**
 * 块窗口层诊断（251 第二步）。
 *
 * 要回答的问题只有一个：**裸读 1 MiB 12.4 MB/s，为什么走窗口的 64 KiB 顺序读只剩 5.4？**
 *
 * 24 MiB 顺序读在块 2 MiB、块数 6 的配置下应该只发生 12 次填充。按裸读 2 MiB 单次的耗时
 * （约 167 ms）算，总时间应在 2.0 s 上下；实测 4.43 s。差的 2.4 秒必须落到某处，
 * 而这三种可能的修法完全不同，不能靠猜：
 *
 * 1. **一次填充发了多次 pread**（服务端把 2 MiB 拆成小 READ，或 [SmbPreadChannel.readFully]
 *    的循环在重试）→ 应放大单次读，或让填充对齐服务端上限；
 * 2. **填充次数远多于 12**（命中判定失效，每 64 KiB 都重填）→ 应查 `copyIfPresent` 的对齐；
 * 3. **填充本身不慢，是 `copyIfPresent` / 统计拖的** → 应查锁与数组拷贝。
 *
 * 本测试**绕过** `SmbByteSource`，直接用 [SmbReadWindow] 复刻窗口路径，把每个计数打出来。
 * 不修改任何生产代码 —— 诊断不该改变被测对象。
 *
 * 参数：`-e smbHost ... -e smbShare ... -e smbPath ...`
 */
class SmbWindowDiagInstrumentation : Instrumentation() {

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
        try {
            val opened = SmbReader(address, credential, path)
            reader = opened
            opened.open()
            val size = opened.size
            val tuning = SmbReadTuning.forFile(size)
            log("OPEN", "size=$size block=${tuning.blockBytes / 1024}K " +
                "blocks=${tuning.blockCount} ahead=${tuning.aheadBytes / 1024 / 1024}MiB")

            // 基准：单次 pread 恰好一个块大小，量出"一次填充"的理论耗时。
            raw(opened, size / 2, tuning.blockBytes, label = "RAW-2MiB")

            // 复刻窗口路径：64 KiB 顺序读，全部计数。
            windowed(opened, size, size / 2, tuning)
        } catch (e: Throwable) {
            log("FAIL", "${e.javaClass.simpleName}: ${e.message}")
        } finally {
            runCatching { reader?.close() }
        }
        finish(Activity.RESULT_OK, Bundle().apply {
            putString("stream", report.joinToString("\n", postfix = "\n"))
        })
    }

    /** 裸 pread：[bytes] 一发，重复到读满 [BUDGET]，报单次耗时与是否短读。 */
    private fun raw(reader: SmbReader, base: Long, bytes: Int, label: String) {
        val requests = BUDGET_BYTES / bytes
        val buf = ByteArray(bytes)
        var offset = base
        var got = 0L
        var shortReads = 0
        val began = System.nanoTime()
        for (i in 0 until requests) {
            val n = reader.pread(offset, buf, 0, bytes)
            if (n <= 0) break
            if (n < bytes) ++shortReads
            offset += n
            got += n
        }
        val seconds = (System.nanoTime() - began) / 1_000_000_000.0
        log(
            label,
            "req=${bytes / 1024}K read=${got / 1024 / 1024}MiB in ${"%.2f".format(seconds)}s " +
                "-> ${"%.1f".format(mbps(got, seconds))} MB/s " +
                "${"%.1f".format(seconds * 1000 / requests)} ms/req short=$shortReads",
        )
    }

    /**
     * 复刻 `SmbByteSource.readAt` 的窗口部分：命中就拷，不命中就填一整块。
     * 每个计数都要单独报 —— 摊在一起就又变成"平均值把锯齿抹平"。
     */
    private fun windowed(reader: SmbReader, size: Long, base: Long, tuning: SmbReadTuning) {
        val window = SmbReadWindow(tuning)
        val chunk = 64 * 1024
        val buf = ByteArray(chunk)

        var offset = base
        var got = 0L
        var fills = 0
        var fillsWithMultiPread = 0
        var preadsTotal = 0
        var preadBytes = 0L
        var fillNsTotal = 0L
        var fillNsMax = 0L
        var copyNsTotal = 0L
        var straddles = 0
        var straddleNsTotal = 0L

        var preadsThisFill = 0
        val began = System.nanoTime()
        while (got < BUDGET_BYTES && offset + chunk <= size) {
            val beforeCopy = System.nanoTime()
            val hit = window.copyIfPresent(offset, buf, 0, chunk)
            copyNsTotal += System.nanoTime() - beforeCopy
            if (!hit) {
                val fillStart = System.nanoTime()
                preadsThisFill = 0
                // 与 SmbPreadChannel.readFully 同语义：读满为止，逐次计数。
                window.fill(offset, size) { at, into, off, len ->
                    var done = 0
                    while (done < len) {
                        val n = reader.pread(at + done, into, off + done, len - done)
                        if (n <= 0) break
                        ++preadsThisFill
                        ++preadsTotal
                        preadBytes += n
                        done += n
                    }
                    if (done == 0) -1 else done
                }
                val fillNs = System.nanoTime() - fillStart
                fillNsTotal += fillNs
                if (fillNs > fillNsMax) fillNsMax = fillNs
                ++fills
                if (preadsThisFill > 1) ++fillsWithMultiPread
                // 填充完成后仍不命中 = 本次请求跨块边界，生产里会退化成一次直读。
                // **这一次直读必须真做并计时**：它只有 64 KiB，却要付一整个往返；
                // 上一版漏了它，于是 24 MiB 的总时间比生产少 1.4 s（凭空好看）。
                if (!window.copyIfPresent(offset, buf, 0, chunk)) {
                    val straddleStart = System.nanoTime()
                    reader.pread(offset, buf, 0, chunk)
                    straddleNsTotal += System.nanoTime() - straddleStart
                    ++straddles
                }
            }
            offset += chunk
            got += chunk
        }
        val seconds = (System.nanoTime() - began) / 1_000_000_000.0

        log(
            "WINDOW",
            "64KiB x ${got / 1024}K in ${"%.2f".format(seconds)}s -> " +
                "${"%.1f".format(mbps(got, seconds))} MB/s",
        )
        log(
            "FILL",
            "count=$fills multi_pread=$fillsWithMultiPread preads=$preadsTotal " +
                "bytes=${preadBytes / 1024 / 1024}MiB " +
                "pread/块=${"%.2f".format(preadsTotal.toDouble() / fills.coerceAtLeast(1))} " +
                "avg=${"%.1f".format(fillNsTotal / 1_000_000.0 / fills.coerceAtLeast(1))}ms " +
                "max=${"%.1f".format(fillNsMax / 1_000_000.0)}ms",
        )
        log(
            "COPY",
            "hit_copy_total=${"%.1f".format(copyNsTotal / 1_000_000.0)}ms " +
                "straddle=$straddles straddle_total=${"%.0f".format(straddleNsTotal / 1_000_000.0)}ms " +
                "avg=${"%.1f".format(straddleNsTotal / 1_000_000.0 / straddles.coerceAtLeast(1))}ms",
        )
    }

    private fun mbps(bytes: Long, seconds: Double): Double =
        if (seconds > 0) bytes / 1024.0 / 1024.0 / seconds else 0.0

    private fun log(tag: String, message: String) {
        report += "$tag $message"
        android.util.Log.i("KivoWindow", "[$tag] $message")
    }

    private companion object {
        const val BUDGET_BYTES = 24L shl 20
    }
}
