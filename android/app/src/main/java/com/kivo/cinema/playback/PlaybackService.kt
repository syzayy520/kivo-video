package com.kivo.cinema.playback

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.media.MediaMetadata
import android.media.session.MediaSession
import android.media.session.PlaybackState
import android.os.IBinder
import android.util.Log
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.ui.compose.PlayerComposeActivity

/**
 * 后台播放 Foreground Service（214）。
 *
 * 持有 [PlaybackSession] 的所有权：Activity 起播后把会话移交给本服务，
 * Activity 销毁后会话不死。回前台时 Activity 从服务取回会话引用。
 *
 * 线程模型：`positionPoll` 在主线程跑（100ms 轮询 position + 更新通知 +
 * 更新 MediaSession PlaybackState）。通知更新用 5s 节流（100ms 刷通知太频）。
 *
 * 生命周期：`START_NOT_STICKY`——系统杀掉后不自动重启，用户手动重开。
 */
class PlaybackService : Service() {

    private var session: PlaybackSession? = null
    private var mediaSession: MediaSession? = null
    private var isPlaying = false
    private var lastNotificationUpdateMs = 0L
    private val handler = android.os.Handler(android.os.Looper.getMainLooper())

    private val positionPoll = object : Runnable {
        override fun run() {
            val s = session ?: return
            val positionUs = s.positionUs()
            updateMediaSessionState(positionUs)
            updateNotification(positionUs)
            if (isPlaying) {
                handler.postDelayed(this, 100)
            }
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        createNotificationChannel()
        // 从 Activity 接收会话（companion 静态引用——M1 最简）
        val transferred = PlaybackSessionHolder.consume()
        if (transferred != null) {
            session = transferred
            setupMediaSession(transferred)
            startForeground(NOTIFICATION_ID, buildNotification(transferred, 0))
            isPlaying = true
            handler.post(positionPoll)
            Log.i(TAG, "service started with session")
        } else {
            Log.w(TAG, "no session to adopt — stopping")
            stopSelf()
        }
        return START_NOT_STICKY
    }

    override fun onDestroy() {
        handler.removeCallbacksAndMessages(null)
        mediaSession?.release()
        mediaSession = null
        session?.close()
        session = null
        Log.i(TAG, "service destroyed, session closed")
        super.onDestroy()
    }

    private fun setupMediaSession(session: PlaybackSession) {
        mediaSession = MediaSession(this, "KivoPlayback").apply {
            setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS or
                     MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS)
            setCallback(object : MediaSession.Callback() {
                override fun onPlay() {
                    session.resume()
                    isPlaying = true
                    handler.post(positionPoll)
                }
                override fun onPause() {
                    session.pause()
                    isPlaying = false
                }
                override fun onSeekTo(pos: Long) {
                    session.seekTo(pos)
                }
            })
            setActive(true)
        }
    }

    private fun updateMediaSessionState(positionUs: Long) {
        mediaSession?.setPlaybackState(
            PlaybackState.Builder()
                .setActions(PlaybackState.ACTION_PLAY or
                            PlaybackState.ACTION_PAUSE or
                            PlaybackState.ACTION_SEEK_TO)
                .setState(if (isPlaying) PlaybackState.STATE_PLAYING else PlaybackState.STATE_PAUSED,
                          positionUs, 1.0f)
                .build()
        )
    }

    private fun updateNotification(positionUs: Long) {
        val now = System.currentTimeMillis()
        if (now - lastNotificationUpdateMs < 5000) return  // 5s 节流
        lastNotificationUpdateMs = now
        val manager = getSystemService(NotificationManager::class.java)
        manager?.notify(NOTIFICATION_ID, buildNotification(session ?: return, positionUs))
    }

    private fun buildNotification(session: PlaybackSession, positionUs: Long): Notification {
        val openIntent = PendingIntent.getActivity(
            this, 0,
            Intent(this, PlayerComposeActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        return Notification.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_media_play)
            .setContentTitle(session.summary().substringBefore(" mime="))
            .setContentIntent(openIntent)
            .setOnlyAlertOnce(true)
            .setOngoing(true)
            .build()
    }

    private fun createNotificationChannel() {
        val channel = NotificationChannel(
            CHANNEL_ID, "播放", NotificationManager.IMPORTANCE_LOW
        )
        getSystemService(NotificationManager::class.java)?.createNotificationChannel(channel)
    }

    companion object {
        private const val TAG = "KivoPlayback"
        private const val CHANNEL_ID = "playback"
        private const val NOTIFICATION_ID = 42

        /** 会话转移通道：Activity 起播后把会话交给这里，Service 在 onStartCommand 取走。 */
        object Holder {
            @Volatile var pending: PlaybackSession? = null
            fun offer(session: PlaybackSession) { pending = session }
            fun consume(): PlaybackSession? { val s = pending; pending = null; return s }
        }
    }
}

/// 顶层别名（避免 PlayComposeActivity import 混淆）。
object PlaybackSessionHolder {
    @Volatile var pending: PlaybackSession? = null
    fun offer(session: PlaybackSession) { pending = session }
    fun consume(): PlaybackSession? { val s = pending; pending = null; return s }
}
