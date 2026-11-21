package com.kivo.cinema.ui.compose

import android.content.ClipboardManager
import android.content.Context
import android.net.Uri
import com.kivo.cinema.mpv.MpvTrackCatalog
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.playback.subtitle.SubtitleCodecMemory
import com.kivo.cinema.playback.subtitle.SubtitleImportStore
import com.kivo.cinema.playback.subtitle.SubtitleRemoteFetcher
import com.kivo.cinema.playback.subtitle.SubtitleSecondaryControl
import com.kivo.cinema.playback.subtitle.SubtitleSourceKey
import com.kivo.cinema.playback.subtitle.subtitlePathKey
import java.io.File

/** 远程字幕的下载落点：可被系统清，持久化由 [SubtitleImportStore] 负责。 */
private const val REMOTE_SUBTITLE_CACHE_DIR = "subtitles-remote"

/**
 * 外挂字幕导入（player-ui-v3 §1.6；P9 · D 改为持久目录）。
 *
 * 【落在哪】`filesDir/subtitles`，不是 `cacheDir/subs` —— 后者会被系统清理，
 * 用户"导入过"的字幕下次播放就没了。落盘/去重/按片源登记都在
 * [SubtitleImportStore]，这里只负责：拿 URI → 落盘 → sub-add → 结果回显。
 *
 * 【为什么不走 Intent 传路径】与 SMB 凭据同一条铁律：外部输入只给 URI，
 * 落盘名字由存储层消毒，路径不暴露给任何第三方组件。
 */
internal fun PlayerComposeActivity.launchSubtitleImport() {
    // 定性埋点：验收方 4 次点击均未拉起 SAF 选择器，但 OpenDocument 在任务内渲染
    // （top activity 不变是正常现象），必须用 trace 区分"没点到/launch 抛异常/
    // launch 正常返回"三种结局，不给猜测。
    smbTrace("sub-import: launching SAF picker")
    runCatching { subtitleImportLauncher.launch(arrayOf("*/*")) }
        .onSuccess { smbTrace("sub-import: launch() returned ok") }
        .onFailure {
            smbTrace("sub-import: launch() threw: ${it.javaClass.simpleName}: ${it.message}")
            showToast("无法打开系统文件选择器：${it.message}")
        }
}

internal fun PlayerComposeActivity.importSubtitleFile(uri: Uri) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法导入字幕")
        return
    }
    Thread {
        val outcome = copyAndAddSubtitle(uri, mpv)
        runOnUiThread {
            if (outcome != null) return@runOnUiThread
            panelRevision++
            showToast("字幕已导入并启用")
        }
    }.start()
}

/**
 * 剪贴板里的字幕链接 → 下载 → 纳入导入库 → 挂载。
 *
 * 【为什么不给输入框】播放页横屏下键盘会挤压画面，而链接通常是刚从网页复制的。
 * 【为什么下载先进 cacheDir】下载完的文件还要经 [SubtitleImportStore] 按内容哈希
 * 收进 `filesDir/subtitles`（与 SAF 导入同一终点、同一去重规则）。
 */
internal fun PlayerComposeActivity.importSubtitleFromClipboard() {
    if (mpvNetSession == null) {
        showToast("内核尚未就绪，暂时无法导入字幕")
        return
    }
    val url = clipboardSubtitleUrl()
    if (url == null) {
        smbTrace("sub-remote: no url in clipboard")
        showToast("剪贴板里没有字幕链接")
        return
    }
    importSubtitleUrl(url)
}

/**
 * URL 导入的**唯一执行入口**（剪贴板与取证探针 `kivo.subUrl` 共用）。
 * 抽出来是为了让"下载 → 导入库 → 挂载"这条链路能在真机上被单点验证 ——
 * 本机 MIUI 的 `cmd clipboard` 不可用，剪贴板没法用 adb 设值。
 */
internal fun PlayerComposeActivity.importSubtitleUrl(url: String) {
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法导入字幕")
        return
    }
    smbTrace("sub-remote: fetching $url")
    Thread {
        val remote = SubtitleRemoteFetcher.fetch(url, File(cacheDir, REMOTE_SUBTITLE_CACHE_DIR))
            .getOrElse { error ->
                val reason = error.message?.takeIf { it.isNotBlank() } ?: "下载失败"
                // 取证：带上异常类名 —— 只靠中文 reason 分不清"网络不可达"和"被拦"。
                smbTrace("sub-remote failed: ${error.javaClass.simpleName}: $reason")
                runOnUiThread { showToast(reason) }
                return@Thread
            }
        val imported = SubtitleImportStore(this) { smbTrace(it) }.importFile(remote.file, remote.title)
        if (imported == null) {
            val reason = "无法保存下载的字幕"
            smbTrace("sub-remote failed: $reason")
            runOnUiThread { showToast("字幕导入失败：$reason") }
            return@Thread
        }
        val failure = attachImported(imported, mpv)
        if (failure != null) {
            smbTrace("sub-remote failed: $failure")
            return@Thread
        }
        runOnUiThread {
            panelRevision++
            showToast("字幕已导入并启用")
        }
        smbTrace("sub-remote ok: ${imported.file.name}")
    }.start()
}

/** @return 剪贴板里第一个 http(s) 链接；空或不是链接返回 null。 */
private fun PlayerComposeActivity.clipboardSubtitleUrl(): String? = runCatching {
    val clipboard = getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
    val clip = clipboard?.primaryClip?.takeIf { it.itemCount > 0 }
    val text = clip?.getItemAt(0)?.coerceToText(this)?.toString()?.trim().orEmpty()
    text.takeIf {
        it.startsWith("http://", ignoreCase = true) || it.startsWith("https://", ignoreCase = true)
    }
}.getOrNull()

/** @return null＝成功；否则是给用户看的失败原因（已 toast，非静默）。 */
private fun PlayerComposeActivity.copyAndAddSubtitle(uri: Uri, mpv: MpvPlaybackSession): String? {
    val store = SubtitleImportStore(this) { smbTrace(it) }
    val imported = store.import(contentResolver, uri)
    if (imported == null) {
        smbTrace("sub-import failed: cannot read $uri")
        runOnUiThread { showToast("字幕导入失败：无法读取所选文件") }
        return "无法读取所选文件"
    }
    return attachImported(imported, mpv)
}

/**
 * @return null＝成功；否则是给用户看的失败原因（已 toast）。
 * SAF / URL / 重编码三条路径共用（重编码后同样要"摘旧轨 → 挂新内容"）。
 */
internal fun PlayerComposeActivity.attachImported(
    imported: SubtitleImportStore.Imported,
    mpv: MpvPlaybackSession,
): String? {
    val store = SubtitleImportStore(this) { smbTrace(it) }
    val catalog = mpv.tracksOrNull()
    // M1：同一文件可能**已被内核加载**（内容去重命中 / 历史状态）。再 `sub-add` 一次
    // 会多出一条指向同一文件的外挂轨 ⇒ 先按 mpv 给的精确路径找已有轨，命中则只选中。
    // 路径两侧都走 subtitlePathKey：mpv 的 /data/data/… 与我们的 /data/user/0/… 是同一文件。
    val importedKey = subtitlePathKey(imported.file.absolutePath)
    val externalTracks = catalog?.subtitleTracks()?.filter { it.external }.orEmpty()
    val alreadyLoaded = externalTracks.firstOrNull {
        it.externalFilename.isNotEmpty() && subtitlePathKey(it.externalFilename) == importedKey
    }
    if (alreadyLoaded != null) {
        // 与 pickMainTrack 同口径：主/次同轨会叠印 —— 选主位前先关次字幕；关不掉就取消。
        val sameAsSecondary = mpv.readProperty("secondary-sid")?.trim()?.toIntOrNull() == alreadyLoaded.id
        if (sameAsSecondary) {
            val native = mpv.nativeOrNull()
            val closed = native != null && SubtitleSecondaryControl.disable(native)
            if (!closed) {
                smbTrace("sub-import reuse cancelled: cannot disable secondary id=${alreadyLoaded.id}")
                runOnUiThread { showToast("未能关闭次字幕，切换已取消") }
                return "未能关闭次字幕"
            }
        }
        val selected = catalog?.selectTrack("sub", alreadyLoaded.id) == true
        smbTrace(
            "sub-import reuse: id=${alreadyLoaded.id} selected=$selected closedSecondary=$sameAsSecondary " +
                "path=${imported.file.name}",
        )
        if (!selected) {
            runOnUiThread { showToast("该字幕已加载，但切换失败（id=${alreadyLoaded.id}）") }
            return "切换失败"
        }
        // 复用也要登记：本片源仍视这份文件为它的导入。
        store.remember(subtitleSourceKey(), imported.file)
        lastSubtitleFileKey = imported.file.nameWithoutExtension
        return null
    }
    if (externalTracks.isNotEmpty() && externalTracks.all { it.externalFilename.isEmpty() }) {
        // 内核不提供 external-filename ⇒ 无法确认是否同一文件；**不按标题猜**，按原流程挂载。
        smbTrace("sub-import dedupe unavailable: external-filename empty (tracks=${externalTracks.size})")
    }
    val added = catalog?.addExternalSubtitle(imported.file.absolutePath, imported.title) ?: false
    if (!added) {
        // 新拷贝且未被登记 ⇒ 不留垃圾文件；去重命中的文件要保留（可能别的片源在用）。
        if (!imported.deduped) store.forget(subtitleSourceKey(), imported.file)
        smbTrace(
            "sub-import add failed: ${imported.file.name} catalog=${catalog != null} " +
                "err=${mpv.lastError} native=${com.kivo.cinema.mpv.MpvNative.lastCommandError}",
        )
        runOnUiThread { showToast("字幕加载失败：${mpv.lastError.ifBlank { imported.title }}") }
        return "加载失败"
    }
    store.remember(subtitleSourceKey(), imported.file)
    lastSubtitleFileKey = imported.file.nameWithoutExtension
    smbTrace(
        "sub-import ok: ${imported.file.absolutePath} deduped=${imported.deduped} " +
            "size=${imported.file.length()}",
    )
    return null
}

/**
 * 本片源的字幕键（与观看记录同一口径）。
 *
 * 本地测试片源没有 `kivo.media` 时回落到当前媒体路径；两者都没有 ⇒ 空串，
 * 此时不登记文件归属（删除时就不会误删别人）。
 */
internal fun PlayerComposeActivity.subtitleSourceKey(): String =
    SubtitleSourceKey.fromIntent(intent)
        ?: currentMediaPath().takeIf { it.isNotBlank() && mediaOverridePath.isNotBlank() }
        ?: ""