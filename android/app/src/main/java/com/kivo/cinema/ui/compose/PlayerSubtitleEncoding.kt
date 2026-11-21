package com.kivo.cinema.ui.compose

import com.kivo.cinema.mpv.MpvTrackCatalog
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.playback.subtitle.SubtitleCodecMemory
import com.kivo.cinema.playback.subtitle.SubtitleImportStore
import java.io.File

/**
 * 字幕**编码切换**（W3-R1 的写侧）：取当前外挂字幕 → 用指定编码重解原始字节 → 摘旧轨重挂 → 记忆。
 *
 * 【为什么单独成文件】它属于"换编码"这一条变化原因，与"从哪导入字幕"（SAF / 剪贴板 / 链接）不同，
 * 也与"挂载与去重"（[attachImported]）不同 —— 拆开后两边都只在自己的变化原因上改。
 *
 * 【与内核属性的关系】真机实测 `track-list/count` 与 `current-tracks/sub/<字段>` 在某些会话**都读不到**，
 * 所以 fileKey 优先用挂载时自己记下的 `lastSubtitleFileKey`，属性只作兜底。
 */

/**
 * 当前**选中**的外挂字幕的 fileKey（= 库内文件名去掉扩展名）；无选中外挂字幕 ⇒ null。
 */
internal fun PlayerComposeActivity.currentSubtitleFileKey(): String? =
    currentExternalSubtitleTrack()?.let { subtitleFileKey(it) }

/**
 * 按指定编码重解原始字节 → 覆盖库内文件 → 摘旧轨重挂 → 记住选择（R1/R2/R3）。
 *
 * 【为什么先摘轨】重编码只改磁盘内容，内核里那条轨仍持有旧的解析结果；
 * 不摘直接 `sub-add` 会多出一条指向同一文件的轨（内容还是乱码）。
 * 【为什么重挂要走 attachImported】复用"已加载则只选中"的去重口径 ——
 * 其他片源共用同一文件时轨道仍在，不能再加一条。
 */
internal fun PlayerComposeActivity.applySubtitleEncoding(codec: String) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法切换编码")
        return
    }
    // ---- 取"当前外挂字幕"的 fileKey ----
    // 【为什么不只走 track-list】真机实测：某些会话 `track-list/count` 读不到 ⇒
    // `subtitleTracks()` 为空、按 id 找不到当前轨，而同会话 `sid` 与 `current-tracks/sub/*` 都正常。
    // 所以①先用 mpv 0.36+ 的 `current-tracks/sub/*` 直读（不依赖遍历），②读不到再退回遍历。
    val extFlag = mpv.native().property("current-tracks/sub/external")
    val extPath = mpv.native().property("current-tracks/sub/external-filename")
    val sid = mpv.native().property("sid")?.toIntOrNull() ?: -1
    val fileKeyFromProps = if (extFlag == "yes" && !extPath.isNullOrBlank()) {
        runCatching { File(extPath).nameWithoutExtension }.getOrNull()?.takeIf { it.isNotBlank() }
    } else {
        null
    }
    // ③ 最后一条：本会话"最近挂上的字幕"（挂载时我们自己记的）—— 内核属性全不可读时兜底。
    val fromMemory = lastSubtitleFileKey
    val track = if (fileKeyFromProps == null && fromMemory == null) currentExternalSubtitleTrack() else null
    val fileKey = fileKeyFromProps ?: fromMemory ?: track?.let { subtitleFileKey(it) }
    val oldId = when {
        fileKeyFromProps != null -> sid
        fromMemory != null -> sid   // 记录值与 sid 同源：内核选中的就是它
        else -> track?.id ?: -1
    }
    // 取证：ext/path 直接反映内核当前字幕；key=null 说明既没选中外挂轨、也不在库里。
    smbTrace(
        "sub-codec req=$codec ext=$extFlag path=$extPath sid=$sid " +
            "fromProps=${fileKeyFromProps ?: "null"} memory=${fromMemory ?: "null"} " +
            "fallback=${track?.id ?: "null"} key=${fileKey ?: "null"}",
    )
    if (fileKey == null || oldId < 0) {
        showToast("先选择一条外挂字幕")
        return
    }
    smbTrace("sub-enc apply fileKey=$fileKey codec=$codec id=$oldId")
    // 位置/暂停在**主线程**读：mpv 的 client API 只允许在 mpv 自己的线程里访问属性，
    // 后台 Thread 调 `mpv_get_property_*` 会失败并返回 null —— 这就是 `pos=0.000` 的根因
    // （不是"时间没保持"，是**属性只能在主线程读**；`sid` 恰好也是在主线程读的，所以它能读到）。
    val pos = mpv.readProperty("time-pos")?.toDoubleOrNull() ?: 0.0
    val wasPaused = mpv.readProperty("pause") == "yes"
    smbTrace("sub-enc pre-pos pos=$pos paused=$wasPaused")
    Thread {
        val store = SubtitleImportStore(this) { smbTrace(it) }
        val imported = store.reencode(fileKey, codec)
        if (imported == null) {
            smbTrace("sub-enc failed fileKey=$fileKey codec=$codec")
            runOnUiThread { showToast("该编码解不开这份字幕") }
            return@Thread
        }
        val memory = SubtitleCodecMemory(this)
        if (codec == "auto") memory.clear(fileKey) else memory.save(fileKey, codec)
        // 【为什么不再 remove + add】真机实测 `sub-remove <sid>` 在某些会话**不被内核接受**
        // （`Run command: sub-remove, args=[id="1"]` 之后没有 `Track removed`）⇒ 摘不掉旧轨。
        // 改用**重新 loadfile**：mpv 会按磁盘上的新内容重建全部轨，再把库内字幕挂回去。
        // 代价是一次短暂重开播放 —— 位置与暂停都保持，这不影响正确性。
        val media = currentMediaPath()
        if (media.isBlank()) {
            smbTrace("sub-enc fail: no media path")
            runOnUiThread { showToast("无法重开片源，编码未切换") }
            return@Thread
        }
        // mpv 的 `start` **不是** per-file option（`loadfile` 的第 4 参数只收 key=value 的选项，
        // `start` 不在其中）⇒ 位置恢复走 `seek`。⚠️ `seek <sec>` 默认是**相对** seek（2026-11-21
        // 真机 trace 揭穿：异常轮 pos_after ≈ target+pos，正常轮其实是 seek 被竞态吞掉 +
        // loadfile 同文件命中 demuxer 缓存**无缝续播**的假象）——必须带 `absolute` 标志才是
        // "跳到第 pos 秒"。
        var attempts = 0
        var landedAt: Double? = null
        while (attempts < 3 && landedAt == null) {
            attempts++
            Thread.sleep(if (attempts == 1) 220 else 350)
            val seeked = mpv.command(
                "seek",
                String.format(java.util.Locale.US, "%.3f absolute", pos),
            )
            var probes = 0
            while (landedAt == null && probes < 5) {
                Thread.sleep(150)
                probes++
                val now = mpv.readProperty("time-pos")?.toDoubleOrNull()
                if (now != null && kotlin.math.abs(now - pos) <= 0.5) landedAt = now
            }
            smbTrace("sub-enc seek-check attempt=$attempts seeked=$seeked target=$pos landed=$landedAt")
        }

        val added = mpv.tracks.addExternalSubtitle(imported.file.absolutePath, imported.title)
        if (!added) {
            smbTrace("sub-enc fail: re-add rejected")
            runOnUiThread { showToast("字幕重新挂载失败") }
            return@Thread
        }
        if (wasPaused) mpv.native().setProperty("pause", "yes")
        // 取证：重开后的实际位置 —— 优先用核对通过的落位读数；3 次都没落位才回退实时读，
        // 且此时 trace 里必有 `seek-check attempt=3 landed=null`，证据链不静默。
        smbTrace("sub-enc reloaded pos_after=${landedAt ?: mpv.readProperty("time-pos")} paused_now=${mpv.readProperty("pause")}")
        runOnUiThread {
            panelRevision++
            showToast(if (codec == "auto") "字幕编码已恢复自动" else "字幕编码已切换为 $codec")
        }
        smbTrace("sub-enc ok(reload) fileKey=$fileKey codec=$codec file=${imported.file.name}")
    }.start()
}

/** 当前选中的外挂字幕轨；没选 / 选的是内嵌轨 ⇒ null。 */
private fun PlayerComposeActivity.currentExternalSubtitleTrack(): MpvTrackCatalog.Track? {
    val catalog = mpvNetSession?.tracksOrNull() ?: return null
    val id = catalog.currentTrackId("sub")
    return catalog.subtitleTracks().firstOrNull { it.id == id && it.external }
}

/** fileKey 与 [SubtitleImportStore.reencode] 同口径：库内文件名去掉扩展名。 */
private fun subtitleFileKey(track: MpvTrackCatalog.Track): String? {
    if (track.externalFilename.isBlank()) return null
    return runCatching { File(track.externalFilename).nameWithoutExtension }
        .getOrNull()?.takeIf { it.isNotBlank() }
}
