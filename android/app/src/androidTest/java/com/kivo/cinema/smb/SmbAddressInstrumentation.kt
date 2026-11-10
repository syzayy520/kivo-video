package com.kivo.cinema.smb

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle

/**
 * SMB 地址解析（243 D2）——纯函数用例，不需要服务器、不需要网络。
 *
 * 跑法：
 * ```
 * ./gradlew :app:connectedDebugAndroidTest \
 *   -Pkivo.testRunner=com.kivo.cinema.smb.SmbAddressInstrumentation
 * ```
 * 输出 `PASS n/m` 或失败栈，风格与仓库其它 Instrumentation 一致（不引 JUnit 依赖）。
 */
class SmbAddressInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var passed = 0
    private var total = 0

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        try {
            case("完整 URI：主机/共享/路径") {
                val address = SmbAddress.parse("smb://192.168.5.1/视频/电影/2024")
                check(address.host == "192.168.5.1") { address.host }
                check(address.port == 445) { address.port }
                check(address.share == "视频") { address.share }
                check(address.path == "电影/2024") { address.path }
                check(address.toUri() == "smb://192.168.5.1/视频/电影/2024") { address.toUri() }
            }
            case("显式端口 + 默认端口不写进存储") {
                val explicit = SmbAddress.parse("smb://nas.local:1445/media")
                check(explicit.port == 1445) { explicit.port }
                check(explicit.toUri() == "smb://nas.local:1445/media") { explicit.toUri() }
                check(SmbAddress.parse("smb://host:445/share").toUri() == "smb://host/share")
            }
            case("只有主机 → 需要先选共享") {
                val address = SmbAddress.parse("192.168.5.197")
                check(address.needsShare)
                check(address.share.isEmpty())
                check(!SmbAddress.parse("host/share").needsShare)
            }
            case("Windows UNC 路径") {
                val address = SmbAddress.parse("""\\192.168.5.1\视频\电影""")
                check(address.host == "192.168.5.1") { address.host }
                check(address.share == "视频") { address.share }
                check(address.path == "电影") { address.path }
            }
            case("裸 主机/共享") {
                val address = SmbAddress.parse("nas/share")
                check(address.host == "nas")
                check(address.share == "share")
            }
            case("scheme 大小写不敏感 + 末尾斜杠") {
                check(SmbAddress.parse("SMB://host/a").host == "host")
                val trailing = SmbAddress.parse("smb://host/share/")
                check(trailing.share == "share")
                check(trailing.path.isEmpty())
            }
            case("共享名里的空格与中文原样保留") {
                check(SmbAddress.parse("smb://host/My Movies").share == "My Movies")
                val address = SmbAddress.parse("smb://host/电影 (2024)/A&B#1")
                check(address.share == "电影 (2024)") { address.share }
                check(address.path == "A&B#1") { address.path }
            }
            case("IPv6 主机") {
                val address = SmbAddress.parse("smb://[fe80::1]/share")
                check(address.host == "fe80::1")
                check(address.toUri() == "smb://[fe80::1]/share") { address.toUri() }
            }
            case("非法输入给出可读原因") {
                expectReject("   ")
                expectReject("smb:///share")
                expectReject("smb://host:70000/share", "端口应为 1–65535")
                expectReject("smb://host:abc/share", "端口应为 1–65535")
            }
            case("filePath 拼接共享与路径") {
                check(SmbAddress.parse("smb://host/视频/电影").filePath("片名.mkv") == "视频/电影/片名.mkv")
                check(SmbAddress.parse("smb://h/share").filePath("a.mkv") == "share/a.mkv")
            }
            case("凭据：匿名判定与密码不进 toString") {
                val credential = SmbCredential("", "alice", "s3cret")
                check(!credential.anonymous)
                check(!credential.toString().contains("s3cret"))
                check(SmbCredential.ANONYMOUS.anonymous)
            }
            case("共享类型：只展示可浏览的磁盘共享") {
                check(SmbShare("视频", "", 0).isBrowsable)
                check(!SmbShare("IPC$", "", 3).isBrowsable)
                check(!SmbShare("HP", "", 1).isBrowsable)
                check(!SmbShare("C$", "", 0).isBrowsable)
                check(SmbShare("归档", "", 0x80000000.toInt()).isHidden)
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

    private fun expectReject(input: String, expected: String? = null) {
        try {
            SmbAddress.parse(input)
            throw IllegalStateException("应被拒绝：$input")
        } catch (e: IllegalArgumentException) {
            check(e.message?.isNotBlank() == true) { "拒绝原因不能为空：$input" }
            if (expected != null) check(e.message == expected) { "期望「$expected」，实际「${e.message}」" }
        }
    }
}
