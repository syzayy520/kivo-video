package com.kivo.cinema.net

import android.content.Context
import android.util.Log
import java.io.File

/**
 * 网络诊断落盘（273）。
 *
 * ## 为什么不只用 Logcat
 * 真机（Redmi K40 / HyperOS）**过滤应用级 logcat** —— 同一时刻系统日志里能拿到
 * `AudioTrack`/`AudioFlinger` 的行，却拿不到本应用任何一行。真机取证必须能落盘、
 * 能 `run-as` 拉出来，所以诊断走文件。
 *
 * ## 落盘位置
 * `files/net-diag.log`（应用私有目录，`adb shell run-as com.kivo.cinema cat files/net-diag.log`）。
 * 追加写，进程存活期间不轮转 —— 单次排查量级只有几十 KB。
 *
 * ## 铁律
 * **不记 token**：只记状态码、区间、字节数、耗时。token 只出现在请求头里。
 */
object NetDiag {

    private const val TAG = "KivoNetDiag"

    @Volatile
    private var file: File? = null

    /** 由任一 Activity 调一次即可（进程内全局）。 */
    fun attach(context: Context) {
        if (file != null) return
        file = File(context.applicationContext.filesDir, "net-diag.log")
    }

    /** 记一行（带时间戳，毫秒）。 */
    fun log(message: String) {
        val target = file
        if (target == null) {
            Log.i(TAG, message)
            return
        }
        runCatching {
            target.appendText("${System.currentTimeMillis()} $message\n")
        }
    }

    /** 清空（新一轮取证前调用，避免与上一轮混读）。 */
    fun clear(context: Context) {
        val target = file ?: File(context.applicationContext.filesDir, "net-diag.log").also { file = it }
        runCatching { target.writeText("") }
    }
}
