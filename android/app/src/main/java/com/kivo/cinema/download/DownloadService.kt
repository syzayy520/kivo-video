package com.kivo.cinema.download

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.IBinder
import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import java.io.File
import java.net.HttpURLConnection
import java.net.URL

/**
 * 下载前台服务（225 设计稿 D2）：OkHttp 下载 + 进度通知。
 *
 * SenPlayer 同款 DownloadService 对位：前台服务 + 进度通知 + 完成跳转。
 * 下载到 getExternalFilesDir("downloads")/ → 媒体库自动可见（同目录扫描）。
 */
class DownloadService : Service() {

    private val jobs = mutableMapOf<String, Job>()
    private lateinit var notifyManager: NotificationManager

    companion object {
        const val TAG = "KivoDownload"
        const val CHANNEL_ID = "download"
        const val NOTIFICATION_ID_BASE = 1000
        const val EXTRA_URL = "download.url"
        const val EXTRA_TITLE = "download.title"

        /**
         * 下载落点（AC3）：公共 Movies/Kivo/。
         * getExternalFilesDir 在 Android 10+ 不被 MediaStore 索引，媒体库查不到；
         * 公共媒体集合里应用可直接用 File API 新建媒体文件（Android 11+ 无需权限），
         * 落盘后 scanFile 入库 → 222 媒体库查询可见。
         */
        fun downloadsDir(activity: android.content.Context): java.io.File =
            java.io.File(
                android.os.Environment.getExternalStoragePublicDirectory(
                    android.os.Environment.DIRECTORY_MOVIES),
                "Kivo"
            ).apply { mkdirs() }
    }

    override fun onCreate() {
        super.onCreate()
        notifyManager = getSystemService(NotificationManager::class.java)
        val channel = NotificationChannel(CHANNEL_ID, "下载", NotificationManager.IMPORTANCE_LOW)
        notifyManager.createNotificationChannel(channel)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val url = intent?.getStringExtra(EXTRA_URL) ?: return START_NOT_STICKY
        val title = intent.getStringExtra(EXTRA_TITLE) ?: url.substringAfterLast('/')
        val notificationId = NOTIFICATION_ID_BASE + url.hashCode()

        // 前台通知
        startForeground(notificationId, buildNotification(title, 0, "准备下载…"))

        val job = CoroutineScope(Dispatchers.IO).launch {
            try {
                downloadFile(url, title, notificationId)
            } catch (e: Exception) {
                Log.e(TAG, "download failed: $url", e)
                updateNotification(notificationId, title, 0, "下载失败：${e.message}")
                stopSelfWhenIdle()
            }
        }
        jobs[url] = job
        return START_NOT_STICKY
    }

    private fun downloadFile(url: String, title: String, notificationId: Int) {
        val dir = downloadsDir(this)
        val localFile = File(dir, title)
        Log.i(TAG, "download target: ${localFile.absolutePath}")
        val existingBytes = if (localFile.exists()) localFile.length() else 0

        val conn = URL(url).openConnection() as HttpURLConnection
        conn.connectTimeout = 10000
        conn.readTimeout = 30000
        if (existingBytes > 0) {
            conn.setRequestProperty("Range", "bytes=$existingBytes-")
        }

        val responseCode = conn.responseCode
        val isResume = responseCode == 206 && existingBytes > 0
        val totalBytes = if (responseCode == 206) {
            val range = conn.getHeaderField("Content-Range") ?: ""
            Regex("bytes \\d+-\\d+/(\\d+)").find(range)?.groupValues?.get(1)?.toLong() ?: -1
        } else {
            conn.contentLengthLong
        }

        Log.i(TAG, "download start: total=$totalBytes resume=$isResume offset=$existingBytes")

        conn.inputStream.use { input ->
            val output = if (isResume) {
                java.io.RandomAccessFile(localFile, "rw").apply { seek(existingBytes) }
            } else {
                localFile.writeBytes(ByteArray(0))
                java.io.RandomAccessFile(localFile, "rw")
            }
            output.use { raf ->
                val buf = ByteArray(64 * 1024)
                var downloaded = existingBytes
                var lastNotify = 0L
                while (running_checks() && remaining(totalBytes, downloaded) > 0) {
                    val n = input.read(buf, 0, minOf(buf.size.toLong(), remaining(totalBytes, downloaded)).toInt())
                    if (n < 0) break
                    raf.write(buf, 0, n)
                    downloaded += n
                    val now = System.currentTimeMillis()
                    if (now - lastNotify > 500) {
                        lastNotify = now
                        val pct = if (totalBytes > 0) (downloaded * 100 / totalBytes).toInt() else 0
                        updateNotification(notificationId, title, pct,
                            "已下载 ${downloaded / (1024 * 1024)}MB")
                        Log.d(TAG, "download progress: $downloaded/$totalBytes")
                    }
                }
            }
        }
        conn.disconnect()

        val finalSize = localFile.length()
        Log.i(TAG, "download done: ${localFile.absolutePath} ($finalSize bytes)")
        updateNotification(notificationId, title, 100, "下载完成 ✓")
        // AC3：落到 Movies/Kivo/ 后触发扫描 → MediaStore 索引 → 222 媒体库可见
        android.media.MediaScannerConnection.scanFile(
            this, arrayOf(localFile.absolutePath), arrayOf("video/mp4"), null)
        stopSelfWhenIdle()
    }

    private fun running_checks(): Boolean = true
    private fun remaining(total: Long, downloaded: Long): Long =
        if (total > 0) total - downloaded else Long.MAX_VALUE

    private fun updateNotification(notificationId: Int, title: String, progress: Int, text: String) {
        notifyManager.notify(notificationId, buildNotification(title, progress, text))
    }

    private fun buildNotification(title: String, progress: Int, text: String): Notification {
        val builder = Notification.Builder(this, CHANNEL_ID)
            .setContentTitle(title)
            .setContentText(text)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setOngoing(true)
        if (progress in 1..99) {
            builder.setProgress(100, progress, false)
        }
        return builder.build()
    }

    private fun stopSelfWhenIdle() {
        if (jobs.values.all { !it.isActive }) {
            stopSelf()
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null
}
