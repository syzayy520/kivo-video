package com.kivo.cinema.ui

import android.app.Activity
import android.os.Bundle
import android.util.Log
import com.kivo.cinema.bridge.NativePlayer
import java.io.File

/**
 * 帧率能力探针入口（P13-A）。**没有 UI** —— 采集完就 `finish()`。
 *
 * 为什么单独一个 Activity 而不是塞进 PlayerActivity：
 * 这个动作不需要 Surface、不需要片源、不产生任何播放状态。塞进播放器会让
 * "打开播放器"和"采一次能力"耦合在一起，而后者要能在没有视频的情况下跑。
 *
 * 用法：
 * ```
 * adb shell am start -n com.kivo.cinema/.ui.FramerateDumpActivity \
 *   --es kivo.out p13a-framerate.json
 * adb shell run-as com.kivo.cinema cat files/evidence/p13a-framerate.json
 * ```
 *
 * @return 结果写在 `files/evidence/<kivo.out>`；返回值只进 logcat。
 *         失败时文件不会被创建 —— **不写半截 JSON**。
 */
class FramerateDumpActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val name = intent.getStringExtra("kivo.out") ?: "p13a-framerate.json"
        val target = File(filesDir, "evidence").apply { mkdirs() }.resolve(name)

        // 先删旧文件：如果这次采集失败，留着上一次的结果会让人以为刚刚成功了。
        target.delete()

        val rc = runCatching { NativePlayer.probeFramerates(target.absolutePath) }
            .onFailure { Log.e(TAG, "probe threw", it) }
            .getOrDefault(-1)

        if (rc == 0 && target.exists()) {
            Log.i(TAG, "framerate probe ok -> ${target.absolutePath} (${target.length()} bytes)")
        } else {
            Log.w(TAG, "framerate probe failed rc=$rc target=${target.absolutePath}")
        }
        finish()
    }

    private companion object {
        const val TAG = "KivoFramerateProbe"
    }
}
