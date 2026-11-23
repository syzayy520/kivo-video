package com.kivo.cinema.ui.design.home.v6.list

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.core.view.WindowCompat
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.NasCategory
import com.kivo.cinema.library.NasLibraryIndex
import com.kivo.cinema.library.NasLibrarySource
import com.kivo.cinema.ui.design.home.nasEntryIntent
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * NAS 分类「查看全部」外壳：读 Intent（分类）→ IO 线程读完整索引（与首页同口径，剔除已删 / 隐藏源）→ [NasCategoryScreen]。
 * 只读本地索引，不触发扫描（扫描是分钟级，挂在页面上会卡死）。
 */
class NasCategoryActivity : ComponentActivity() {

    companion object {
        private const val EXTRA_CATEGORY = "nas_category.key"

        fun intent(context: Context, category: NasCategory): Intent =
            Intent(context, NasCategoryActivity::class.java).putExtra(EXTRA_CATEGORY, category.name)
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
        val category = intent.getStringExtra(EXTRA_CATEGORY)?.let { runCatching { NasCategory.valueOf(it) }.getOrNull() }
            ?: return finish()
        setContent {
            val entries by produceState<List<HomeEntry>?>(null, category) {
                value = withContext(Dispatchers.IO) {
                    NasLibrarySource.entries(NasLibraryIndex.aliveVideos(applicationContext), category, limit = Int.MAX_VALUE)
                }
            }
            NasCategoryScreen(category.label, entries, onBack = { finish() }, onOpen = ::open)
        }
    }

    private fun open(entry: HomeEntry) {
        val target = nasEntryIntent(this, entry)
        if (target != null) startActivity(target)
        else Toast.makeText(this, "无法定位这部影片在 NAS 上的位置，请重新扫描片库", Toast.LENGTH_SHORT).show()
    }
}
