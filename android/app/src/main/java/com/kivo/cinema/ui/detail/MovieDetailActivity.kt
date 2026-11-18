package com.kivo.cinema.ui.detail

import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.core.view.WindowCompat
import androidx.compose.runtime.remember
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.media.marks.FilmKey
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.detail.actions.localOnlyActions
import com.kivo.cinema.ui.detail.actions.rememberDetailActions

/** 详情壳（254 D14）：只解析 intent（TMDB / Emby 双入口）+ 主题/边缘到边缘；编排在 DetailPage。 */
class MovieDetailActivity : ComponentActivity() {

    companion object {
        const val EXTRA_ID = "kivo.tmdbId"
        const val EXTRA_TV = "kivo.tmdbTv"
        const val EXTRA_EMBY_ITEM = "kivo.embyItemId"
        const val EXTRA_EMBY_SOURCE = "kivo.embySourceId"
        const val EXTRA_EMBY_TOKEN = "kivo.embyToken"
        const val EXTRA_EMBY_USER = "kivo.embyUserId"
        const val EXTRA_EMBY_NAME = "kivo.embyName"
        const val EXTRA_EMBY_YEAR = "kivo.embyYear"
        const val EXTRA_EMBY_SERVER = "kivo.embyServerName"
        const val EXTRA_EMBY_SERIES = "kivo.embySeries"
        /** D12：本地文件入口（沿用旧页关键字，调用方零迁移）。 */
        const val EXTRA_PATH = "kivo.media"
        const val EXTRA_DURATION = "kivo.durationMs"

        fun intent(context: Context, item: TmdbHome.Item): Intent =
            DetailPreview.fill(
                Intent(context, MovieDetailActivity::class.java)
                    .putExtra(EXTRA_ID, item.id)
                    .putExtra(EXTRA_TV, item.isTv),
                item)

        /** D12：本地文件 → 详情（第三入口）；path 即片源，TMDB 只做增强。 */
        fun localIntent(context: Context, item: MediaItem): Intent =
            Intent(context, MovieDetailActivity::class.java)
                .putExtra(EXTRA_PATH, item.path)
                .putExtra(EXTRA_DURATION, item.durationMs)

        /** Emby 海报 → 详情（254 D1）：token/userId 供收藏/已看与播放，凭据只经 intent 不进 URL。 */
        fun embyIntent(context: Context, sourceId: String?, item: EmbyItem,
                       token: String?, userId: String?, serverName: String): Intent =
            Intent(context, MovieDetailActivity::class.java)
                .putExtra(EXTRA_EMBY_ITEM, item.id)
                .putExtra(EXTRA_EMBY_SOURCE, sourceId)
                .putExtra(EXTRA_EMBY_TOKEN, token)
                .putExtra(EXTRA_EMBY_USER, userId)
                .putExtra(EXTRA_EMBY_NAME, item.name)
                .putExtra(EXTRA_EMBY_YEAR, item.year ?: 0)
                .putExtra(EXTRA_EMBY_SERVER, serverName)
                .putExtra(EXTRA_EMBY_SERIES, item.series)
                .putExtra(EXTRA_TV, item.series)
                .putExtra(DetailPreview.EXTRA_TITLE, item.name)
                .putExtra(DetailPreview.EXTRA_DATE, item.year?.toString() ?: "")
                .putStringArrayListExtra(DetailPreview.EXTRA_GENRES, ArrayList(item.genres))
                .putExtra(DetailPreview.EXTRA_RATING, item.rating?.toFloat() ?: 0f)
                .putExtra(DetailPreview.EXTRA_POSTER, item.posterUrl)
                .putExtra(DetailPreview.EXTRA_HERO, item.posterUrl)
                .putExtra(DetailPreview.EXTRA_SOURCE, "emby")
    }

    private var openingPage = false

    /** Emby 详情资源（273 v16：销毁时要关掉没人接手的源站预热）。 */
    private var emby: EmbyResource? = null

    override fun onResume() {
        super.onResume()
        openingPage = false
    }

    override fun onDestroy() {
        // 273 v16：没人接手的预热（用户没点播放就走了）要关掉，
        // 否则源站连接与片尾缓存一直占着。
        emby?.discardPrewarm()
        emby = null
        super.onDestroy()
    }

    private fun openPage(intent: Intent) {
        if (openingPage) return
        openingPage = true
        try { startActivity(intent) }
        catch (_: ActivityNotFoundException) {
            openingPage = false
            Toast.makeText(this, "无法打开：设备没有可用的应用", Toast.LENGTH_LONG).show()
        } catch (_: SecurityException) {
            openingPage = false
            Toast.makeText(this, "无法打开：目标应用拒绝访问", Toast.LENGTH_LONG).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        com.kivo.cinema.net.NetDiag.log("detail-perf activity-create")
        // D366：Emby 慢变元数据缓存（详情/季集/图片尺寸）。幂等，应用内首个入口调一次即可。
        com.kivo.cinema.emby.EmbyResponseCache.init(cacheDir)
        enableEdgeToEdge()
        setDarkStatusIcons(false)
        val id = intent.getIntExtra(EXTRA_ID, 0)
        val isTv = intent.getBooleanExtra(EXTRA_TV, false)
        val embyItemId = intent.getStringExtra(EXTRA_EMBY_ITEM)?.takeIf { it.isNotBlank() }
        val localPath = intent.getStringExtra(EXTRA_PATH)?.takeIf { it.isNotBlank() }
        if (localPath == null && id <= 0 && embyItemId == null) {
            Toast.makeText(this, "无法打开详情：影片 ID 无效", Toast.LENGTH_LONG).show()
            finish()
            return
        }
        val embyRes = embyItemId?.let { EmbyResource.from(this, intent, it) }
        emby = embyRes
        val tmdb = TmdbDetail(TmdbClient(this))
        val durationHint = intent.getLongExtra(EXTRA_DURATION, 0L)
        val load: () -> DetailContent? = when {
            embyRes != null -> embyRes::loadDetails
            localPath != null -> { { localFileContent(this, localPath, durationHint) } }
            else -> { { localDetailContent(this, tmdb, isTv, id) } }
        }
        // 336 R2：列表页已有的标题/海报/评分先渲染（首帧即可见），完整详情后台补。
        // 预览体没有片源匹配/演员/相似，标 preview=true 让页面暂不显示这些未解析区块。
        val previewDetails = DetailPreview.from(intent)
        val previewContent = previewDetails?.let {
            DetailContent(details = it, media = null, quality = "",
                playIntent = null, tmdbId = id, tmdbIsTv = isTv,
                loadImage = embyRes?.previewLoader(), preview = true)
        }
        setContent {
            val actions = when {
                embyRes != null -> embyRes.actions()
                id > 0 -> rememberDetailActions(FilmKey(id, isTv))
                // 本地文件无 TMDB id：FilmKey 要求 id>0，不能构造；给禁用态（不伪造标记键）。
                else -> remember { localOnlyActions() }
            }
            DetailPage(actions, load, previewContent,
                onBack = { finish() },
                onDarkStatusIcons = { setDarkStatusIcons(it) },
                openPage = { openPage(it) })
        }
    }

    private fun setDarkStatusIcons(dark: Boolean) {
        WindowCompat.getInsetsController(window, window.decorView)
            .isAppearanceLightStatusBars = dark
    }

}

