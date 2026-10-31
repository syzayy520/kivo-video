package com.kivo.cinema.evidence

import android.util.Log
import com.kivo.cinema.bridge.PlaybackSession
import java.io.File

/**
 * 一条会话的收尾：**停 → 写 → 封口**（P14-R 从 `PlayerActivity` 里搬出来）。
 *
 * ## 顺序不可换：先停，再写
 *
 * 上一版的顺序是「写解码证据 → stop() → 写合并证据」，两个写之间夹一个 stop，
 * 理由是**只有**合并证据要读"节拍器停下来之后的完整直方图"。
 * 解码那一份当时不需要停，因为它只在"已经播完"时才被写。
 *
 * 这张卡加了第二个入口（被打断），那个前提就不成立了：
 * 打断时线程还在跑，`frames_received` 已经加了 1 而 `frames_released` 还没加 ——
 * 一份**本该闭合**的证据会看起来没闭合。层 2 早就承认过这个窗口
 * （`check_frames_closed`：「允许 1 帧在途（交出与计数之间有一个观察窗口）」），
 * 但那是给"读统计"留的余地，不该被用来给"写证据"开脱。
 *
 * 所以顺序统一成一条**更强的规则**：**写之前先让所有线程停下。**
 * 它对"为什么写"不作任何假设 —— 自然收尾、被打断、出错，走的都是同一条路。
 *
 * `stop()` 之后句柄仍然可用（`PlaybackSession.stop()` 只停、不释放），
 * 所以 `writeStats` 读到的是一份冻结的终值，不是"最后一次轮询碰巧看到的值"。
 *
 * ## 封口标志为什么分开
 *
 * 失败要重试、成功不许重写。这一点沿用 P12-D4 的原语义：
 * 之前的写法是"每轮询一次写一次"，同一个文件被重写十几遍 —— 数字没变所以没人发现，
 * 但"最后一次写"可能落在会话已经半停的时刻，那才是风险。
 */
class SessionFinalizer(
    private val paths: EvidencePaths,
    /** 主证据文件名（`kivo.evidence`，缺省 `p12b-decode.json`）。 */
    private val statsName: String,
    /** 合并证据文件名（`kivo.sync`，缺省主文件名 + `-sync.json`）。 */
    private val syncName: String,
    /** 这一轮要不要写合并证据。等于「有没有开节拍」：没节拍器就没有偏差样本。 */
    private val writesSync: Boolean,
) {

    private var statsWritten = false
    private var syncWritten = false

    /** 两份都封口了。取证模式下据此决定能不能退出。 */
    val sealed: Boolean
        get() = statsWritten && (!writesSync || syncWritten)

    /** 新一轮起播时解除封口。 */
    fun reset() {
        statsWritten = false
        syncWritten = false
    }

    /**
     * 收尾并落盘。**可重复调用**：已经封口的那一份不会重写。
     *
     * 调用方负责在此之前停掉自己的轮询 / 长稳采样 —— 那些是 UI 侧的心跳，
     * 这里不认识 `Handler`。
     */
    fun finalize(live: PlaybackSession): Outcome {
        // 第一句就是 stop()：见类注释。等下面两个写都完成时，账已经冻住了。
        live.stop()

        val statsFile = paths.statsFile(statsName)
        val statsRc = if (statsWritten) {
            0
        } else {
            live.writeStats(statsFile.absolutePath).also { rc ->
                Log.i(TAG, "stats written rc=$rc -> ${statsFile.absolutePath}")
                if (rc == 0) {
                    statsWritten = true
                }
            }
        }

        // 合并证据只在开了节拍时写：没有节拍器就没有偏差样本，写出来只是一份
        // "全是 0"的文件，比不写更容易被误读成"零偏差"。
        var syncFile: File? = null
        var syncRc: Int? = null
        if (writesSync) {
            // 单独一个非空局部量：`var syncFile: File?` 在 lambda 里不会被智能转型，
            // 写成 `syncFile.absolutePath` 会编译报错 —— 而"给可空量加 !!" 只是把
            // 编译错误换成运行时崩溃。这里用一次显式赋值把它钉死。
            val target = paths.syncFile(syncName)
            syncFile = target
            syncRc = if (syncWritten) {
                0
            } else {
                live.writeSyncStats(target.absolutePath).also { rc ->
                    Log.i(TAG, "sync stats written rc=$rc -> ${target.absolutePath}")
                    if (rc == 0) {
                        syncWritten = true
                    }
                }
            }
        }

        return Outcome(statsFile, statsRc, syncFile, syncRc)
    }

    /**
     * 一次收尾的结果。
     *
     * 只带"写到哪、成没成"，不带"要不要退出 App"—— 那是调用方的策略
     * （取证模式写完就退出，产品模式继续跑）。
     */
    class Outcome(
        val statsFile: File,
        val statsRc: Int,
        val syncFile: File?,
        val syncRc: Int?,
    ) {
        /** 两份都落地了。 */
        val written: Boolean
            get() = statsRc == 0 && (syncRc == null || syncRc == 0)
    }

    private companion object {
        const val TAG = "KivoCinema"
    }
}
