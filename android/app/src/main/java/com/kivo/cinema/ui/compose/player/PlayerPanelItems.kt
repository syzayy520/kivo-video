package com.kivo.cinema.ui.compose.player

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
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
    danmakuOn: Boolean,
    danmakuSettings: com.kivo.cinema.danmaku.DanmakuSettings,
    origin: PlayerSourceOrigin,
    revision: Int,
): List<PlayerOptionItem> {
    val context = LocalContext.current
    var items by remember { mutableStateOf<List<PlayerOptionItem>>(emptyList()) }
    // revision = 写侧每次成功写入后 +1：stepper/重置之后读侧必须重算，
    // 否则面板上的数字停在旧值（读侧没有别的刷新信号）。
    LaunchedEffect(kind, session, danmakuOn, danmakuSettings, origin, revision) {
        items = if (kind.isBlank()) {
            emptyList()
        } else {
            withContext(Dispatchers.IO) {
                enumeratePanel(kind, session, danmakuOn, danmakuSettings, origin, context)
            }
        }
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
    "resource" -> "播放资源"
    "audioDelay" -> "音频延迟"
    "subtitleDisplay" -> "字幕显示设置"
    "subtitleTracks" -> "内嵌字幕"
    "subtitleExternal" -> "外挂字幕"
    "skip" -> "跳过片段"
    "skipMode" -> "跳过片段方式"
    "danmakuOps" -> "弹幕"
    "danmakuSettings" -> "弹幕显示设置"
    else -> ""
}

/** 真正的枚举 —— 只在 IO 线程跑。 */
private fun enumeratePanel(
    kind: String,
    mpv: MpvPlaybackSession?,
    danmakuOn: Boolean,
    danmakuSettings: com.kivo.cinema.danmaku.DanmakuSettings,
    origin: PlayerSourceOrigin,
    context: Context,
): List<PlayerOptionItem> {
    val forms = PlayerPanelForms
    // 无内核依赖的形态先返回：起播前也能看到完整形态与缺能力原因。
    when (kind) {
        "skip" -> return forms.skip()
        "skipMode" -> return forms.skipMode()
        "episodes" -> return forms.episodes()
        "danmakuOps" -> return forms.danmakuOps(danmakuOn, danmakuSettings.hasItems)
        "danmakuSettings" -> return PlayerPanelData.danmakuSettings(danmakuSettings)
    }
    if (mpv == null) return forms.notReady()
    val data = PlayerPanelData
    return when (kind) {
        "speed" -> data.speed(data.currentSpeedLabel(mpv))
        "quality" -> data.qualities(mpv.readProperty("video-aspect-override").orEmpty())
        "audioDelay" -> data.audioDelay(mpv.readProperty("audio-delay")?.toDoubleOrNull() ?: 0.0)
        "subtitleDisplay" -> subtitleDisplay(mpv)
        "audio" -> {
            val catalog = mpv.tracksOrNull() ?: return forms.notReady()
            data.audioTracks(catalog.audioTracks(), catalog.currentTrackId("audio")) +
                forms.audioExtras(mpv.readProperty("audio-delay")?.toDoubleOrNull() ?: 0.0)
        }
        "subtitle" -> subtitleEntry(mpv)
        "subtitleTracks" -> {
            val catalog = mpv.tracksOrNull() ?: return forms.notReady()
            val embedded = catalog.subtitleTracks().filter { !it.external }
            if (embedded.isEmpty()) forms.noEmbeddedSubtitles()
            else data.subtitleTracks(embedded, catalog.currentTrackId("sub"))
        }
        "subtitleExternal" -> {
            val catalog = mpv.tracksOrNull() ?: return forms.notReady()
            val external = catalog.subtitleTracks().filter { it.external }
            if (external.isEmpty()) forms.noExternalSubtitles()
            else data.subtitleTracks(external, catalog.currentTrackId("sub"))
        }
        "resource" -> resource(mpv, origin, context)
        else -> emptyList()
    }
}

/**
 * 字幕面板入口（形态 1，iPhone 现屏行序）：
 * 内嵌字幕> / 外挂字幕> 两条钻入行 + 搜索/导入/显示设置/关闭。
 * 副题是内核当前真值：内嵌＝当前轨名或"关闭"，外挂＝当前轨名或"无"。
 */
private fun subtitleEntry(mpv: MpvPlaybackSession): List<PlayerOptionItem> {
    val catalog = mpv.tracksOrNull() ?: return PlayerPanelForms.notReady()
    val tracks = catalog.subtitleTracks()
    val current = tracks.firstOrNull { it.id == catalog.currentTrackId("sub") }
    val embedded = current?.takeIf { !it.external }?.displayName(current.id) ?: "关闭"
    val external = current?.takeIf { it.external }?.displayName(current.id) ?: "无"
    return listOf(
        PlayerOptionItem(
            title = "内嵌字幕",
            subtitle = embedded,
            icon = "captions",
            tag = "subtitleTracks",
            control = PlayerRowControl.CHEVRON,
        ),
        PlayerOptionItem(
            title = "外挂字幕",
            subtitle = external,
            icon = "doc",
            tag = "subtitleExternal",
            control = PlayerRowControl.CHEVRON,
        ),
    ) + PlayerPanelForms.subtitleActions()
}

/** 五个 sub-* 真值任一读不到 ⇒ 不编默认值，直接报"未就绪"。 */
private fun subtitleDisplay(mpv: MpvPlaybackSession): List<PlayerOptionItem> {
    val font = mpv.readProperty("sub-font-size")?.toDoubleOrNull()
    val pos = mpv.readProperty("sub-pos")?.toDoubleOrNull()
    val delay = mpv.readProperty("sub-delay")?.toDoubleOrNull()
    val border = mpv.readProperty("sub-border-size")?.toDoubleOrNull()
    if (font == null || pos == null || delay == null || border == null) {
        return PlayerPanelForms.notReady()
    }
    val standard = mpv.readProperty("sub-ass-override") == "yes"
    return PlayerPanelData.subtitleDisplay(font, pos, delay, border, standard)
}

/** 资源卡：徽章/体积/码率来自内核，源名来自来源推导（路由器/NAS/Emby 名，不是文件名）。 */
private fun resource(mpv: MpvPlaybackSession, origin: PlayerSourceOrigin, context: Context): List<PlayerOptionItem> {
    val gamma = mpv.readProperty("video-params/gamma").orEmpty()
    val badge = when (gamma) {
        "pq" -> "HDR10"
        "hlg" -> "HLG"
        else -> "SDR"
    }
    val w = mpv.readProperty("video-params/w")?.toIntOrNull() ?: 0
    val h = mpv.readProperty("video-params/h")?.toIntOrNull() ?: 0
    // 与底栏徽章共用同一分档（PlayerResolution）：1920×870 宽银幕也是 1080P。
    val chip = resolutionTier(w, h).ifEmpty { if (h > 0) "${h}P" else "未知" }
    val bytes = mpv.readProperty("file-size")?.toLongOrNull() ?: 0L
    val bitrate = mpv.readProperty("video-bitrate")?.toLongOrNull() ?: 0L
    val size = if (bytes > 0) String.format("%.2f GB", bytes / 1e9) else "体积未知"
    val rate = if (bitrate > 0) String.format("%.2fMbps", bitrate / 1e6) else "码率未知"
    return PlayerPanelData.resource(badge, "$size · $rate", origin.resolve(context), chip)
}