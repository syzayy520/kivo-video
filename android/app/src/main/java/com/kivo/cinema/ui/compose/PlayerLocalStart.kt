package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import com.kivo.cinema.net.FileByteSource
import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.playback.PlaybackStack

/**
 * 280 · **本地文件的起播分流**（本地文件此前一直走自研旧内核）。
 *
 * ## 为什么要这个文件
 *
 * 280 审查发现：Emby/URL 走 mpv（`PlayerUrlStart`）、SMB 走 mpv（`PlayerSmbStart`），
 * 而**本地文件走的是自研旧内核**（`PlayerSessionStart` 里直接 `PlaybackSession(...)`）。
 * 五个入口（MainActivity / MediaLibraryActivity / LocalResultCard / DetailContent /
 * PrivateActivity）全都带 `kivo.media` ⇒ 汇到同一条旧链路。
 *
 * 后果：本地播放没有 gpu-next、没有完整 libplacebo（HDR 峰值计算）、
 * 没有 aimagereader 零拷贝、没有内核侧多轨控制面
 * ⇒ "对齐并超过 SenPlayer"在**本地场景一条都没落地**。
 *
 * ## 怎么分
 *
 * 按 [PlaybackStack] 的决策走（不是另起一套判断）：
 *
 * | 条件 | 走哪条 | 理由 |
 * | --- | --- | --- |
 * | `debug.kivo.engine=legacy` | 自研旧内核 | 取证/紧急回退 |
 * | 开了「后台继续播放音频」 | 自研旧内核 | **功能对等**：mpv 路径尚未接入后台服务，迁过去会丢这个能力 |
 * | 其余（默认） | **mpv（gpu-next）** | 目标形态 |
 *
 * 第三行是默认路径 ⇒ 绝大多数用户第一次就吃到新内核。
 *
 * ## 为什么"后台播放"要回退而不是硬迁
 *
 * 旧本地链路会 `startForegroundService` + `PlaybackSessionHolder.offer`，
 * 会话在 Activity 销毁后仍活着（这是「后台继续播放音频」的实现）。
 * mpv 路径没有这一层：`onDestroy → closeNetworkPlayback()` 直接把会话关掉。
 * ⇒ 硬迁等于悄悄删掉一个已有功能。所以按开关分流，并在日志里写明原因
 * （**不许静默降级**）。真正的解法是给 mpv 会话加 Service 所有权，列为后续任务。
 *
 * ## 与缩略图的关系
 *
 * 本地片源有完整索引 ⇒ 仍用 [com.kivo.cinema.thumbnail.ThumbnailReader]（不走网络预览内核）。
 * 这里不碰缩略图：它由 `maybeStartPlayback` 的后续流程按需挂。
 */
internal fun PlayerComposeActivity.startLocalPlayback(
    holder: SurfaceHolder,
    media: java.io.File,
): Boolean {
    val stack = PlaybackStack.resolve(this, intent)

    // 1) 内核选择器要求 legacy（取证 / 紧急回退）。
    if (!stack.usesMpv) {
        NetDiag.log("[local] 内核=legacy（debug.kivo.engine 指定）⇒ 走自研链路")
        return false
    }

    // 2) 功能对等闸门：开了后台播放就走旧链路（见文件头说明）。
    if (playbackPrefs.backgroundAudio) {
        NetDiag.log(
            "[local] 开了「后台继续播放音频」⇒ 走自研链路（mpv 路径尚未接入后台服务）"
        )
        return false
    }

    // 3) 默认：走 mpv。本地文件也通过 [FileByteSource] 喂给本地代理，
    //    与 SMB/HTTP 完全同一条链路（只是读源换成本地文件）。
    NetDiag.log("[local] ${stack.describe()} ⇒ 走 mpv 内核（本地文件）")

    // 弹幕要在起播前挂（旧本地路径同样在起播前调）。
    loadDanmaku(media.absolutePath)
    // 本地片源能直接探时长，不必等内核报（进度条从一开始就是对的）。
    durationMs = probeDurationUs(media.absolutePath) / 1000

    startMpvPlayback(holder, FileByteSource(media.absolutePath), isLocal = true)
    return true
}
