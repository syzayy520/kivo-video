package com.kivo.cinema.ui.compose

import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.HapticFeedbackConstants
import com.kivo.cinema.playback.MpvPlaybackSession
import java.math.BigDecimal
import java.math.RoundingMode

/** Activity/mpv 适配层；380ms 识别由检测器负责，这里没有定时器或 native 主线程读写。 */
internal fun PlayerComposeActivity.onLongPressStart(x: Float, y: Float) {
    if (!playbackPrefs.longPressEnabled) return
    if (screenLocked && !playbackPrefs.lockAllowGestures) return
    val metrics = resources.displayMetrics
    if (y > metrics.heightPixels - metrics.density * 96f) return
    speedGesture.start(x, y, playbackPrefs.longPressBaseSpeed)
}

internal fun PlayerComposeActivity.onLongPressDrag(x: Float, y: Float) {
    speedGesture.drag(
        x, y,
        allowSpeed = !screenLocked && playbackPrefs.longPressSwipeSpeed,
        allowLock = !screenLocked && playbackPrefs.longPressSwipeLock,
        lockPx = PlayerComposeActivity.LOCK_UP_THRESHOLD_DP * resources.displayMetrics.density,
    )
}

internal fun PlayerComposeActivity.onLongPressEnd() {
    speedGesture.end(keep = playbackPrefs.longPressRestore == "keep")
}

/** 指针取消、失焦、后台都走强制恢复；不服从 keep，也不能遗留锁定。 */
internal fun PlayerComposeActivity.onLongPressCancel() { speedGesture.cancel() }
internal fun PlayerComposeActivity.unlockLongPress() { speedGesture.unlock() }
internal fun PlayerComposeActivity.applySpeed(value: Double) { speedGesture.applySpeed(value) }
internal fun PlayerComposeActivity.formatSpeed(value: Double): String = speedGestureLabel(value)

/** 不受 Locale 影响，不把二进制浮点尾数暴露给用户。 */
internal fun speedGestureLabel(value: Double): String = if (!value.isFinite() || value <= 0) "未知" else
    BigDecimal.valueOf(value).setScale(2, RoundingMode.HALF_UP).stripTrailingZeros().toPlainString() + "x"

/** 与纯逻辑构造器同名的 Activity 工厂：Activity 只需持有一个 lazy owner。 */
internal fun PlayerSpeedGestureController(activity: PlayerComposeActivity): PlayerSpeedGestureController {
    val main = Handler(Looper.getMainLooper())
    return PlayerSpeedGestureController(object : PlayerSpeedGestureController.Host {
        override fun capture(): PlayerSpeedGestureController.Port? = activity.mpvNetSession?.let(::MpvSpeedPort)
        override fun currentIdentity(): Any? = activity.mpvNetSession
        override fun post(action: () -> Unit) { main.post { action() } }
        override fun render(active: Boolean, locked: Boolean, speed: Double?) {
            if (activity.isDestroyed) return
            speed?.let { activity.longPressSpeed = it }
            activity.longPressActive = active
            activity.longPressLocked = locked
        }
        override fun feedback(started: Boolean, locked: Boolean) {
            if (activity.isDestroyed) return
            if (started) activity.haptic(HapticFeedbackConstants.LONG_PRESS)
            if (locked) {
                activity.haptic(HapticFeedbackConstants.VIRTUAL_KEY)
                activity.showToast("已锁定快进 ${speedGestureLabel(activity.longPressSpeed)}，点按画面解除")
            }
        }
        override fun report(identity: Any?, reason: String) {
            Log.w("KivoSpeedGesture", reason)
            // 旧会话失败仍留明确日志，但不把旧错误显示到新影片上。
            if (!activity.isDestroyed && (identity == null || identity === activity.mpvNetSession)) {
                activity.showToast(reason)
            }
        }
        override fun unlocked() {
            if (!activity.isDestroyed) activity.showToast("已解除快进锁定")
        }
    })
}

/** 永远捕获具体会话，不在后台读取 Activity.mpvNetSession 再写（可能已换集）。 */
private class MpvSpeedPort(private val session: MpvPlaybackSession) : PlayerSpeedGestureController.Port {
    override val identity: Any get() = session
    override val closed: Boolean get() = session.state == MpvPlaybackSession.State.CLOSED
    override fun read(): Double? = session.currentSpeedOrNull()
    override fun write(value: Double): Boolean = session.setSpeed(value)
    override fun failure(): String = session.lastError
}
