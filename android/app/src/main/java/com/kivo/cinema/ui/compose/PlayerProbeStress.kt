package com.kivo.cinema.ui.compose

import android.util.Log

// ---- 215 取证 harness（`kivo.evidence=p215`）：60 点扫描 + 主链路 writeStats 对照 ----
// 与字幕/选项/缩略图探针分开：这一份是本地解码压测，驱动 scrub 与预览reader，
// 收的是 p215-stats-{baseline,stress}.json 与 p215-thumb.json。

internal fun PlayerComposeActivity.scheduleP215Stress() {
    android.os.Handler(mainLooper).postDelayed({ runP215Stress(0) }, 2000)
}

internal fun PlayerComposeActivity.runP215Stress(index: Int) {
    val session = playbackSession ?: return
    val reader = thumbnailReader ?: return
    val evidenceDir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
    if (index == 0) {
        statusText = "p215 baseline writeStats..."
        session.writeStats("${evidenceDir.absolutePath}/p215-stats-baseline.json")
        stressDurationUs = probeDurationUs(currentMediaPath())
    }
    if (index >= 60) {
        // 扫描完补一次已缓存位置（cache_hit 通道），再收证据。
        statusText = "p215 scan done, cache-hit probe..."
        val posMs = stressDurationUs / 4 / 1000
        previewMs = posMs
        previewRequestedMs = posMs
        scrub.update(posMs)
        reader.seekTo(posMs * 1000)   // 60 点覆盖全片，1/4 处必已缓存
        android.os.Handler(mainLooper).postDelayed({ finishP215Stress(evidenceDir) }, 1500)
        return
    }
    if (stressDurationUs > 0) {
        // 截断到整毫秒：回调身份校验按 previewMs*1000 == positionUs 精确比对。
        val posMs = ((index + 0.5) / 60.0 * stressDurationUs).toLong() / 1000
        previewMs = posMs
        previewRequestedMs = posMs
        if (!scrub.active) scrub.begin(posMs) else scrub.update(posMs)
        reader.seekTo(posMs * 1000)
        if (index == 30 && evidenceName == "p215") {
            // 预览演示窗口：与拖动共用同一条浮窗 UI（给截图留确定性窗口）。
            controlsVisible = true
            lastTouchMs = System.currentTimeMillis()
            android.os.Handler(mainLooper).postDelayed({ previewMs = -1L }, 4000)
        }
    }
    statusText = "p215 scan ${index + 1}/60"
    android.os.Handler(mainLooper).postDelayed({ runP215Stress(index + 1) }, 250)
}

internal fun PlayerComposeActivity.finishP215Stress(evidenceDir: java.io.File) {
    val session = playbackSession ?: return
    val reader = thumbnailReader ?: return
    session.writeStats("${evidenceDir.absolutePath}/p215-stats-stress.json")
    val meta = org.json.JSONObject()
    meta.put("device_model", android.os.Build.MODEL)
    meta.put("device_product", android.os.Build.PRODUCT)
    meta.put("media", mediaOverridePath.ifBlank { "located-test-media" })
    meta.put("stress_points", 60)
    meta.put("scan_spacing_ms", 250)
    val root = org.json.JSONObject()
    root.put("schema", "kivo.p215.thumbnail/1")
    root.put("generated_at_unix_ms", System.currentTimeMillis())
    root.put("meta", meta)
    root.put("thumbnail", org.json.JSONObject(reader.evidenceJson()))
    val out = java.io.File(evidenceDir, "p215-thumb.json")
    out.writeText(root.toString(2))
    statusText = "p215 evidence written: ${out.name}"
    Log.i(PlayerComposeActivity.TAG, "p215 evidence: $out")
}