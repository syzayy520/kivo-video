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
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.media.marks.FilmKey
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.tmdb.TmdbHome
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

        fun intent(context: Context, item: TmdbHome.Item): Intent =
            Intent(context, MovieDetailActivity::class.java)
                .putExtra(EXTRA_ID, item.id)
                .putExtra(EXTRA_TV, item.isTv)

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
    }

    private var openingPage = false

    override fun onResume() {
        super.onResume()
        openingPage = false
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
        enableEdgeToEdge()
        setDarkStatusIcons(false)
        val id = intent.getIntExtra(EXTRA_ID, 0)
        val isTv = intent.getBooleanExtra(EXTRA_TV, false)
        val embyItemId = intent.getStringExtra(EXTRA_EMBY_ITEM)?.takeIf { it.isNotBlank() }
        if (id <= 0 && embyItemId == null) {
            Toast.makeText(this, "无法打开详情：影片 ID 无效", Toast.LENGTH_LONG).show()
            finish()
            return
        }
        val emby = embyItemId?.let { EmbyResource.from(this, intent, it) }
        val tmdb = TmdbDetail(TmdbClient(this))
        val load: () -> DetailContent? =
            if (emby != null) emby::loadDetails else { { localDetailContent(this, tmdb, isTv, id) } }
        setContent {
            val actions = if (emby != null) emby.actions()
                else rememberDetailActions(FilmKey(id, isTv))
            DetailPage(actions, load,
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

