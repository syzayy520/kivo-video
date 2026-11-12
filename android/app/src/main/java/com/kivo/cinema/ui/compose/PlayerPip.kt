package com.kivo.cinema.ui.compose

import android.app.PictureInPictureParams
import android.graphics.Rect
import android.os.Build
import android.util.Rational

/**
 * 画中画（v3.1 #10）：参数 / 进入 / 退出态，单一职责。
 *
 * 【为什么之前永远失败】manifest 没声明 `supportsPictureInPicture`，
 * 且进 PiP 传的是空参数 ⇒ 系统一律拒绝，toast 却说"设备不支持"——
 * 其实是我们自己没声明。K40（API33）硬件能力没有问题。
 *
 * 【自动进入】API31+ `setAutoEnterEnabled(true)`：播放中按 Home 自动进 PiP。
 * 参数必须在离场**之前**设好 ⇒ 三条起播链路 ready 后各调一次 [updatePipParams]。
 *
 * 【小窗里画什么】只留视频：chrome/手势层全收（[PlayerOverlay] 按 pipActive 门控），
 * 小窗点击交给系统 PiP 控件（播放/暂停/关闭）。会话由 Foreground Service 托管（214），
 * PiP 期间播放不死。
 */

/** 宽高比取内核真值（video-params/w/h）；读不到退 16:9；夹在系统允许区间。 */
internal fun PlayerComposeActivity.pipAspectRatio(): Rational {
    val native = mpvNetSession?.nativeOrNull()
    val w = runCatching { native?.property("video-params/w")?.toIntOrNull() }.getOrNull() ?: 0
    val h = runCatching { native?.property("video-params/h")?.toIntOrNull() }.getOrNull() ?: 0
    val ratio = if (w > 0 && h > 0) Rational(w, h) else Rational(16, 9)
    return when {
        ratio.toFloat() < 100f / 239f -> Rational(100, 239)
        ratio.toFloat() > 239f / 100f -> Rational(239, 100)
        else -> ratio
    }
}

internal fun PlayerComposeActivity.buildPipParams(): PictureInPictureParams {
    val decor = window.decorView
    val builder = PictureInPictureParams.Builder()
        .setAspectRatio(pipAspectRatio())
        .setSourceRectHint(Rect(0, 0, decor.width, decor.height))
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        // 自动进小窗由设置页"自动开启画中画"决定（262）：关掉则按 Home 不自动进。
        builder.setAutoEnterEnabled(playbackPrefs.autoPip)
    }
    return builder.build()
}

/** 起播就绪后调一次：自动进入依赖参数提前设好。 */
internal fun PlayerComposeActivity.updatePipParams() {
    runCatching { setPictureInPictureParams(buildPipParams()) }
}

/**
 * 进入 PiP。系统仍可能拒绝（任务不在最近任务 / 用户在系统设置里关了 PiP）。
 * 静默失败 = 用户以为图标坏了 ⇒ 必须给可见反馈。
 * 注意：带参的 enterPictureInPictureMode 返回 void，成败只能看是否抛异常；
 * **不抛异常也可能不进**（真机实测 MIUI 静默拒绝）⇒ 1.5s 看门狗核对
 * onPictureInPictureModeChanged 是否真的来过，没来就 toast 原因。
 */
internal fun PlayerComposeActivity.enterPictureInPicture() {
    if (!packageManager.hasSystemFeature(android.content.pm.PackageManager.FEATURE_PICTURE_IN_PICTURE)) {
        showToast("此设备不支持画中画")
        return
    }
    pipEnterConfirmed = false
    val ok = runCatching { enterPictureInPictureMode(buildPipParams()) }.isSuccess
    smbTrace("pip enter requested ok=$ok")
    if (!ok) {
        showToast("无法进入画中画：系统拒绝（请在最近任务里确认允许画中画）")
        return
    }
    android.os.Handler(mainLooper).postDelayed({
        if (!pipEnterConfirmed && !isFinishing && !isDestroyed) {
            smbTrace("pip watchdog: mode-changed never arrived")
            showToast("系统未进入画中画：请在系统设置里允许本应用使用画中画")
        }
    }, 1500)
}

/** 进小窗：收掉一切 chrome；出小窗：恢复控制条并刷新自隐计时。 */
internal fun PlayerComposeActivity.onPipModeChanged(inPip: Boolean) {
    pipActive = inPip
    if (inPip) {
        pipEnterConfirmed = true
        smbTrace("pip mode-changed in=true")
        activePanel = ""
        castSheetVisible = false
        controlsVisible = false
    } else {
        controlsVisible = true
        lastTouchMs = System.currentTimeMillis()
    }
}