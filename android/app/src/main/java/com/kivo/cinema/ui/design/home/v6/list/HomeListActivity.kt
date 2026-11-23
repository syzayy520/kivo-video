package com.kivo.cinema.ui.design.home.v6.list

import android.content.Context
import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.core.view.WindowCompat
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.TmdbList
import com.kivo.cinema.ui.detail.MovieDetailActivity

/**
 * 「查看全部」页外壳：读 Intent（榜单 + 标题）→ 交给 [HomeListScreen]，点海报进详情，返回即 finish。
 * 窗口口径同 CalendarActivity：edge-to-edge、浅色画布 ⇒ 状态栏 / 导航栏图标固定深色。
 */
class HomeListActivity : ComponentActivity() {

    companion object {
        private const val EXTRA_KIND = "home_list.kind"
        private const val EXTRA_TITLE = "home_list.title"

        fun intent(context: Context, kind: TmdbList, title: String): Intent =
            Intent(context, HomeListActivity::class.java).putExtra(EXTRA_KIND, kind.name).putExtra(EXTRA_TITLE, title)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        if (android.os.Build.VERSION.SDK_INT >= 29) window.isNavigationBarContrastEnforced = false
        window.navigationBarColor = android.graphics.Color.TRANSPARENT
        WindowCompat.getInsetsController(window, window.decorView).apply {
            isAppearanceLightStatusBars = true
            isAppearanceLightNavigationBars = true
        }
        // 榜单名不认识（旧版本残留的 Intent 等）⇒ 直接关闭，不显示一个永远空的页面。
        val kind = intent.getStringExtra(EXTRA_KIND)?.let { runCatching { TmdbList.valueOf(it) }.getOrNull() }
            ?: return finish()
        val title = intent.getStringExtra(EXTRA_TITLE).orEmpty()
        val home = TmdbHome(TmdbClient(applicationContext))
        setContent {
            val scope = rememberCoroutineScope()
            val pager = remember { HomeListPager(home, kind, scope) }
            HomeListScreen(title, pager, onBack = { finish() },
                onOpen = { startActivity(MovieDetailActivity.intent(this, it)) })
        }
    }
}
