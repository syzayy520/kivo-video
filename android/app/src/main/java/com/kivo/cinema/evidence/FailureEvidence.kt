package com.kivo.cinema.evidence

import android.util.Log

/**
 * 失败也要留证据（P14-R 从 `PlayerActivity` 搬出来，**输出逐字节不变**）。
 *
 * ## 为什么它必须存在
 *
 * 本机 HyperOS 对 adb 完全不开放 logcat（`adb logcat -d` 返回 0 行），
 * 屏幕上的一行字又会被下一次刷新覆盖。所以失败必须落成文件，
 * 否则等于「什么都没发生」—— 这正是 G9 禁止的静默失败。
 *
 * ## 为什么搬出来
 *
 * 它与 [SessionFinalizer] 是同一件事的两个时刻：一个在失败时写，一个在收尾时写。
 * 原先一个在 `PlayerActivity` 里、一个正在被搬进来，两边各自拼路径 ——
 * 那正是"两处各自拼文件名"的老病（`p12f3` 的四条负向用例就是被这个坑覆盖过）。
 */
object FailureEvidence {

    private const val TAG = "KivoCinema"

    /**
     * 写一份 `kivo.p12f.negative/1`。
     *
     * 字段顺序**不要调整**：`docs/evidence/p12f/` 里那批历史证据是这个顺序，
     * 而 `verify_p12_all.py` 虽然按 key 读，人眼比对时顺序不同会以为是两份东西。
     *
     * @return 写成功与否。调用方**不**因为写失败改变控制流 —— 失败本身已经发生了。
     */
    fun write(
        paths: EvidencePaths,
        caseName: String,
        stage: String,
        returnCode: Int,
        reason: String,
        mediaName: String,
    ): Boolean {
        val escaped = reason.replace("\\", "\\\\").replace("\"", "\\\"")
        val payload = buildString {
            appendLine("{")
            appendLine("  \"schema\": \"kivo.p12f.negative/1\",")
            // P12-F4 层 1：证据必须带实测时间戳，否则分不清真机产物与手写片段。
            appendLine("  \"generated_at_unix_ms\": ${System.currentTimeMillis()},")
            appendLine("  \"case\": \"${caseName.replace("\"", "\\\"")}\",")
            appendLine("  \"stage\": \"$stage\",")
            appendLine("  \"return_code\": $returnCode,")
            appendLine("  \"media\": \"$mediaName\",")
            appendLine("  \"reason\": \"$escaped\"")
            appendLine("}")
        }
        val file = paths.failureFile(caseName)
        val written = runCatching { file.writeText(payload) }.isSuccess
        Log.w(TAG, "failure evidence written=$written ${file.name} stage=$stage rc=$returnCode $reason")
        return written
    }
}
