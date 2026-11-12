package com.kivo.cinema.ui.compose

/**
 * 会话收尾（243/253）。
 *
 * 【为什么单独一个文件】收尾顺序是**正确性约束**而不是风格问题：
 * 先停会话（它借用的 fd 还活着），再关 RangeFd（fd 的所有者）。
 * 顺序反了就是"读到已关闭的描述符"。把它与起播/轮询分开，
 * 改起播的人不会顺手动到这条顺序。
 */

internal fun PlayerComposeActivity.closeNetworkPlayback() {
    ++networkGeneration
    networkPreparationCancelled?.set(true)
    networkPreparationCancelled = null
    val network = smbNetSession
    smbNetSession = null
    if (network != null) {
        closingNetworks += network
        network.stop {
            android.os.Handler(mainLooper).post { closingNetworks -= network }
        }
    }
    // libmpv 内核与旧内核互斥，但收尾路径要一并处理。
    val mpv = mpvNetSession
    mpvNetSession = null
    mpv?.stop()
    // 软解 close() 会 join 解码线程（内部可能有阻塞读），不能放主线程。
    val soft = smbSoftSession
    smbSoftSession = null
    val softAudio = smbSoftAudio
    smbSoftAudio = null
    val softAudioPfd = smbSoftAudioPfd
    smbSoftAudioPfd = null
    smbSoftAudioGated = false
    val softRange = smbSoftRange
    smbSoftRange = null
    val softAudioRange = smbSoftAudioRange
    smbSoftAudioRange = null
    val softAudioSource = smbSoftAudioSource
    smbSoftAudioSource = null
    val thumbSource = netThumbSource
    netThumbSource = null
    softRateMarkMs = 0L
    softRateMarkPosUs = -1L
    softAudioMarkUs = -1L
    if (soft != null || softRange != null || softAudio != null ||
        softAudioRange != null || softAudioSource != null || thumbSource != null) {
        Thread({
            runCatching { softAudio?.close() }
            runCatching { soft?.close() }
            runCatching { softAudioPfd?.close() }
            runCatching { softAudioRange?.close() }
            runCatching { softAudioSource?.close() }
            runCatching { softRange?.close() }
            runCatching { thumbSource?.close() }
        }, "KivoSmbSoftClose").start()
    }
}