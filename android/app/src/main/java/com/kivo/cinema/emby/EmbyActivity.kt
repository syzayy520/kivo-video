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
import com.kivo.cinema.sources.access.SourceAccessSession
import com.kivo.cinema.sources.access.SourceUnlockController
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.emby.ui.EmbyConnectionScreen
import com.kivo.cinema.emby.ui.EmbyLibraryScreen
import com.kivo.cinema.ui.compose.PlayerComposeActivity

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
        val model = ViewModelProvider(this)[EmbySessionModel::class.java]
        val sourceId = intent.getStringExtra("kivo.sourceId")
        setContent {
            var checked by remember { mutableStateOf(sourceId == null) }
            var protected by remember { mutableStateOf(true) }
            var gateError by remember { mutableStateOf<String?>(null) }
            val unlocked = SourceAccessSession.unlocked
            LaunchedEffect(sourceId, unlocked) {
                if (sourceId != null) {
                    val result = withContext(Dispatchers.IO) { SourceStore(this@EmbyActivity).read() }
                    val source = result.getOrNull()?.firstOrNull { it.id == sourceId }
                    protected = source?.hidden ?: true
                    checked = source != null
                    gateError = if (source == null) "无法读取此资源库，请返回资源库列表重试" else null
                    if (source != null && (!protected || unlocked)) model.openSaved(this@EmbyActivity, sourceId)
                    else model.back()
                }
            }
            SideEffect {
                if (sourceId != null && protected) window.addFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
                else window.clearFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
            }
            MaterialTheme(colorScheme = lightColorScheme(
                primary = Color(0xFF238A59), onPrimary = Color.White,
                background = Color(0xFFF5F4F8), surface = Color.White,
                onSurface = Color(0xFF20222A), onSurfaceVariant = Color(0xFF70727F)
            )) {
                BackHandler(enabled = model.inLibrary || model.connecting) {
                    model.back()
                    if (sourceId != null) finish()
                }
                Box(Modifier.fillMaxSize().background(MaterialTheme.colorScheme.background)
                    .safeDrawingPadding().imePadding()) {
                    if (sourceId != null && (!checked || (protected && !unlocked))) {
                        Column(Modifier.fillMaxSize().padding(28.dp), verticalArrangement = Arrangement.Center,
                            horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally) {
                            Text(gateError ?: if (checked) "资源库已锁定" else "正在读取资源库")
                            if (checked) Button(onClick = { unlock.authenticate({ gateError = it }, {}) }) { Text("验证身份并解锁") }
                            TextButton(onClick = { model.back(); finish() }) { Text("返回资源库") }
                        }
                    } else if (model.inLibrary) EmbyLibraryScreen(model, onBack = { model.back(); if (sourceId != null) finish() }) { item ->
                        model.streamUrl(item)?.let { url ->
                            startActivity(Intent(this@EmbyActivity, PlayerComposeActivity::class.java)
                                .putExtra("kivo.playurl", url).putExtra("kivo.label", item.name)
                                .putExtra("kivo.routing", "emby-direct"))
                        }
                    } else if (sourceId != null) Column(Modifier.fillMaxSize().padding(28.dp),
                        verticalArrangement = Arrangement.Center, horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally) {
                        if (model.connecting) { CircularProgressIndicator(); Spacer(Modifier.height(16.dp)); Text("正在连接资源库") }
                        else {
                            Text(model.error ?: "连接未完成", color = MaterialTheme.colorScheme.error)
                            Button(onClick = { model.openSaved(this@EmbyActivity, sourceId, retry = true) }) { Text("重试") }
                        }
                        TextButton(onClick = { model.cancel(); finish() }) { Text("返回资源库") }
                    } else EmbyConnectionScreen(model) { model.cancel(); finish() }
                }
            }
        }
    }
}
