package com.kivo.cinema.ui.compose.player

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.ui.compose.readProperty
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 面板条目的**取数时机**（v3 §五.1 的结构性防再犯）。
 *
 * 【round-4 为什么打开必空】旧实现把条目放在 Activity 的缓存字段里，
 * 而缓存只在 `onPanelSelect` 的回调里刷新 —— 也就是"选完之后"才有内容，
 * 第一次打开永远是空列表，看起来像"没有音轨选项"。
 *
 * 【v3 怎么修】取数挂在 `LaunchedEffect(kind, session)` 上：
 * 面板一打开就算，会话一就绪再算。没有节流缓存，没有"上次刷新时间"。
 *
 * 【为什么必须切 IO】`MpvTrackCatalog.tracks()` 每条轨道读 9 个 mpv 属性，
 * 11 条轨道 = 99 次 JNI 读取，每次都要抢 mpv 核心锁。
 * 在主线程做 ⇒ 点开面板卡好几秒（用户实测）。
 */
@Composable
internal fun rememberPanelItems(
    kind: String,
    session: MpvPlaybackSession?,
): List<PlayerOptionItem> {
    var items by remember { mutableStateOf<List<PlayerOptionItem>>(emptyList()) }
    LaunchedEffect(kind, session) {
        items = if (kind.isBlank()) emptyList()
        else withContext(Dispatchers.IO) { enumeratePanel(kind, session) }
    }
    return items
}

/** 面板标题（唯一来源，避免壳组件各自写一份中文名）。 */
internal fun panelTitle(kind: String): String = when (kind) {
    "speed" -> "倍速"
    "quality" -> "画质比例"
    "audio" -> "音轨"
    "subtitle" -> "字幕"
    "episodes" -> "选集"
    "equalizer" -> "均衡器"
    "resource" -> "资源"
    else -> ""
}

/** 真正的枚举 —— 只在 IO 线程跑。 */
private fun enumeratePanel(kind: String, mpv: MpvPlaybackSession?): List<PlayerOptionItem> {
    // 内核未就绪时**任何**档位都不能预选中：那等于告诉用户"当前就是 1x / 自动"。
    if (mpv == null) return if (kind in KERNEL_KINDS) listOf(notReady()) else emptyList()
    val data = PlayerPanelData
    return when (kind) {
        "speed" -> data.speed(data.currentSpeedLabel(mpv))
        "quality" -> data.qualities(mpv.readProperty("video-aspect-override").orEmpty())
        "equalizer" -> data.equalizer(mpv.readProperty("af").orEmpty())
        "audio", "subtitle" -> {
            val catalog = mpv.tracksOrNull() ?: return listOf(notReady())
            if (kind == "audio") data.audioTracks(catalog.audioTracks(), catalog.currentTrackId("audio"))
            else data.subtitleTracks(catalog.subtitleTracks(), catalog.currentTrackId("sub"))
        }
        else -> emptyList()
    }
}

private val KERNEL_KINDS = setOf("speed", "quality", "equalizer", "audio", "subtitle")

/**
 * 内核未就绪时的**非空 reason 行**。
 *
 * 铁律：不许静默降级。空列表会被用户读成"这个片源没有音轨"，
 * 而真相是"还没起播" —— 两者必须分得开。
 */
private fun notReady(): PlayerOptionItem = PlayerOptionItem(
    title = "内核尚未就绪",
    subtitle = "起播后即可切换",
)