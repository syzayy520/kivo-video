package com.kivo.cinema.ui.compose

import android.media.AudioManager
import android.provider.Settings
import com.kivo.cinema.ui.compose.player.PlayerHudKind
import com.kivo.cinema.ui.compose.player.PlayerMotion
import com.kivo.cinema.ui.design.AppearancePrefs
import kotlin.math.roundToInt

internal fun PlayerComposeActivity.onVerticalDragStart(x: Float) {
    cancelHudHide()
    hudKind = null
    hudDragging = false
    if (!playbackPrefs.verticalSwipeEnabled) return
    val width = window.decorView.width.toFloat().coerceAtLeast(1f)
    val kind = if (x < width / 2f) PlayerHudKind.BRIGHTNESS else PlayerHudKind.VOLUME
    if (kind == PlayerHudKind.BRIGHTNESS && !playbackPrefs.independentBrightness) return
    val current = runCatching { currentHudFraction(kind) }.getOrElse {
        showToast("无法读取${if (kind == PlayerHudKind.VOLUME) "音量" else "亮度"}：${it.message ?: "系统未提供数值"}")
        return
    }
    hudFraction = current
    hudOriginFraction = current
    hudRequestedFraction = current
    hudKind = kind
    hudDragging = true
    lastTouchMs = System.currentTimeMillis()
}

internal fun PlayerComposeActivity.onVerticalDrag(dragAmount: Float) {
    if (!hudDragging) return
    val kind = hudKind ?: return
    val height = window.decorView.height.toFloat().coerceAtLeast(1f)
    // 手指连续量独立于系统离散音量档位，微小位移不会被读回值吞掉。
    hudRequestedFraction = (hudRequestedFraction - dragAmount / (height * .55f)).coerceIn(0f, 1f)
    runCatching {
        applyHudValue(kind, hudRequestedFraction)
        hudFraction = currentHudFraction(kind)
    }.onFailure {
        hudDragging = false
        showToast("无法调整${if (kind == PlayerHudKind.VOLUME) "音量" else "亮度"}：${it.message ?: "系统拒绝操作"}")
        scheduleHudHide()
    }
    lastTouchMs = System.currentTimeMillis()
}

internal fun PlayerComposeActivity.onVerticalDragEnd() {
    hudDragging = false
    scheduleHudHide()
}

internal fun PlayerComposeActivity.cancelLevelGesture() {
    cancelHudHide()
    hudDragging = false
    hudKind = null
}

private fun PlayerComposeActivity.scheduleHudHide() {
    cancelHudHide()
    val token = Runnable { hudKind = null; hudHideToken = null }
    hudHideToken = token
    window.decorView.postDelayed(token, PlayerMotion.hudHoldMs.toLong())
}

private fun PlayerComposeActivity.cancelHudHide() {
    hudHideToken?.let { window.decorView.removeCallbacks(it) }
    hudHideToken = null
}

internal fun PlayerComposeActivity.haptic(constant: Int) {
    if (AppearancePrefs.hapticAllowed(this)) window.decorView.performHapticFeedback(constant)
}

internal fun PlayerComposeActivity.currentHudFraction(kind: PlayerHudKind): Float = when (kind) {
    PlayerHudKind.BRIGHTNESS -> currentWindowBrightness()
    PlayerHudKind.VOLUME -> currentMediaVolume()
}

internal fun PlayerComposeActivity.applyHudValue(kind: PlayerHudKind, value: Float) {
    when (kind) {
        PlayerHudKind.BRIGHTNESS -> {
            val params = window.attributes
            params.screenBrightness = value.coerceIn(0f, 1f)
            window.attributes = params
        }
        PlayerHudKind.VOLUME -> {
            val manager = audioManager()
            val max = manager.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
            check(max > 0) { "媒体音量档位不可用" }
            manager.setStreamVolume(AudioManager.STREAM_MUSIC, (value * max).roundToInt().coerceIn(0, max), 0)
        }
    }
}

internal fun PlayerComposeActivity.currentWindowBrightness(): Float {
    val value = window.attributes.screenBrightness
    if (value >= 0f) return value.coerceIn(0f, 1f)
    return (Settings.System.getInt(contentResolver, Settings.System.SCREEN_BRIGHTNESS) / 255f).coerceIn(0f, 1f)
}

internal fun PlayerComposeActivity.currentMediaVolume(): Float {
    val manager = audioManager()
    val max = manager.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
    check(max > 0) { "媒体音量档位不可用" }
    return manager.getStreamVolume(AudioManager.STREAM_MUSIC).toFloat() / max
}

internal fun PlayerComposeActivity.audioManager(): AudioManager =
    getSystemService(android.content.Context.AUDIO_SERVICE) as AudioManager
