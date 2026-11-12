package com.kivo.cinema.ui.compose

import android.util.Log
import com.kivo.cinema.dlna.DlnaDevice

/**
 * DLNA 投屏（230）：按钮 → sheet → CastController。
 *
 * 扫描与 SOAP 都是网络往返，一律后台线程；只有结果回主线程落状态。
 */

internal fun PlayerComposeActivity.openCastSheet() {
    smbTrace("cast sheet open, scanning begin")
    castSheetVisible = true
    castScanning = true
    castMessage = ""
    Thread {
        val list = try {
            castController.scan { line -> smbTrace("cast $line") }
        } catch (e: Exception) {
            Log.w(PlayerComposeActivity.TAG, "dlna scan failed", e)
            smbTrace("cast scan EXC ${e.javaClass.name}: ${e.message}")
            emptyList()
        }
        smbTrace("cast scan done: ${list.size} devices ${list.joinToString(";") { it.friendlyName }}")
        runOnUiThread {
            castDevices = list
            castScanning = false
            if (list.isEmpty()) castMessage = "4s 内未发现渲染器"
        }
    }.start()
}

internal fun PlayerComposeActivity.onCastSelect(device: DlnaDevice) {
    smbTrace("cast select ${device.friendlyName} ctrl=${device.controlUrl}")
    castMessage = "连接 ${device.friendlyName}…"
    Thread {
        val title = java.io.File(
            currentMediaPath().ifBlank { playUrlOverride },
        ).name.ifBlank { "kivo" }
        val rc = when {
            // SMB 片源投屏需要第二条 SmbByteSource 给渲染器取流，尚未开放。
            smbPlayback -> -2
            playUrlOverride.isNotBlank() -> castController.castUrl(device, playUrlOverride, title)
            else -> castController.castSource(
                device, com.kivo.cinema.net.FileByteSource(currentMediaPath()), title)
        }
        runOnUiThread {
            if (rc == 0) {
                castDevice = device
                castSheetVisible = false
                // D6：渲染端成为主屏，本地暂停。软解会话无 pause API，偏差记 status 文档。
                runCatching { playbackSession?.pause() }
                statusText = "投屏中：${device.friendlyName}"
            } else {
                castMessage = if (rc == -2) "SMB 片源投屏暂未开放" else "投屏失败（rc=$rc）"
            }
        }
    }.start()
}