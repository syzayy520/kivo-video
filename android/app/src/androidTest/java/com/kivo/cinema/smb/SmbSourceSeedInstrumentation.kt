package com.kivo.cinema.smb

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.storage.SourceStore

/**
 * 往资源库里补一条 SMB 源（252 真机验收的前置）。
 *
 * 为什么需要它：`AppEndToEndSmbInstrumentation` 走的是**用户真实路径**——先按
 * `kivo.sourceId` 取资源库记录，再拉起播放页。而重装/清数据之后资源库是空的
 * （实测 `0 条记录`），于是那条端到端验收根本起不来，只能退回离屏管线。
 * 离屏管线证明不了"播放页里到底卡不卡"，所以这个前置是必需的。
 *
 * 只写一条匿名只读记录（无账号密码），不改动已有记录；同 id 的旧记录会被替换，
 * 因此可以反复跑。**不删任何用户的源**。
 *
 * 参数：`-e smbHost 192.168.5.1 -e name "XiaoMi"`
 */
class SmbSourceSeedInstrumentation : Instrumentation() {

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        val host = arguments?.getString("smbHost") ?: "192.168.5.1"
        val name = arguments?.getString("name") ?: "XiaoMi"

        var detail: String
        var ok = true
        try {
            val store = SourceStore(targetContext)
            val record = SourceRecord(
                id = SEED_ID,
                revision = 1L,
                name = name,
                address = host,
                username = "",
                password = "",
                protocol = SourceRecord.PROTOCOL_SMB,
                hidden = false,
                defaultHome = false,
                aggregateEnabled = false,
                aggregateSearchEnabled = false,
            )
            val result = store.update { current ->
                current.filterNot { it.id == SEED_ID } + record
            }
            val records = result.getOrThrow()
            detail = "seeded id=$SEED_ID host=$host; 现有 " + records.size + " 条: " +
                records.joinToString { "${it.id}(${it.protocol}@${it.address})" }
        } catch (e: Throwable) {
            ok = false
            detail = "FAIL ${e.javaClass.simpleName}: ${e.message}"
        }
        finish(if (ok) Activity.RESULT_OK else Activity.RESULT_CANCELED, Bundle().apply {
            putString("stream", detail + "\n")
        })
    }

    private companion object {
        /** 固定 id：重复跑时替换而不是堆积。 */
        const val SEED_ID = "seed-smb-xiaomi"
    }
}
