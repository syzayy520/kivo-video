package com.kivo.cinema.playback
import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle

/**
 * 301 · `NetworkRecoveryWatch` 状态机单测（275 §6.1 的 `network_recovery_ms`）。
 *
 * ## 为什么用虚拟时间喂，而不是真断网
 * 真断网要断 60 s 以上（mpv 的 demux 缓存能撑约 20–37 s，断短了测不出停滞），
 * 而且断网会同时影响 adb/供片/其它测试。状态机是**纯逻辑**，用虚拟时间戳
 * 就能把四条分支逐一钉死；真机端到端留给"断网 60 s"那类专项（P253 已做过）。
 *
 * ## 四条分支
 * 1. 正常播放 ⇒ 永不计恢复；
 * 2. 暂停 ⇒ 位置不动但**不是断网**，不许误判；
 * 3. 断网（位置停 + 离线）→ 网络回来 → 时钟再前进 ⇒ 记恢复耗时；
 * 4. 位置停但**网络一直在线**（解码慢/seek）⇒ 不许记恢复。
 */
class NetworkRecoveryWatchInstrumentation : Instrumentation() {

    private val results = mutableListOf<Pair<String, Boolean>>()

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        // ---- 1：正常播放，位置每拍都在涨 ----
        NetworkRecoveryWatch.reset()
        var t = 0L
        var pos = 10.0
        repeat(10) {
            NetworkRecoveryWatch.note(pos, paused = false, online = true, nowMs = t)
            pos += 1.0
            t += 2_000
        }
        results += "1 正常播放不计恢复" to (NetworkRecoveryWatch.lastRecoveryMs() == -1L)

        // ---- 2：暂停时位置不动，但不应被当成断网 ----
        NetworkRecoveryWatch.reset()
        t = 0L
        pos = 30.0
        NetworkRecoveryWatch.note(pos, paused = false, online = true, nowMs = t)
        t += 2_000
        repeat(10) {
            // paused=true：位置不变，且系统可能因为息屏等报离线
            NetworkRecoveryWatch.note(pos, paused = true, online = false, nowMs = t)
            t += 2_000
        }
        results += "2 暂停不误判为断网" to (NetworkRecoveryWatch.lastRecoveryMs() == -1L)

        // ---- 3：断网 → 恢复 → 时钟前进 ----
        NetworkRecoveryWatch.reset()
        t = 0L
        // 正常两拍
        NetworkRecoveryWatch.note(40.0, false, true, 0)
        NetworkRecoveryWatch.note(42.0, false, true, 2_000)
        // 位置停 + 离线：t=4000 起停，t=8000 时 stalledFor=4000 ≥3000 ⇒ 认定离线
        NetworkRecoveryWatch.note(42.0, false, true, 4_000)
        NetworkRecoveryWatch.note(42.0, false, true, 6_000)
        NetworkRecoveryWatch.note(42.0, false, false, 8_000)
        NetworkRecoveryWatch.note(42.0, false, false, 10_000)
        // 网络回来（t=12000）⇒ 记 onlineAt
        NetworkRecoveryWatch.note(42.0, false, true, 12_000)
        // 时钟前进（t=14000）⇒ 恢复耗时 = 14000-12000 = 2000
        NetworkRecoveryWatch.note(42.6, false, true, 14_000)
        val rec3 = NetworkRecoveryWatch.lastRecoveryMs()
        results += "3 断网恢复记为 2000ms（实际 ${rec3}）" to (rec3 == 2_000L)

        // ---- 4：位置停但网络一直在线（解码慢 / seek）⇒ 不许记 ----
        NetworkRecoveryWatch.reset()
        t = 0L
        NetworkRecoveryWatch.note(60.0, false, true, 0)
        NetworkRecoveryWatch.note(61.0, false, true, 2_000)
        repeat(6) {
            NetworkRecoveryWatch.note(61.0, false, true, 4_000L + it * 2_000L)
        }
        // 之后继续播 —— 但期间网络一直在线 ⇒ 从来没进过"离线"分支
        NetworkRecoveryWatch.note(62.0, false, true, 20_000)
        results += "4 位置停但在线不计恢复" to (NetworkRecoveryWatch.lastRecoveryMs() == -1L)

        val pass = results.count { it.second }
        val sb = StringBuilder()
        results.forEach { (name, ok) ->
            sb.append(if (ok) "PASS " else "FAIL ").append(name).append('\n')
        }
        sb.append("$pass/${results.size} PASS\n")
        finish(
            if (pass == results.size) Activity.RESULT_OK else Activity.RESULT_CANCELED,
            Bundle().apply {
                putString("stream", sb.toString())
                putInt("pass", pass)
                putInt("total", results.size)
            },
        )
    }
}
