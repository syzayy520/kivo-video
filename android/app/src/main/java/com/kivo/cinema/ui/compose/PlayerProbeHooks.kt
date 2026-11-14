package com.kivo.cinema.ui.compose

import android.util.Log
import com.kivo.cinema.playback.MpvPlaybackSession

/**
 * 取证钩子（253-P6-2 字幕 / 215 缩略图压测）。
 *
 * 【为什么与播放页 UI 分开】这两段都由 intent extra 驱动、只在取证时跑，
 * 不是用户功能。混在 Activity 里会让"组合根"读起来像业务代码。
 */

/**
 * 按 intent 指定轨道选字幕，并可 seek + 冻结 + 截图。
 *
 * 用法：
 * ```
 * am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
 *   --es kivo.sourceId <id> --es kivo.smbPath <path> \
 *   --ei kivo.subtitleId 2 --ei kivo.seekMs 14014 --ez kivo.freeze true
 * ```
 */
internal fun PlayerComposeActivity.applyProbeSubtitle(session: MpvPlaybackSession) {
    val subtitleId = intent.getIntExtra("kivo.subtitleId", -1)
    val seekSec = intent.getIntExtra("kivo.seekSec", -1)
    // PGS 是位图字幕，一个包只覆盖几十毫秒（实测中文轨 14.014–14.056，42ms）。
    // 整秒 seek 会落在包外 ⇒ 必然拍到"没有字幕"，看起来像渲染坏了。
    val seekMs = intent.getIntExtra("kivo.seekMs", -1)
    val freeze = intent.getBooleanExtra("kivo.freeze", false)

    // 【顺序不可换】mpv 切字幕轨会做一次轨道刷新（日志 `refresh seek to 3.000000`），
    // 把位置打回该轨自己的起点。先 seek 后选轨 ⇒ 位置从 14s 被打回 3s，
    // 而字幕 13.639s 才开始 ⇒ "字幕不上屏"其实是取证落在了没有字幕的时刻。
    // 所以：先选轨 → 等落定 → 再 seek → 最后暂停。
    val selectDelayMs = 1_500L
    val seekDelayMs = selectDelayMs + 1_200L

    if (subtitleId >= 0) {
        android.os.Handler(mainLooper).postDelayed({
            val ok = session.selectSubtitle(subtitleId)
            smbTrace("probe subtitle id=$subtitleId ok=$ok")
            statusText = if (ok) "字幕轨 #$subtitleId 已选中"
            else "字幕轨 #$subtitleId 选中失败：${session.lastError}"
            // 外挂 SRT 走 libass 文本通路，PGS 走位图通路 —— 挂一条 SRT 就能一刀切开：
            // SRT 能显示 ⇒ 合成通路正常，问题在位图解码；SRT 也不显示 ⇒ 整个字幕层有问题。
            val subFile = intent.getStringExtra("kivo.subFile")
            if (!subFile.isNullOrBlank()) {
                val file = java.io.File(filesDir, subFile)
                if (file.isFile) {
                    val added = session.loadExternalSubtitle(file)
                    smbTrace("probe sub-add $subFile added=$added")
                    // mpv 不会自动选外挂轨，挂上后必须选中。
                    if (added) {
                        val pick = session.tracks.externalSubtitleIds().firstOrNull()
                        if (pick != null) {
                            val sel = session.selectSubtitle(pick)
                            smbTrace("probe sub-select id=$pick ok=$sel")
                            statusText = "外挂字幕 #$pick 已选中"
                        }
                    }
                } else {
                    smbTrace("probe sub-add $subFile NOT FOUND")
                }
            }
        }, selectDelayMs)
    }

    val targetMs = if (seekMs >= 0) seekMs.toLong() else if (seekSec > 0) seekSec * 1000L else -1L
    if (targetMs < 0) return
    android.os.Handler(mainLooper).postDelayed({
        session.seekTo(targetMs * 1000L)
        if (!freeze) return@postDelayed
        // 等落位完成再停 —— 立刻 pause 可能停在跳转途中。
        android.os.Handler(mainLooper).postDelayed({
            session.setPaused(true)
            smbTrace("probe freeze at ${targetMs}ms")
            probeScreenshot(session)
        }, 2_500)
    }, seekDelayMs)
}

/**
 * 取证钩子：播放中**运行时**改 mpv 选项，做 A/B 取证。
 *
 * 用法：
 * ```
 * am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
 *   --es kivo.playurl <url> --es kivo.mpvOpt "demuxer-max-bytes=8MiB,hwdec=mediacodec"
 * ```
 *
 * 【为什么要有这个钩子】内存/卡顿这类问题要归因到**某一个**选项，
 * 而每次改代码重编装机一轮要好几分钟。把改选项做成运行时下发，
 * 一次装机就能把候选组合逐个跑完，每组都有回执与内存读数。
 *
 * 【不许静默】每项 setProperty 的回执都写进 trace —— 改不动的选项要看得见。
 */
internal fun PlayerComposeActivity.applyProbeMpvOptions(session: MpvPlaybackSession) {
    val spec = intent.getStringExtra("kivo.mpvOpt")?.trim().orEmpty()
    if (spec.isEmpty()) return
    val items = spec.split(',').map { it.trim() }.filter { it.isNotEmpty() }
    if (items.isEmpty()) return
    // 起播后再下发：内核 initialize 之前设置会走 option 通道（受回执检查约束），
    // 这里走的是 property 通道，用于**已经跑起来**的会话。
    android.os.Handler(mainLooper).postDelayed({
        items.forEach { item ->
            val name = item.substringBefore('=').trim()
            val value = item.substringAfter('=', "").trim()
            val ok = runCatching { session.native().setProperty(name, value) }.getOrDefault(false)
            smbTrace("probe mpv-opt $name=$value ok=$ok")
        }
    }, 3_000)
}

/**
 * 让 mpv 自己截"合成后的画面"。
 *
 * 【为什么必须在播放页而不是 instrumentation】`screenshot-to-file ... window` 走
 * MODE_FULL_WINDOW，尺寸取自 `osd_get_vo_res`。instrumentation 用 SurfaceTexture(0)
 * 虚拟纹理，没有消费者 ⇒ VO 退化成 1×1 ⇒ 截出 166 字节空图。只有真实 Surface 才有意义。
 *
 * 【两张图回答什么】window 有字幕 ⇒ 内部渲染正常，问题在送 Surface；
 * window 没有但 subtitles 有 ⇒ 生成了没合成；两张都没有 ⇒ 内部就没生成。
 */
private fun PlayerComposeActivity.probeScreenshot(session: MpvPlaybackSession) {
    val shot = intent.getStringExtra("kivo.shot")
    if (shot.isNullOrBlank()) return
    // 诊断用：把字幕往上顶。出现 ⇒ 位置问题；仍不出现 ⇒ 位图没进合成。
    val subPos = intent.getStringExtra("kivo.subPos")
    if (!subPos.isNullOrBlank()) {
        val ok = session.native().setProperty("sub-pos", subPos)
        smbTrace("probe sub-pos=$subPos ok=$ok")
    }
    val out = java.io.File(filesDir, "p253-shots").apply { mkdirs() }
    val windowShot = java.io.File(out, shot)
    val subOnly = java.io.File(out, shot.replace(".png", "-subs.png"))
    windowShot.delete()
    val okWindow = session.command("screenshot-to-file", "${windowShot.absolutePath} window")
    val okSubs = session.command("screenshot-to-file", "${subOnly.absolutePath} subtitles")
    Thread.sleep(1_200)
    smbTrace(
        "probe shot $shot window=$okWindow(${if (windowShot.isFile) windowShot.length() else -1}) " +
            "subs=$okSubs(${if (subOnly.isFile) subOnly.length() else -1})",
    )
    statusText = "截图：window ${if (windowShot.isFile) windowShot.length() else -1}B" +
        " / subs ${if (subOnly.isFile) subOnly.length() else -1}B"
}

// ---- 273-E 取证：`--es kivo.thumbOpt "name=value,name=value"`` 追加预览内核的 mpv 选项 ----
// 预览实例"取一帧却读了 38 MiB"（真机 273-E）需要归因到**某一个**选项，
// 而每次改代码重编装机一轮要好几分钟。把改选项做成运行时下发，
// 一次装机就能把候选组合逐个跑完（每组都有回执与下载量读数）。
internal fun PlayerComposeActivity.probeThumbOptions(): List<Pair<String, String>> {
    val spec = intent.getStringExtra("kivo.thumbOpt")?.trim().orEmpty()
    if (spec.isEmpty()) return emptyList()
    return spec.split(',')
        .map { it.trim() }
        .filter { it.isNotEmpty() }
        .mapNotNull { item ->
            val name = item.substringBefore('=').trim()
            val value = item.substringAfter('=', "").trim()
            if (name.isEmpty()) null else name to value
        }
}

// ---- v3.1 取证：`--ei kivo.thumbDumpSec N` 在 N 秒后落缩略图证据（SMB 拖动预览用）----

internal fun PlayerComposeActivity.scheduleThumbDump() {
    val delaySec = intent.getIntExtra("kivo.thumbDumpSec", -1)
    if (delaySec < 0) return
    android.os.Handler(mainLooper).postDelayed({
        val reader = thumbnailReader ?: return@postDelayed
        // evidenceJson 要等解码线程最多 2s：放主线程就是取证自己造卡顿，
        // 且解码被 SMB 慢读占住时曾返回空串 → JSONObject 抛异常闪退。
        Thread {
            val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
            val root = org.json.JSONObject()
            root.put("schema", "kivo.v31.thumbnail/1")
            root.put("generated_at_unix_ms", System.currentTimeMillis())
            root.put("source", intent.getStringExtra("kivo.smbPath").orEmpty().ifBlank { currentMediaPath() })
            root.put("thumbnail", org.json.JSONObject(reader.evidenceJson()))
            java.io.File(dir, "v31-thumb.json").writeText(root.toString(2))
            smbTrace("v31 thumb evidence written")
        }.start()
    }, delaySec * 1000L)
}

// ---- v3.1 取证：`--ei kivo.thumbProbeSec S` 确定性预览探针 ----
// 自动化拖动手势难稳定命中轨道（视频层横滑会被当成 seek 手势吃掉）。探针直接
// seekTo(S) 并把控制条+浮窗强制可见，给截屏留确定窗口；grabs[] 的 px0 同时
// 证明 EGL→ImageReader 传输是否真的落到非黑帧（红清屏二分后的验收口径）。
internal fun PlayerComposeActivity.scheduleThumbProbe() {
    val sec = intent.getIntExtra("kivo.thumbProbeSec", -1)
    if (sec < 0) return
    android.os.Handler(mainLooper).postDelayed({
        val reader = thumbnailReader ?: return@postDelayed
        controlsVisible = true
        lastTouchMs = System.currentTimeMillis()
        previewMs = sec * 1000L
        reader.seekTo(sec * 1_000_000L)
        android.os.Handler(mainLooper).postDelayed({ previewMs = -1L }, 5000)
    }, 6000)
}

// ---- v3.1d 取证：`--ei kivo.thumbScan N` 网络预览 N 点拖动扫描 ----
// p215 只跑本地 ThumbnailReader；SMB 的 MpvThumbnailReader 需要自己的扫描。
// 250ms 间距模拟连续拖动，latest-wins 自然丢弃被超越的请求；warm 分位数即
// "手指停下后画面多久跟上"。末尾补一次已缓存位置证明 cache-hit 通道瞬时。
internal fun PlayerComposeActivity.scheduleThumbScan() {
    val points = intent.getIntExtra("kivo.thumbScan", 0)
    if (points <= 0) return
    android.os.Handler(mainLooper).postDelayed({ runThumbScan(0, points) }, 8000)
}

internal fun PlayerComposeActivity.runThumbScan(index: Int, points: Int) {
    val reader = thumbnailReader ?: return
    if (index >= points) {
        // 回到扫描中段（必已缓存）：cache-hit 通道应瞬时出帧。
        if (durationMs > 0) reader.seekTo(durationMs / 2 * 1000L)
        android.os.Handler(mainLooper).postDelayed({ writeThumbScanEvidence() }, 2500)
        return
    }
    if (durationMs > 0) {
        val posUs = ((index + 0.5) / points * durationMs).toLong() * 1000L
        previewMs = posUs / 1000
        controlsVisible = true
        lastTouchMs = System.currentTimeMillis()
        reader.seekTo(posUs)
    }
    android.os.Handler(mainLooper).postDelayed({ runThumbScan(index + 1, points) }, 250)
}

internal fun PlayerComposeActivity.writeThumbScanEvidence() {
    val reader = thumbnailReader ?: return
    Thread {
        val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
        val root = org.json.JSONObject()
        root.put("schema", "kivo.v31.thumbnail/1")
        root.put("generated_at_unix_ms", System.currentTimeMillis())
        root.put("source", intent.getStringExtra("kivo.smbPath").orEmpty())
        root.put("scan_points", intent.getIntExtra("kivo.thumbScan", 0))
        root.put("thumbnail", org.json.JSONObject(reader.evidenceJson()))
        java.io.File(dir, "v31-thumb.json").writeText(root.toString(2))
        smbTrace("v31 thumb scan evidence written")
    }.start()
}

// ---- 215 取证 harness（`kivo.evidence=p215`）：60 点扫描 + 主链路 writeStats 对照 ----

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
        reader.seekTo(stressDurationUs / 4)   // 60 点覆盖全片，1/4 处必已缓存
        android.os.Handler(mainLooper).postDelayed({ finishP215Stress(evidenceDir) }, 1500)
        return
    }
    if (index == 30 && evidenceName == "p215") {
        // 预览演示窗口：与拖动共用同一条浮窗 UI（给截图留确定性窗口）。
        previewMs = (stressDurationUs / 2) / 1000
        controlsVisible = true
        android.os.Handler(mainLooper).postDelayed({ previewMs = -1L }, 4000)
    }
    if (stressDurationUs > 0) {
        reader.seekTo(((index + 0.5) / 60.0 * stressDurationUs).toLong())
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