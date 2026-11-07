package com.kivo.cinema.smb

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 220 AC0 选型 spike（设计稿 §四）+ 协商参数矩阵诊断。
 *
 * 用法（adb）：
 *   am start -n com.kivo.cinema/.smb.SmbSpikeActivity \
 *     --es kivo.smb '{"host":"192.168.5.1","port":445,"share":"视频","path":"电影"}'
 *
 * 取证：协商参数矩阵一次跑完（default/signing/encryption/dialect 钳位），
 * 首个胜者继续 connect→list 两级→offset 读 1MB；全程写
 * files/evidence/p220-spike.json（schema kivo.p220.spike/1）。
 */
class SmbSpikeActivity : ComponentActivity() {

    private var statusText by mutableStateOf("smb spike idle")

    companion object {
        private const val TAG = "KivoSmb"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val cfgJson = intent.getStringExtra("kivo.smb") ?: ""
        setContent {
            MaterialTheme(colorScheme = darkColorScheme()) {
                Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
                    androidx.compose.material3.Text(
                        text = statusText,
                        color = Color.White,
                        fontSize = 11.sp,
                        modifier = Modifier.align(androidx.compose.ui.Alignment.TopStart)
                            .background(Color.Black.copy(alpha = 0.75f))
                            .padding(4.dp)
                    )
                }
            }
        }
        // jcifs 全链路日志落文件（诊断 TreeConnect 拒绝；收口前移除）
        val logDir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
        System.setProperty("org.slf4j.simpleLogger.logFile", java.io.File(logDir, "jcifs-trace.log").absolutePath)
        System.setProperty("org.slf4j.simpleLogger.defaultLogLevel", "trace")
        System.setProperty("org.slf4j.simpleLogger.showDateTime", "true")
        if (cfgJson.isNotBlank()) {
            Thread { runSpike(cfgJson) }.start()
        } else {
            statusText = "missing kivo.smb extra"
        }
    }

    private class Step(val name: String) {
        val t0 = android.os.SystemClock.elapsedRealtime()
        var ok = true
        var info = org.json.JSONObject()
        fun done(): org.json.JSONObject = info
            .put("step", name)
            .put("elapsed_ms", android.os.SystemClock.elapsedRealtime() - t0)
            .put("ok", ok)
    }

    /** 协商参数矩阵（TreeConnect 被拒的诊断轮）。 */
    private val matrix: List<Pair<String, Map<String, String>>> = listOf(
        // v2 矩阵：基线=PC 成功路径（SPNEGO，无 rawNTLM）；PC 上 GUEST 成功发的是
        // guest 用户名 + NTLMv2（lm24/ntlm252），K40 从未测过这个组合
        "default-spnego-guest" to emptyMap(),
        "rawNTLM" to mapOf("jcifs.smb.useRawNTLM" to "true"),
        "signingEnforced" to mapOf("jcifs.smb.client.signingEnforced" to "true"),
        "dialect300" to mapOf("jcifs.smb.client.maxVersion" to "SMB300"),
        "smb2Negotiation" to mapOf("jcifs.smb.client.useSMB2Negotiation" to "true"),
    )

    private fun runSpike(cfgJson: String) {
        val steps = mutableListOf<Step>()
        fun step(name: String): Step = Step(name).also { steps.add(it) }
        var ok = true
        val root = org.json.JSONObject()
        root.put("schema", "kivo.p220.spike/1")
        root.put("generated_at_unix_ms", System.currentTimeMillis())
        val cfg = SmbConfig.fromJson(cfgJson)
        try {
            if (cfg == null) throw IllegalArgumentException("bad kivo.smb json")
            root.put("meta", org.json.JSONObject()
                .put("host", cfg.host).put("port", cfg.port)
                .put("share", cfg.share).put("path", cfg.path)
                .put("user_empty", cfg.username.isEmpty()))

            // ---- 阶段 A：libsmb2（SenPlayer 同款库）× 用户名候选扫描 ----
            // 用户名空 → JNI 传 NULL → libsmb2 默认 user=Login名/"Guest"（init.c L430）
            // 真机此前全拒；扫描其他候选：SenPlayer 空凭据成功说明存在某种可接受形态
            var winner: Smb2Native.Source? = null
            var jcifsWinner: SmbSource? = null
            val libsmb2Results = org.json.JSONArray()
            val userCandidates = listOf("", "guest", "anonymous", "admin", "root")
            var libsmb2Step: Step? = null
            for (u in userCandidates) {
                val s = step("libsmb2_user_" + if (u.isEmpty()) "(null)" else u)
                var src: Smb2Native.Source? = null
                try {
                    src = Smb2Native.Source(cfg.copy(username = u))
                    src.connect()
                    val entries = src.list()
                    s.ok = true
                    s.info.put("entries", entries.size)
                    winner = src
                    libsmb2Step = s
                    break
                } catch (e: Exception) {
                    s.ok = false
                    s.info.put("error", e.message ?: e.javaClass.simpleName)
                    val chain = org.json.JSONArray()
                    var cause: Throwable? = e.cause
                    while (cause != null) {
                        chain.put(cause.javaClass.name + ": " + (cause.message ?: ""))
                        cause = cause.cause
                    }
                    s.info.put("chain", chain)
                    runCatching { src?.close() }
                }
                libsmb2Results.put(s.done())
            }
            root.put("libsmb2", (libsmb2Step ?: steps.last { it.name.startsWith("libsmb2") }).done())
            root.put("libsmb2_matrix", libsmb2Results)

            // ---- 阶段 B（诊断）：jcifs 协商参数矩阵（libsmb2 已通则跳过）----
            val matrixResults = org.json.JSONArray()
            if (winner == null) {
                for ((name, extra) in matrix) {
                    val s = step("variant_$name")
                    val source = SmbSource(cfg, extra)
                    try {
                        source.connect()
                        val entries = source.list()
                        s.ok = true
                        s.info.put("entries", entries.size)
                        jcifsWinner = source
                        break
                    } catch (e: Exception) {
                        s.ok = false
                        s.info.put("error", e.message ?: e.javaClass.simpleName)
                        val chain = org.json.JSONArray()
                        var cause: Throwable? = e.cause
                        while (cause != null) {
                            chain.put(cause.javaClass.name + ": " + (cause.message ?: ""))
                            cause = cause.cause
                        }
                        s.info.put("chain", chain)
                        source.close()
                    }
                    matrixResults.put(s.done())
                }
            }
            root.put("matrix", matrixResults)

            if (winner == null && jcifsWinner == null) {
                ok = false
                val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
                java.io.File(dir, "p220-spike.json").writeText(root.toString(2))
                statusText = "p220 spike FAIL: libsmb2 + all jcifs variants denied"
                Log.i(TAG, statusText)
                return
            }

            // ---- 胜者走全流程：browse 两级 → offset 读 1MB ----
            val s2 = step("browse_root")
            val entries: List<SmbSource.Entry> =
                if (winner != null) winner.list() else jcifsWinner!!.list()
            s2.info.put("count", entries.size)
            val firstDir = entries.firstOrNull { it.isDirectory }
            var fileCandidate: SmbSource.Entry? = entries.firstOrNull { !it.isDirectory }
            var fileDir = cfg.path   // 文件所在目录（相对共享根），open 时要带上
            if (firstDir != null) {
                val subPath = joinPath(cfg.path, firstDir.name)
                val s2b = step("browse_sub")
                val entries2: List<SmbSource.Entry> =
                    if (winner != null) winner.list(subPath) else jcifsWinner!!.list(subPath)
                s2b.info.put("count", entries2.size).put("dir", firstDir.name)
                val sub2 = entries2.firstOrNull { !it.isDirectory }
                if (sub2 != null) {
                    fileCandidate = sub2
                    fileDir = subPath
                }
                fileCandidate?.let { s2b.info.put("file", it.name) }
            }
            if (entries.isEmpty()) {
                s2.ok = false
                ok = false
            }

            if (fileCandidate == null) {
                val s3 = step("read_at")
                s3.ok = false
                s3.info.put("error", "no file found in listing")
                ok = false
            } else {
                val s3 = step("read_at")
                val src: SmbSource.ByteSource =
                    if (winner != null) winner.openForRead(joinPath(fileDir, fileCandidate.name))
                    else jcifsWinner!!.openForRead(joinPath(fileDir, fileCandidate.name))
                val size = src.size()
                val off = if (size > 2L * 1024 * 1024) 1024L * 1024 else 0L
                val buf = ByteArray(1024 * 1024)
                val n = src.readAt(off, buf, 0, buf.size)
                val dt = android.os.SystemClock.elapsedRealtime() - s3.t0
                s3.info.put("file", fileCandidate.name)
                    .put("size", size)
                    .put("offset", off)
                    .put("bytes_read", n)
                    .put("mbps", if (n > 0 && dt > 0) (n / 1024.0 / 1024.0) / (dt / 1000.0) else 0.0)
                if (n <= 0) {
                    s3.ok = false
                    ok = false
                }
                src.close()
            }
            winner?.close()
            jcifsWinner?.close()
        } catch (e: Exception) {
            Log.e(TAG, "spike failed", e)
            ok = false
            val s = step("error")
            s.ok = false
            s.info.put("message", e.message ?: e.javaClass.simpleName)
            s.info.put("exception_class", e.javaClass.name)
            val chain = org.json.JSONArray()
            var cause: Throwable? = e.cause
            while (cause != null) {
                chain.put(cause.javaClass.name + ": " + (cause.message ?: ""))
                cause = cause.cause
            }
            s.info.put("cause_chain", chain)
            s.info.put("stack", Log.getStackTraceString(e).take(2000))
        }
        root.put("ok", ok)
        root.put("steps", org.json.JSONArray().apply { steps.forEach { put(it.done()) } })
        val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
        java.io.File(dir, "p220-spike.json").writeText(root.toString(2))
        statusText = if (ok) "p220 spike PASS -> p220-spike.json" else "p220 spike FAIL -> p220-spike.json"
        Log.i(TAG, statusText)
    }

    private fun joinPath(base: String, name: String): String =
        base.trimEnd('/') + "/" + name
}
