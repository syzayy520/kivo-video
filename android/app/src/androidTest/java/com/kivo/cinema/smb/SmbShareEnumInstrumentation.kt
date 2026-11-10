package com.kivo.cinema.smb

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle

/**
 * S2 真机协议验收（也是 243 S0 的"共享枚举复算"）：枚举 → 连共享 → 列目录 → 随机读。
 *
 * 服务器从 am instrument 参数传入，不写死：
 * ```
 * -Pkivo.testRunner=com.kivo.cinema.smb.SmbShareEnumInstrumentation \
 * -Pandroid.testInstrumentationRunnerArguments.smbHost=192.168.5.1
 * ```
 * 默认匿名（小米路由形态）。全部输出以 `P243 ` 开头，便于从 logcat 归档成证据。
 */
class SmbShareEnumInstrumentation : Instrumentation() {

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
        val credential = SmbCredential(
            username = args?.getString("smbUser") ?: "",
            password = args?.getString("smbPass") ?: "",
            domain = args?.getString("smbDomain") ?: "",
        )
        val port = args?.getString("smbPort")?.toIntOrNull() ?: SmbAddress.DEFAULT_PORT
        // 服务器对匿名账号会隐藏部分共享（小米路由实测：枚举只给 XiaoMi，
        // 但 net view 里有 6 个）。所以允许直接指定共享名复算"手填"这条路。
        val forcedShare = args?.getString("smbShare")?.takeIf { it.isNotBlank() }
        val forcedPath = args?.getString("smbPath")?.takeIf { it.isNotBlank() }
        var share: String? = forcedShare
        var filePath: String? = null
        try {
            case("共享枚举（IPC$ + NetrShareEnum）") {
                val shares = SmbShareLister(host, port, credential).list()
                log("SHARES", shares.joinToString(",") { "${it.name}|${it.remark}|${it.type}" })
                if (share == null) {
                    check(shares.isNotEmpty()) { "服务器没有返回可浏览的磁盘共享（检查账号或共享设置）" }
                    share = shares.first().name
                } else {
                    log("SHARES", "指定共享=$share（枚举命中共 ${shares.count { it.name == share }} 个）")
                }
            }
            case("连到共享并列出根目录") {
                val address = SmbAddress(host, port, share!!)
                SmbSession(address, credential).use { session ->
                    session.connect()
                    val root = session.list("")
                    log("ROOT", "$share → ${root.size} 项: " +
                        root.joinToString(",") { "${it.name}${if (it.isDirectory) "/" else ""}" })
                    check(root.isNotEmpty()) { "共享根目录为空或不可读" }

                    val rootFile = root.firstOrNull { !it.isDirectory && it.size > 0 }
                    if (rootFile != null) filePath = rootFile.name
                    if (forcedPath != null) {
                        val listing = session.list(forcedPath)
                        log("PATH", "$forcedPath → ${listing.size} 项: " +
                            listing.joinToString(",") { "${it.name}${if (it.isDirectory) "/" else ":" + it.size}" })
                        val picked = listing.firstOrNull { !it.isDirectory && it.size > 0 }
                        if (picked != null) filePath = "$forcedPath/${picked.name}"
                    }
                    // 共享根常常全是目录；逐层往下找第一个可读文件（最多两层）
                    for (directory in root.filter { it.isDirectory }.take(6)) {
                        if (filePath != null) break
                        val child = session.list(directory.name)
                        log("LEVEL2", "${directory.name} → ${child.size} 项")
                        val nestedFile = child.firstOrNull { !it.isDirectory && it.size > 0 }
                        if (nestedFile != null) {
                            filePath = "${directory.name}/${nestedFile.name}"
                            break
                        }
                        val nestedDir = child.firstOrNull { it.isDirectory }
                        if (nestedDir != null) {
                            val deep = session.list("${directory.name}/${nestedDir.name}")
                            log("LEVEL3", "${directory.name}/${nestedDir.name} → ${deep.size} 项")
                            val deepFile = deep.firstOrNull { !it.isDirectory && it.size > 0 }
                            if (deepFile != null) {
                                filePath = "${directory.name}/${nestedDir.name}/${deepFile.name}"
                            }
                        }
                    }
                }
            }
            case("随机读：文件头 1 MiB + 尾部 1 MiB") {
                val path = filePath ?: run {
                    log("READ", "共享里没有可读文件，跳过随机读")
                    return@case
                }
                val address = SmbAddress(host, port, share!!)
                SmbReader(address, credential, path).use { reader ->
                    reader.open()
                    val size = reader.size
                    val buffer = ByteArray(1 shl 20)
                    val head = readFully(reader, 0, buffer, minOf(buffer.size.toLong(), size).toInt())
                    val tailStart = (size - buffer.size.toLong()).coerceAtLeast(0)
                    val tailLength = minOf(buffer.size.toLong(), size - tailStart).toInt()
                    val tail = readFully(reader, tailStart, buffer, tailLength)
                    log("READ", "$path size=$size head=$head tail=$tail")
                    check(head > 0 || tail > 0) { "1 MiB 读失败" }
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

    private fun readFully(reader: SmbReader, offset: Long, buffer: ByteArray, length: Int): Int {
        if (length <= 0) return 0
        var done = 0
        while (done < length) {
            val read = reader.pread(offset + done, buffer, done, length - done)
            if (read <= 0) break
            done += read
        }
        return done
    }

    private fun case(name: String, body: () -> Unit) {
        total += 1
        body()
        passed += 1
        report += "PASS $name"
    }

    private fun log(tag: String, message: String) {
        val line = "P243 $tag $message"
        // 必须进 report：am instrument -w 只回传 finish() 的 stream，println 会被丢掉。
        report += line
        println(line)
        android.util.Log.i("P243SMB", "$tag $message")
    }
}
