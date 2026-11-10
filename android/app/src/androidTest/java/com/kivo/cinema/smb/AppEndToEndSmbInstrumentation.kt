package com.kivo.cinema.smb

import android.app.Instrumentation
import android.content.Intent
import android.os.Bundle
import android.util.Log
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.ui.compose.PlayerComposeActivity

/**
 * 真机端到端 harness —— 走**用户实际走的那条路**，不是直接调 native 会话。
 *
 * 存在的理由：`SoftDecodeSmbInstrumentation` 测的是会话本身（open/seek/pause），
 * 它证明不了"在播放页里放不动"这种问题。用户看到的卡顿是解码 + 渲染 + 音画闸门
 * + 主线程轮询叠加后的结果，必须在 Activity 里量才算数。
 *
 * 用法：
 * ```
 * am instrument -w -e smbShare 视频 -e smbPath "电影/xxx/xxx.mkv" -e seconds 40 \
 *   com.kivo.cinema.test/com.kivo.cinema.smb.AppEndToEndSmbInstrumentation
 * ```
 * 期间 App 自己在 `KivoCompose` 下打 `soft rate media=... wall=... ratio=...`。
 */
class AppEndToEndSmbInstrumentation : Instrumentation() {

    private var args: Bundle? = null

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        args = arguments
        start()
    }

    override fun onStart() {
        var ok = true
        var detail = ""
        try {
            val ctx = targetContext
            val path = args?.getString("smbPath")
                ?: "电影/[恐怖废墟].The.Ruins.2008.GBR.UNRATED.BluRay.REMUX.1080p.VC-1.TrueHD.5.1-CMCT/" +
                "[恐怖废墟].The.Ruins.2008.GBR.UNRATED.BluRay.REMUX.1080p.VC-1.TrueHD.5.1-CMCT.mkv"
            val share = args?.getString("smbShare").orEmpty()
            val seconds = args?.getString("seconds")?.toIntOrNull() ?: 30

            val records = SourceStore(ctx).read().getOrNull().orEmpty()
            val record = records.firstOrNull { it.protocol == "smb" }
            if (record == null) {
                ok = false
                detail = "设备上没有 SMB 资源库，共 ${records.size} 条记录"
            } else {
                Log.i(TAG, "[SOURCE] name=${record.name} id=${record.id} addr=${record.address}")
                val intent = Intent(ctx, PlayerComposeActivity::class.java).apply {
                    addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    putExtra("kivo.sourceId", record.id)
                    putExtra("kivo.smbPath", path)
                    putExtra("kivo.smbShare", share)
                }
                ctx.startActivity(intent)
                Log.i(TAG, "[LAUNCH] share=$share watch=${seconds}s path=$path")
                // 采样期间不做任何干预：让 App 自己跑完整链路，日志里出现的
                // `soft rate` 就是用户实际看到的推进速度。
                val deadline = System.currentTimeMillis() + seconds * 1000L
                while (System.currentTimeMillis() < deadline) {
                    Thread.sleep(500)
                }
                detail = "watched ${seconds}s"
                Log.i(TAG, "[DONE] $detail")
            }
        } catch (e: Exception) {
            ok = false
            detail = "${e.javaClass.simpleName}: ${e.message}"
            Log.e(TAG, "[FAIL] $detail", e)
        }
        val bundle = Bundle().apply { putString("result", detail) }
        finish(if (ok) 0 else -1, bundle)
    }

    private companion object {
        const val TAG = "KivoE2E"
    }
}
