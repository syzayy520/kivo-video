package com.kivo.cinema.ui

import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.util.TypedValue
import android.view.Gravity
import android.view.SurfaceHolder
import android.view.WindowManager
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.kivo.cinema.bridge.PlaybackSession
import java.io.File

/**
 * 多路并发解码探针（P12-D2）。
 *
 * 与 `PlayerActivity` 的关系：那个是"一条会话 + 一个面 + 一堆探针开关"，
 * 这条卡要的是"N 条会话同时跑"。塞进同一个 Activity 会让
 * `onSurfaceAvailable` / `onSurfaceLost` 的一对一假设立刻失效（D1 刚把换面做对），
 * 所以单开一个。
 *
 * 明确不做：换面（D1 已验）、seek、音频。这里只回答一个问题：
 * **N 路 1080p 同时解码，各自还能跑多快、会不会互相拖垮。**
 *
 * 证据：每条会话写一份 `kivo.p12c.decode/6`（复用既有渲染器，不新造 schema），
 * 文件名 `<prefix>-<index>.json`。跨会话的比较交给
 * `tools/verify_p12d_multi.py` —— 数字必须从证据里算，不许手填。
 */
class MultiPlayerActivity : AppCompatActivity(), PlayerSurfaceView.Listener {

    private var sessionCount = 3
    private var targetFps = 0
    private var allowDrop = true
    private var atTimePresent = true
    private var evidencePrefix = "p12d2-multi.json"
    private var timeoutMs = 60_000L
    private var mediaPath: String? = null

    private val views = mutableListOf<PlayerSurfaceView>()
    private val readyCells = mutableSetOf<Int>()
    private val sessions = mutableListOf<PlaybackSession>()
    private lateinit var statusView: TextView

    private var started = false
    private var finished = false
    private var beganAtMs = 0L

    private val handler = Handler(Looper.getMainLooper())
    private val pollRunnable = object : Runnable {
        override fun run() {
            if (finished) {
                return
            }
            val elapsed = System.currentTimeMillis() - beganAtMs
            val lines = StringBuilder()
            for (session in sessions) {
                lines.append('#').append(session.index).append(' ')
                    .append(session.summary().take(140)).append('\n')
            }
            statusView.text = "elapsed ${elapsed}ms\n$lines"

            val allDone = sessions.isNotEmpty() && sessions.all { it.isFinished() }
            if (allDone || elapsed >= timeoutMs) {
                finishRun(timedOut = !allDone)
                return
            }
            handler.postDelayed(this, POLL_INTERVAL_MS)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        readExtras()

        val root = FrameLayout(this)
        val row = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }

        for (index in 0 until sessionCount) {
            val view = PlayerSurfaceView(this).apply {
                listener = this@MultiPlayerActivity
                tag = index
            }
            views.add(view)
            row.addView(
                view,
                LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1f)
            )
        }

        root.addView(
            row,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
        )

        statusView = TextView(this).apply {
            setTextColor(Color.WHITE)
            setBackgroundColor(0x99000000.toInt())
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 9f)
            setPadding(16, 16, 16, 16)
        }
        root.addView(
            statusView,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.START or Gravity.TOP
            )
        )
        setContentView(root)

        val version = runCatching { com.kivo.cinema.bridge.NativePlayer.nativeVersion() }
            .getOrElse { "load-failed" }
        Log.i(TAG, "native version: $version, sessions=$sessionCount")
        statusView.text = "native: $version\nwaiting for $sessionCount surfaces…"
    }

    override fun onSurfaceAvailable(holder: SurfaceHolder) {
        val index = views.indexOfFirst { it.holder == holder }
        if (index < 0) {
            return
        }
        readyCells.add(index)
        if (readyCells.size == sessionCount) {
            startAll()
        }
    }

    override fun onSurfaceLost() {
        // D2 不验换面，切后台即结束这一轮：继续跑会污染 fps 数字。
        if (!finished) {
            Log.w(TAG, "surface lost, aborting multi run")
            finishRun(timedOut = true)
        }
    }

    /**
     * 两轮：先全部 open，再全部 start。
     *
     * 一条一条 open+start 会让第一路先跑几十毫秒，测出来的是接力不是并发 ——
     * 而这条卡问的正是"同时跑会怎样"。
     */
    private fun startAll() {
        if (started) {
            return
        }
        started = true

        val media = mediaPath?.let { File(it) }?.takeIf { it.isFile && it.length() > 0 }
            ?: locateMedia()
        if (media == null) {
            statusView.text = "no clip found"
            writeFailure("media", -1, "no media file", "")
            finishRun(timedOut = true)
            return
        }

        for (index in 0 until sessionCount) {
            val session = PlaybackSession(index, media, media.name)
            sessions.add(session)

            val rc = session.openVideo(views[index].holder.surface)
            if (rc != 0) {
                writeFailure("open", rc, session.lastError, "#$index")
                finishRun(timedOut = true)
                return
            }
            // 基线轮（fps = 0）刻意不挂节拍：那几轮量的是满速吞吐，
            // 挂上节拍器量到的就是"节拍器放行速度"，两回事。
            if (targetFps > 0) {
                val pacingRc = session.configurePacing(targetFps, allowDrop)
                if (pacingRc != 0) {
                    writeFailure("pacing", pacingRc, session.lastError, "#$index")
                    finishRun(timedOut = true)
                    return
                }
            }
            val pipelineRc = session.configurePipeline(
                split = false,
                present = true,
                atTime = atTimePresent
            )
            if (pipelineRc != 0) {
                writeFailure("pipeline", pipelineRc, session.lastError, "#$index")
                finishRun(timedOut = true)
                return
            }
        }

        beganAtMs = System.currentTimeMillis()
        for (session in sessions) {
            val rc = session.start()
            if (rc != 0) {
                writeFailure("start", rc, session.lastError, "#${session.index}")
                finishRun(timedOut = true)
                return
            }
        }
        Log.i(TAG, "all $sessionCount sessions started")
        handler.post(pollRunnable)
    }

    private fun finishRun(timedOut: Boolean) {
        if (finished) {
            return
        }
        finished = true
        handler.removeCallbacks(pollRunnable)

        val dir = evidenceDir()
        for (session in sessions) {
            val file = File(dir, evidenceName(evidencePrefix, session.index))
            val rc = session.writeStats(file.absolutePath)
            Log.i(TAG, "stats #${session.index} rc=$rc -> ${file.name}")
        }
        statusView.append("\nwritten ${sessions.size} files" + if (timedOut) " (timeout)" else "")

        for (session in sessions) {
            session.close()
        }
        // 取证模式下写完就退：adb 侧不用猜"这一轮结束没有"。
        handler.postDelayed({ finish() }, 400L)
    }

    override fun onDestroy() {
        for (session in sessions) {
            session.close()
        }
        super.onDestroy()
    }

    private fun readExtras() {
        val intent = intent ?: return
        mediaPath = intent.getStringExtra("kivo.media")
        sessionCount = intent.getIntExtra("kivo.sessions", 3).coerceIn(1, 6)
        targetFps = intent.getIntExtra("kivo.fps", 0)
        allowDrop = intent.getIntExtra("kivo.drop", 1) != 0
        atTimePresent = intent.getStringExtra("kivo.present") != "cpu_paced"
        evidencePrefix = intent.getStringExtra("kivo.evidence") ?: "p12d2-multi.json"
        timeoutMs = intent.getIntExtra("kivo.timeout", 60_000).toLong()
        Log.i(
            TAG,
            "extras: media=$mediaPath sessions=$sessionCount fps=$targetFps " +
                "drop=$allowDrop present=${if (atTimePresent) "at_time" else "cpu_paced"} " +
                "timeout=$timeoutMs prefix=$evidencePrefix"
        )
    }

    private fun locateMedia(): File? {
        val external = File(getExternalFilesDir(null) ?: filesDir, "media")
        return listOf(external, File(filesDir, "media"))
            .asSequence()
            .filter { it.isDirectory }
            .flatMap { dir -> (dir.listFiles()?.sortedBy { it.name } ?: emptyList()).asSequence() }
            .firstOrNull { it.isFile && it.length() > 0 && it.name.endsWith(".mp4") }
    }

    private fun evidenceDir(): File = File(filesDir, "evidence").apply { mkdirs() }

    /** `<prefix>-<index>.json`；prefix 自带扩展名时也拼得出合理名字。 */
    private fun evidenceName(prefix: String, index: Int): String =
        prefix.replace(Regex("\\.json$"), "") + "-" + index + ".json"

    private fun writeFailure(stage: String, rc: Int, reason: String, where: String) {
        val payload = buildString {
            appendLine("{")
            appendLine("  \"schema\": \"kivo.p12d.multi-failure/1\",")
            appendLine("  \"stage\": \"$stage\",")
            appendLine("  \"session\": \"$where\",")
            appendLine("  \"return_code\": $rc,")
            appendLine("  \"reason\": \"${reason.replace("\\", "\\\\").replace("\"", "\\\"")}\"")
            appendLine("}")
        }
        runCatching { File(evidenceDir(), "p12d2-failure.json").writeText(payload) }
        Log.w(TAG, "failure written: $stage $where rc=$rc $reason")
    }

    private companion object {
        const val TAG = "KivoMulti"
        const val POLL_INTERVAL_MS = 200L
    }
}
