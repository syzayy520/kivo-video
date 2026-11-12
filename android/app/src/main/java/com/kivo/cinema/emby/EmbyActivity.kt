package com.kivo.cinema.emby

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.lifecycle.ViewModelProvider
import androidx.compose.runtime.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import com.kivo.cinema.MainActivity
import com.kivo.cinema.sources.access.SourceAccessSession
import com.kivo.cinema.sources.access.SourceUnlockController
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.emby.ui.EmbyBrowseScreen
import com.kivo.cinema.emby.ui.EmbyLibraryScreen
import com.kivo.cinema.ui.detail.MovieDetailActivity

/** Emby host: navigation and theme only; session lives across configuration changes. */
class EmbyActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        SourceAccessSession.install(application)
        val unlock = SourceUnlockController(this)
        @Suppress("DEPRECATION")
        window.statusBarColor = android.graphics.Color.rgb(245, 244, 248)
        @Suppress("DEPRECATION")
        window.navigationBarColor = android.graphics.Color.rgb(245, 244, 248)
        androidx.core.view.WindowCompat.getInsetsController(window, window.decorView).apply {
            isAppearanceLightStatusBars = true
            isAppearanceLightNavigationBars = true
        }
        val sourceId = intent.getStringExtra("kivo.sourceId")
        if (sourceId == null) {
            // 254 D15：无 sourceId 不是有效入口——添加 Emby 统一走资源库的 workspace 添加流。
            startActivity(Intent(this, MainActivity::class.java)
                .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_SINGLE_TOP))
            finish()
            return
        }
        val model = ViewModelProvider(this)[EmbySessionModel::class.java]
        setContent {
            var checked by remember { mutableStateOf(false) }
            var protected by remember { mutableStateOf(true) }
            var gateError by remember { mutableStateOf<String?>(null) }
            val unlocked = SourceAccessSession.unlocked
            LaunchedEffect(sourceId, unlocked) {
                val result = withContext(Dispatchers.IO) { SourceStore(this@EmbyActivity).read() }
                val source = result.getOrNull()?.firstOrNull { it.id == sourceId }
                protected = source?.hidden ?: true
                checked = source != null
                gateError = if (source == null) "无法读取此资源库，请返回资源库列表重试" else null
                if (source != null && (!protected || unlocked)) model.openSaved(this@EmbyActivity, sourceId)
                else model.back()
            }
            SideEffect {
                if (protected) window.addFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
                else window.clearFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
            }
            MaterialTheme(colorScheme = lightColorScheme(
                primary = Color(0xFF238A59), onPrimary = Color.White,
                background = Color(0xFFF5F4F8), surface = Color.White,
                onSurface = Color(0xFF20222A), onSurfaceVariant = Color(0xFF70727F)
            )) {
                BackHandler(enabled = model.inLibrary || model.connecting) {
                    if (model.drilled != null) model.closeView() else { model.back(); finish() }
                }
                Box(Modifier.fillMaxSize().background(MaterialTheme.colorScheme.background)
                    .safeDrawingPadding().imePadding()) {
                    if (!checked || (protected && !unlocked)) {
                        Column(Modifier.fillMaxSize().padding(28.dp), verticalArrangement = Arrangement.Center,
                            horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally) {
                            Text(gateError ?: if (checked) "资源库已锁定" else "正在读取资源库")
                            if (checked) Button(onClick = { unlock.authenticate({ gateError = it }, {}) }) { Text("验证身份并解锁") }
                            TextButton(onClick = { model.back(); finish() }) { Text("返回资源库") }
                        }
                    } else if (model.inLibrary) {
                        // 254 D1/D14：海报 → 复用详情页（TMDB 智能填充 + Emby 资源槽），不再直接播放。
                        val openDetail: (EmbyItem) -> Unit = { item ->
                            startActivity(MovieDetailActivity.embyIntent(this@EmbyActivity, sourceId, item,
                                model.accessToken, model.accessUserId, model.serverName))
                        }
                        val drilled = model.drilled
                        if (drilled == null) EmbyBrowseScreen(model, onBack = { model.back(); finish() },
                            onOpenView = { model.openView(it) }, onItem = openDetail)
                        else EmbyLibraryScreen(model, onBack = { model.closeView() },
                            title = drilled.name, series = drilled.collectionType == "tvshows",
                            onItem = openDetail)
                    } else Column(Modifier.fillMaxSize().padding(28.dp),
                        verticalArrangement = Arrangement.Center, horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally) {
                        if (model.connecting) { CircularProgressIndicator(); Spacer(Modifier.height(16.dp)); Text("正在连接资源库") }
                        else {
                            Text(model.error ?: "连接未完成", color = MaterialTheme.colorScheme.error)
                            Button(onClick = { model.openSaved(this@EmbyActivity, sourceId, retry = true) }) { Text("重试") }
                        }
                        TextButton(onClick = { model.cancel(); finish() }) { Text("返回资源库") }
                    }
                }
            }
        }
    }
}
