package com.kivo.cinema.net

/** Bounded debug-only numeric I/O events; no addresses or exception messages. */
internal class NetworkReadTrace {
    private val originNs = System.nanoTime()
    private val first = ArrayList<String>(32)
    private val recent = java.util.ArrayDeque<String>(512)
    private var events = 0L

    @Synchronized fun record(event: String) {
        val line = "t_ms=${(System.nanoTime() - originNs) / 1_000_000} $event"
        events++
        if (first.size < 32) first += line else {
            if (recent.size == 512) recent.removeFirst()
            recent.addLast(line)
        }
    }

    @Synchronized fun snapshot(): List<String> =
        listOf("events=$events retained=${first.size + recent.size}") + first + recent
}
