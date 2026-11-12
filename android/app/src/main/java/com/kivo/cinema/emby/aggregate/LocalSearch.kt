package com.kivo.cinema.emby.aggregate

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.provider.MediaStore
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.privatezone.PrivateStore
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext

/**
 * 256 全局搜索本地分区：MediaStore 标题匹配 + 私密区排除（231 红线）+ 授权态非空 reason。
 * 单一职责：本地匹配数据；UI 在 LocalResultCard，编排在 AggregateSearchScreen。
 */
object LocalSearch {
    data class State(val denied: String? = null, val items: List<MediaItem> = emptyList())

    fun match(context: Context, query: String): State {
        val perm = if (Build.VERSION.SDK_INT >= 33) Manifest.permission.READ_MEDIA_VIDEO
                   else Manifest.permission.READ_EXTERNAL_STORAGE
        if (context.checkSelfPermission(perm) != PackageManager.PERMISSION_GRANTED)
            return State(denied = "本地未授权视频访问，请在系统设置开启后重试")
        val hidden = PrivateStore(context).all()
        val out = mutableListOf<MediaItem>()
        runCatching {
            context.contentResolver.query(
                MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                arrayOf(MediaStore.Video.Media._ID, MediaStore.Video.Media.DISPLAY_NAME,
                    MediaStore.Video.Media.DATA, MediaStore.Video.Media.DURATION,
                    MediaStore.Video.Media.WIDTH, MediaStore.Video.Media.HEIGHT,
                    MediaStore.Video.Media.DATE_ADDED),
                "${MediaStore.Video.Media.DISPLAY_NAME} LIKE ?", arrayOf("%$query%"),
                "${MediaStore.Video.Media.DATE_ADDED} DESC")?.use { c ->
                while (c.moveToNext() && out.size < 50) {
                    val id = c.getLong(0)
                    if (hidden.contains(id.toString())) continue
                    out.add(MediaItem(id, c.getString(1) ?: "", c.getString(2) ?: "",
                        c.getLong(3), c.getInt(4), c.getInt(5), c.getLong(6)))
                }
            }
        }
        return State(items = out)
    }
}

@Composable
fun rememberLocalMatches(query: String): LocalSearch.State {
    val context = LocalContext.current
    var state by remember { mutableStateOf(LocalSearch.State()) }
    LaunchedEffect(query) {
        if (query.isBlank()) { state = LocalSearch.State(); return@LaunchedEffect }
        delay(200)
        state = withContext(Dispatchers.IO) { LocalSearch.match(context, query.trim()) }
    }
    return state
}