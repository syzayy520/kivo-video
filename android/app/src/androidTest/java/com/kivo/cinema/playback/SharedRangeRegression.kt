package com.kivo.cinema.playback

import android.system.Os
import com.kivo.cinema.net.HttpRangeFile
import java.util.concurrent.atomic.AtomicInteger

/** A container's head/index bytes must be reusable across its independent extractors. */
internal object SharedRangeRegression {
    fun exhaustedIndex(source: HttpRangeFile, requests: AtomicInteger, length: Int) {
        source.prepare()
        source.openDescriptor().use { fd ->
            for (offset in listOf(0, length - 65536, 3 * 1024 * 1024, 65536)) {
                val bytes = ByteArray(65536)
                check(Os.pread(fd.fileDescriptor, bytes, 0, bytes.size, offset.toLong()) == bytes.size)
                check(bytes.contentEquals(ByteArray(bytes.size) { ((offset + it) % 251).toByte() }))
            }
            check(requests.get() == 4) {
                "exhausted index evicted live media response: ${requests.get()} requests, expected 4"
            }
        }
    }

    fun continuous(source: HttpRangeFile, requests: AtomicInteger, length: Int) {
        source.prepare()
        val block = 65536
        fun read(fd: android.os.ParcelFileDescriptor, offset: Int) {
            val bytes = ByteArray(block)
            check(Os.pread(fd.fileDescriptor, bytes, 0, block, offset.toLong()) == block)
            check(bytes.contentEquals(ByteArray(block) { ((offset + it) % 251).toByte() }))
        }
        source.openDescriptor().use { first ->
            source.openDescriptor().use { second ->
                read(first, 0)
                // The other extractor gets several MiB ahead. Reusing these
                // bytes must not strand the first descriptor's old HTTP cursor.
                for (offset in block until 4 * 1024 * 1024 step block) read(second, offset)
                read(first, 4 * 1024 * 1024)
                read(first, length - block) // distant index must preserve media response
                read(second, 4 * 1024 * 1024 + block)
                check(requests.get() == 3) {
                    "shared continuous response reopened HTTP: ${requests.get()} requests, expected 3"
                }
            }
        }
    }

    fun run(source: HttpRangeFile, requests: AtomicInteger, length: Int) {
        source.prepare()
        source.openDescriptor().use { first ->
            source.openDescriptor().use { second ->
                for ((fd, offset) in listOf(first to 0, first to length - 65536,
                    second to 0, second to length - 65536, first to 0)) {
                    val bytes = ByteArray(65536)
                    check(Os.pread(fd.fileDescriptor, bytes, 0, bytes.size, offset.toLong()) == bytes.size)
                    check(bytes.contentEquals(ByteArray(bytes.size) { ((offset + it) % 251).toByte() }))
                }
                check(requests.get() == 3) {
                    "shared head/index reads reopened HTTP: ${requests.get()} requests, expected 3"
                }
            }
        }
    }
}
