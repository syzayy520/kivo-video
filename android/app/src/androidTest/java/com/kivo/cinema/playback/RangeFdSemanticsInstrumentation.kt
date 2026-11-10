package com.kivo.cinema.playback

import android.app.Instrumentation
import android.os.Bundle
import android.os.SystemClock
import android.system.ErrnoException
import android.system.Os
import android.system.OsConstants
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.net.HttpRangeFile
import com.kivo.cinema.sources.storage.SourceStore

/**
 * 只读诊断：代理文件描述符的 fstat / lseek / pread 语义。
 *
 * 不打开 MediaExtractor、不解码、不改变生产路径。目的只有一件事：
 * 判定 K40 容器解析失败属于哪一类 ——
 *   A 类：代理 fd 对 SEEK_END 或随机偏移不可用，导致平台 demux 认为源不可 seek，
 *          退化为顺序扫描（可修 fd 语义）；
 *   B 类：fd 完全正常，平台 demux 只是不解析 MKV SeekHead（必须换 demux）。
 *
 * 两类修法互斥，先拿到数字再决定，不用推测代替证据。
 */
class RangeFdSemanticsInstrumentation : Instrumentation() {
    private var stage = "login"
    private var args: Bundle? = null

    override fun onCreate(bundle: Bundle?) {
        args = bundle
        super.onCreate(bundle)
        start()
    }

    override fun onStart() {
        val began = SystemClock.elapsedRealtime()
        var success = false
        fun report(message: String) {
            sendStatus(0, Bundle().apply {
                putString("stream", "t=${SystemClock.elapsedRealtime() - began} $message\n")
            })
        }
        try {
            val sourceIndex = args?.getString("source")?.toIntOrNull() ?: 1
            val itemIndex = args?.getString("item")?.toIntOrNull() ?: 2
            val source = SourceStore(targetContext).read().getOrThrow()
                .filterNot { it.hidden }[sourceIndex]
            val client = EmbyClient(source.address)
            val auth = client.login(source.username, source.password)
            val item = client.items(auth, start = itemIndex, limit = 1).items.first()
            report("source=${sourceIndex + 1} item=${itemIndex + 1} login=ok")

            HttpRangeFile(targetContext, client.streamUrl(auth, item.id)) { event ->
                report(event)
            }.use { input ->
                stage = "prepare"
                input.prepare()
                report("declared_size=${input.size}")

                input.openDescriptor().use { pfd ->
                    val fd = pfd.fileDescriptor

                    stage = "fstat"
                    val stat = Os.fstat(fd)
                    report("fstat_size=${stat.st_size} matches=${stat.st_size == input.size}")

                    stage = "lseek_end"
                    val end = runCatching { Os.lseek(fd, 0, OsConstants.SEEK_END) }
                    report("lseek_end=${end.fold({ it.toString() }, { err(it) })}")

                    stage = "lseek_set"
                    val back = runCatching { Os.lseek(fd, 0, OsConstants.SEEK_SET) }
                    report("lseek_set_zero=${back.fold({ it.toString() }, { err(it) })}")

                    stage = "pread_head"
                    val head = ByteArray(16 * 1024)
                    val headCount = Os.pread(fd, head, 0, head.size, 0)
                    report("pread_head bytes=$headCount magic=${head.copyOf(4).joinToString("") { "%02x".format(it) }}")

                    stage = "pread_tail"
                    val tail = ByteArray(8 * 1024)
                    val tailAt = input.size - tail.size
                    val tailCount = Os.pread(fd, tail, 0, tail.size, tailAt)
                    report("pread_tail offset=$tailAt bytes=$tailCount nonzero=${tail.count { it.toInt() != 0 }}")

                    stage = "lseek_tail_read"
                    val seekTail = runCatching { Os.lseek(fd, tailAt, OsConstants.SEEK_SET) }
                    val readCount = runCatching { Os.read(fd, tail, 0, tail.size) }
                    report("lseek_tail=${seekTail.fold({ it.toString() }, { err(it) })} " +
                        "read_bytes=${readCount.fold({ it.toString() }, { err(it) })}")

                    stage = "lseek_end_again"
                    val end2 = runCatching { Os.lseek(fd, 0, OsConstants.SEEK_END) }
                    report("lseek_end_after_read=${end2.fold({ it.toString() }, { err(it) })}")

                    stage = "pread_mid"
                    val mid = ByteArray(4 * 1024)
                    val midAt = input.size / 2
                    val midCount = Os.pread(fd, mid, 0, mid.size, midAt)
                    report("pread_mid offset=$midAt bytes=$midCount nonzero=${mid.count { it.toInt() != 0 }}")

                    success = headCount == head.size && tailCount == tail.size &&
                        midCount == mid.size && stat.st_size == input.size
                }
            }
        } catch (e: Throwable) {
            report("FAIL stage=$stage type=${e.javaClass.simpleName} msg=${e.message?.take(80)}")
        }
        finish(if (success) 0 else 1, Bundle().apply {
            putString("result", if (success) "pass" else "fail@$stage")
        })
    }

    private fun err(e: Throwable): String {
        val errno = (e as? ErrnoException)?.errno
        return "ERR:${e.javaClass.simpleName}${if (errno != null) ":$errno" else ""}"
    }
}
