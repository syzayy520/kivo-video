package com.kivo.cinema.sources

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbCredential
import com.kivo.cinema.smb.SmbSession
import com.kivo.cinema.sources.storage.SourceStore

/**
 * S4 数据层验收：把一条 SMB 源写进加密存储，再读回来、连上去、列目录。
 *
 * 走的是生产代码同一条路径（[SourceStore] + [SourceRepository] + [SmbSession]），
 * 不碰 UI —— UI 输入在真机上已经用截图确认过，特殊字符由 adb 的 `input text`
 * 吃掉（`smb://` 会变成 `smb`），不能作为自动化通道。
 *
 * 参数：`-e smbHost` / `-e smbShare` / `-e smbUser` / `-e smbPass`
 */
class SmbSourceSaveInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var passed = 0
    private var total = 0
    private var args: Bundle? = null

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        args = arguments
        start()
    }

    override fun onStart() {
        val host = args?.getString("smbHost") ?: "192.168.5.1"
        val share = args?.getString("smbShare") ?: "XiaoMi"
        val user = args?.getString("smbUser") ?: ""
        val pass = args?.getString("smbPass") ?: ""
        val store = SourceStore(targetContext)
        try {
            val before = store.read().getOrThrow()
            log("BEFORE", "已有 ${before.size} 条源")

            case("写入 SMB 源（schema 2）") {
                val repository = SourceRepository(store)
                val address = SmbAddress(host, SmbAddress.DEFAULT_PORT, share).toUri()
                // 幂等：同地址同账号的旧记录先删掉，否则去重会拒绝（这正是 D2b 的行为）。
                before.filter { it.protocol == SourceRecord.PROTOCOL_SMB && it.address == address &&
                    it.username == user }.forEach { repository.remove(it.id, it.revision).getOrThrow() }
                val draft = SourceRecord(
                    name = "小米路由",
                    address = address,
                    username = user,
                    password = pass,
                    protocol = SourceRecord.PROTOCOL_SMB,
                )
                val saved = repository.saveVerified(draft).getOrThrow()
                val record = saved.first { it.id == draft.id }
                check(record.protocol == SourceRecord.PROTOCOL_SMB) { record.protocol }
                check(record.address == address) { record.address }
                log("SAVED", "${record.name} ${record.protocol} ${record.address} " +
                    "user=${record.username.ifEmpty { "<匿名>" }} id=${record.id}")
            }

            case("读回：protocol / address / 密文不含明文") {
                val after = store.read().getOrThrow()
                val record = after.firstOrNull { it.protocol == SourceRecord.PROTOCOL_SMB }
                check(record != null) { "读不到刚写入的 SMB 源" }
                check(record!!.address.endsWith("/$share")) { record.address }
                // 磁盘上是密文：原始字节里不该出现源名/地址这类明文字段。
                // （密码为空时不能拿空串做包含判断 —— 密文必然含 0 字节，会误报。）
                val raw = java.io.File(targetContext.noBackupFilesDir, "sources/accounts.bin").readBytes()
                check(!String(raw, Charsets.UTF_8).contains(record.name)) { "存储里出现了明文源名" }
                check(!String(raw, Charsets.UTF_8).contains("smb://")) { "存储里出现了明文地址" }
                if (pass.isNotEmpty()) {
                    check(!String(raw, Charsets.ISO_8859_1).contains(pass)) { "存储里出现了明文密码" }
                }
                log("RELOAD", "${record.name} still there, cipher ok")
            }

            case("用保存的源连接并列出根目录") {
                val record = store.read().getOrThrow()
                    .firstOrNull { it.protocol == SourceRecord.PROTOCOL_SMB }
                    ?: error("没有 SMB 源")
                val address = SmbAddress.parse(record.address)
                SmbSession(address, SmbCredential(username = record.username, password = record.password))
                    .use { session ->
                        session.connect()
                        val entries = session.list("")
                        log("BROWSE", "${record.name} → ${entries.size} 项: " +
                            entries.take(8).joinToString(",") { it.name })
                        check(entries.isNotEmpty()) { "根目录为空" }
                    }
            }

            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n"))
            })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", report.joinToString("\n") + "\nFAIL ${e.stackTraceToString()}")
            })
        }
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }

    private fun log(tag: String, message: String) {
        val line = "P243 $tag $message"
        report += line
        println(line)
        android.util.Log.i("P243S4", "$tag $message")
    }
}
