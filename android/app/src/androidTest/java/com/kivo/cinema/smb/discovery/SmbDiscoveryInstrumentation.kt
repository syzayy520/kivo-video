package com.kivo.cinema.smb.discovery

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * S5 真机验收：用生产代码 [SmbDiscovery] 发现局域网里的 SMB 服务。
 *
 * 期望（2026-11-09 本机网络，PC 侧 zeroconf 实测）：只能发现 NAS `N1`（192.168.5.197），
 * 小米路由不广播 `_smb._tcp`。若一台都没有，说明这台设备上的 NSD 被 ROM 限制，
 * 需要记录而不是当成"没有 SMB 服务器"。
 */
class SmbDiscoveryInstrumentation : Instrumentation() {

    private val report = mutableListOf<String>()
    private var passed = 0
    private var total = 0

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        val found = java.util.Collections.synchronizedList(mutableListOf<SmbDiscovery.Found>())
        val latch = CountDownLatch(1)
        // 先让应用到前台：MIUI 对后台进程的组播/网络有额外限制，
        // 纯 instrumentation 进程跑 mDNS 会被判"后台"而失败。
        runCatching {
            val intent = android.content.Intent()
                .setClassName(targetContext, "com.kivo.cinema.MainActivity")
                .addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
            targetContext.startActivity(intent)
            Thread.sleep(2_500)
        }
        val discovery = SmbDiscovery(targetContext)
        try {
            case("启动 mDNS 浏览并收集 10 秒") {
                discovery.start { item ->
                    if (found.none { it.host == item.host }) {
                        found += item
                        report += "P243 FOUND ${item.host}:${item.port} ${item.label}"
                    }
                }
                // 最多等 10 秒或至少发现一台后 3 秒
                val deadline = System.currentTimeMillis() + 10_000
                while (System.currentTimeMillis() < deadline) {
                    Thread.sleep(500)
                    if (found.isNotEmpty() && System.currentTimeMillis() > deadline - 7_000) break
                }
            }
            case("结果非空且端口为 445") {
                log("TOTAL", "${found.size} 台: " + found.joinToString(",") { "${it.host}(${it.label})" })
                check(found.isNotEmpty()) {
                    "没有发现任何 _smb._tcp 服务；同网段可能有服务器但不广播（小米路由就是不广播的）"
                }
                check(found.all { it.port == 445 }) { found.joinToString { "${it.port}" } }
            }
            finish(Activity.RESULT_OK, Bundle().apply {
                putString("stream", report.joinToString("\n", postfix = "\n$passed/$total PASS\n"))
            })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply {
                putString("stream", report.joinToString("\n") + "\nFAIL ${e.stackTraceToString()}")
            })
        } finally {
            discovery.stop()
            latch.countDown()
            latch.await(1, TimeUnit.SECONDS)
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
        android.util.Log.i("P243S5", "$tag $message")
    }
}
