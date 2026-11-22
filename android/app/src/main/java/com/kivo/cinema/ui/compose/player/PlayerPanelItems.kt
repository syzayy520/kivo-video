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
import com.kivo.cinema.playback.PlaybackPrefs
import com.kivo.cinema.playback.subtitle.SubtitleCodecMemory
import com.kivo.cinema.subtitle.SubtitleIssue
import com.kivo.cinema.ui.compose.PlayerComposeActivity
import com.kivo.cinema.ui.compose.aiSubtitleController
import com.kivo.cinema.ui.compose.currentSubtitleFileKey
import com.kivo.cinema.ui.compose.player.subtitle.SubtitlePanelRows
import com.kivo.cinema.ui.compose.player.subtitle.aiRowSubtitle
import com.kivo.cinema.ui.compose.player.subtitle.isTextSubtitleCodec
import com.kivo.cinema.ui.compose.readProperty
import com.kivo.cinema.ui.compose.smbTrace
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
        // 投影态观察 trace：string 接口读 track-list/count 稳定返回 0（2026-11-22
        // 实测），node 接口枚举（tracksOrNull）同样空——字幕条件层在本会话不可
        // 靠，withAiSubtitle 跳过该层（乐观 AVAILABLE），通道修复列后续。
        val subCount = session?.readProperty("track-list/count")?.trim()?.toIntOrNull() ?: 0
        (context as? PlayerComposeActivity)?.smbTrace("panel-ai-proj kind=$kind session=${session != null} subCount=$subCount")
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
    "subtitleSecondary" -> "次字幕"
    "subtitleEncoding" -> "字幕编码"
    "skip" -> "跳过片段"
    "skipMode" -> "跳过片段方式"
    "danmakuOps" -> "弹幕"
    "danmakuSettings" -> "弹幕显示设置"
    else -> ""
}

/** AI 总开关关闭（UX v3 禁用态）：移除面板里的「AI 中文字幕」入口行，不灰显不占位。 */
private fun stripAiRow(items: List<PlayerOptionItem>, aiEnabled: Boolean): List<PlayerOptionItem> =
    if (aiEnabled) items else items.filter { it.tag != "sub-ai" }

/**
 * AI 行副行投影（UX v3 §3 四态，T-C 评审登记偏差的落地）：
 * Controller.entryState 快照 → aiRowSubtitle 文案，写入 sub-ai 行副行。
 * 面板打开时快照一次；onDone 的 panelRevision++ 是完成态的刷新通道。
 */
private fun withAiSubtitle(items: List<PlayerOptionItem>, context: Context): List<PlayerOptionItem> {
    val activity = context as? PlayerComposeActivity
    if (activity == null) {
        android.util.Log.w("kivo-ai-proj", "context not PlayerComposeActivity: $context")
        return items
    }
    activity.smbTrace("ai-row-proj enter items=${items.size}")
    val controller = activity.aiSubtitleController()
    // 字幕条件层跳过（偏差登记）：track-list 判定不可靠（string 接口 count=0 /
    // node 接口枚举空，2026-11-22 实测），乐观投影 AVAILABLE；无字幕片的纠正由
    // 点击 toast 给非空 reason（ch4/ch4b），不静默。通道修复列后续。
    val state = controller.entryState(
        featureEnabled = true,
        hasEnglishSrt = true,
        subtitleIssue = SubtitleIssue.NONE,
    )
    val subtitleText = aiRowSubtitle(state, controller.currentProgress)
    activity.smbTrace("ai-row-proj state=$state text=$subtitleText")
    return items.map { if (it.tag == "sub-ai") it.copy(subtitle = subtitleText) else it }
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
        "episodes" -> return episodesFromIntent(context) ?: forms.episodes()
        "danmakuOps" -> return forms.danmakuOps(danmakuOn, danmakuSettings.hasItems)
        "danmakuSettings" -> return PlayerPanelData.danmakuSettings(danmakuSettings)
        "subtitleEncoding" -> return subtitleEncoding(context)
    }
    if (mpv == null) return forms.notReady()
    val data = PlayerPanelData
    return when (kind) {
        "speed" -> data.speed(data.currentSpeedLabel(mpv))
        "quality" -> data.qualities(mpv.readProperty("video-aspect-override").orEmpty())
        "audioDelay" -> data.audioDelay(mpv.readProperty("audio-delay")?.toDoubleOrNull() ?: 0.0)
        "subtitleDisplay" -> SubtitlePanelRows.display(mpv)
        "audio" -> {
            val catalog = mpv.tracksOrNull() ?: return forms.notReady()
            data.audioTracks(catalog.audioTracks(), catalog.currentTrackId("audio")) +
                forms.audioExtras(mpv.readProperty("audio-delay")?.toDoubleOrNull() ?: 0.0)
        }
        "subtitle" -> stripAiRow(withAiSubtitle(SubtitlePanelRows.entry(mpv), context), PlaybackPrefs(context).aiSubtitleEnabled)
        "subtitleTracks" -> {
            val catalog = mpv.tracksOrNull() ?: return forms.notReady()
            val embedded = catalog.subtitleTracks().filter { !it.external }
            if (embedded.isEmpty()) forms.noEmbeddedSubtitles()
            else SubtitlePanelRows.embedded(embedded, catalog.currentTrackId("sub"))
        }
        "subtitleExternal" -> {
            val catalog = mpv.tracksOrNull() ?: return forms.notReady()
            val external = catalog.subtitleTracks().filter { it.external }
            if (external.isEmpty()) stripAiRow(withAiSubtitle(forms.noExternalSubtitles(), context), PlaybackPrefs(context).aiSubtitleEnabled)
            else SubtitlePanelRows.external(external, catalog.currentTrackId("sub"))
        }
        "subtitleSecondary" -> {
            val catalog = mpv.tracksOrNull() ?: return forms.notReady()
            val subs = catalog.subtitleTracks()
            if (subs.isEmpty()) forms.noEmbeddedSubtitles()
            else SubtitlePanelRows.secondary(
                subs,
                mpv.readProperty("secondary-sid")?.trim()?.toIntOrNull(),
                catalog.currentTrackId("sub"),
            )
        }
        "resource" -> resource(mpv, origin, context)
        else -> emptyList()
    }
}

/**
 * 字幕编码面板（R1）：7 档，当前生效项打勾。
 *
 * 【打勾口径】与写侧同源（C-PLUMB-CODEC@1）：当前外挂字幕的 fileKey →
 * [SubtitleCodecMemory] 记住的编码；没记过 / 没挂字幕 ⇒ `"auto"`（自动探测）。
 * 自动探测出来的具体编码**不**回填成勾 —— 记忆里没有它，勾在「自动」才对得上写侧行为。
 */
private fun subtitleEncoding(context: Context): List<PlayerOptionItem> {
    val activity = context as? PlayerComposeActivity
    val current = activity
        ?.let { it.currentSubtitleFileKey()?.let { key -> SubtitleCodecMemory(it).load(key) } }
        ?: SUBTITLE_ENCODING_AUTO
    return SUBTITLE_ENCODINGS.map { codec ->
        PlayerOptionItem(
            title = if (codec == SUBTITLE_ENCODING_AUTO) "自动" else codec,
            selected = codec.equals(current, ignoreCase = true),
            tag = "sub-enc-$codec",
            control = PlayerRowControl.CHECK,
        )
    }
}

/** 行序 = 面板展示顺序；除「自动」外全是 `Charset` 规范名（`Shift-JIS` 由解码侧换算）。 */
private val SUBTITLE_ENCODINGS = listOf(
    SUBTITLE_ENCODING_AUTO,
    "UTF-8",
    "GB18030",
    "BIG5",
    "Shift-JIS",
    "UTF-16LE",
    "UTF-16BE",
)

private const val SUBTITLE_ENCODING_AUTO = "auto"

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

/**
 * 317 · 选集面板的真实数据（来自详情页打包的 intent 载荷）。
 *
 * @return null = 没有剧集载荷（单片片源 / 非剧集）⇒ 调用方退回占位说明行。
 *
 * 卡片两行文案按 SenPlayer 的形态排：**上「第N集」下「集名」**
 * （[PlayerEpisodeStrip.EpisodeThumb] 里 subtitle 在上、title 在下）。
 */
private fun episodesFromIntent(context: Context): List<PlayerOptionItem>? {
    val activity = context as? android.app.Activity ?: return null
    val blocks = com.kivo.cinema.playback.EpisodeListPayload.decode(
        activity.intent.getStringExtra(com.kivo.cinema.playback.EpisodeListPayload.KEY),
    )
    if (blocks.isEmpty()) return null
    val playUrl = activity.intent.getStringExtra("kivo.playurl")
    val (playingSeason, playingEpisode) = com.kivo.cinema.playback.EpisodeListPayload.locate(blocks, playUrl)
    // 面板当前展示哪一季：-1 = 跟随正在播的那一季（用户点季标签后才有明确值）。
    val override = (activity as? com.kivo.cinema.ui.compose.PlayerComposeActivity)?.episodeSeasonPick ?: -1
    val shownSeason = if (override in blocks.indices) override else playingSeason

    val items = mutableListOf<PlayerOptionItem>()
    // ① 季标签行（SenPlayer 形态：当前季打勾，其余可点）
    if (blocks.size > 1) {
        blocks.forEachIndexed { si, b ->
            items += PlayerOptionItem(
                title = b.title,
                selected = si == shownSeason,
                tag = "season:$si",
                section = "season",
            )
        }
    }
    // ② 当前展示季的集卡
    blocks.getOrNull(shownSeason)?.entries?.forEachIndexed { ei, e ->
        items += PlayerOptionItem(
            title = e.name.ifBlank { "第${e.index}集" },
            subtitle = "第${e.index}集",
            selected = shownSeason == playingSeason && ei == playingEpisode,
            enabled = true,
            tag = "ep:$shownSeason:$ei",
            section = "episode",
            imageUrl = e.thumbUrl.ifBlank { null },
        )
    }
    return items
}
