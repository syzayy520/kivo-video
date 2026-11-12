package com.kivo.cinema.emby.aggregate.ui

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.ViewModelProvider
import com.kivo.cinema.emby.aggregate.AggregateViewModel
import com.kivo.cinema.sources.access.SourceAccessSession

/**
 * 256 跨源聚合宿主：主题 + 搜索⇄类型墙切换 + 结果→**所属源**详情导航。
 * 不寄生单源 EmbyActivity；连接/搜索态在 AggregateViewModel 跨配置变更存活。
 */
class AggregateActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        SourceAccessSession.install(application)
        @Suppress("DEPRECATION")
        window.statusBarColor = android.graphics.Color.rgb(245, 244, 248)
        @Suppress("DEPRECATION")
        window.navigationBarColor = android.graphics.Color.rgb(245, 244, 248)
        androidx.core.view.WindowCompat.getInsetsController(window, window.decorView).apply {
            isAppearanceLightStatusBars = true
            isAppearanceLightNavigationBars = true
        }
        val model = ViewModelProvider(this)[AggregateViewModel::class.java]
        setContent {
            var mode by rememberSaveable { mutableStateOf("search") }
            val unlocked = SourceAccessSession.unlocked
            LaunchedEffect(Unit) { model.start(this@AggregateActivity) }
            LaunchedEffect(model.session.sources) {
                if (model.session.sources.isNotEmpty()) model.search.attach(model.session.sources)
            }
            SideEffect {
                if (unlocked) window.addFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
                else window.clearFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
            }
            MaterialTheme(colorScheme = lightColorScheme(
                primary = Color(0xFF238A59), onPrimary = Color.White,
                background = Color(0xFFF5F4F8), surface = Color.White,
                onSurface = Color(0xFF20222A), onSurfaceVariant = Color(0xFF70727F)
            )) {
                BackHandler(enabled = mode != "search") { mode = "search" }
                Column(Modifier.fillMaxSize().background(MaterialTheme.colorScheme.background)
                    .safeDrawingPadding().imePadding()) {
                    Row(Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 4.dp),
                        verticalAlignment = Alignment.CenterVertically) {
                        TextButton(onClick = { finish() }) { Text("‹  返回", fontSize = 16.sp) }
                        Spacer(Modifier.weight(1f))
                        ModeTab("搜索", mode == "search") { mode = "search" }
                        ModeTab("类型", mode == "wall") { mode = "wall" }
                    }
                    val session = model.session
                    when {
                        session.connecting -> CenterBox { CircularProgressIndicator(); Spacer(Modifier.height(12.dp)); Text("正在接入资源库…") }
                        session.sources.isEmpty() -> CenterBox {
                            Text(session.error ?: "没有可聚合搜索的资源库", color = MaterialTheme.colorScheme.error)
                            Spacer(Modifier.height(12.dp))
                            Button(onClick = { model.start(this@AggregateActivity) }) { Text("重试") }
                        }
                        mode == "wall" -> GenreWallScreen(session.sources) { genre ->
                            model.search.pickGenre(genre); mode = "search"
                        }
                        else -> AggregateSearchScreen(model.search, session.sources) { result ->
                            startActivity(result.source.detailIntent(this@AggregateActivity, result.item))
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun ModeTab(label: String, selected: Boolean, onClick: () -> Unit) {
    TextButton(onClick = onClick) {
        Text(label, fontSize = 15.sp,
            fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal,
            color = if (selected) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurfaceVariant)
    }
}

@Composable
private fun CenterBox(content: @Composable ColumnScope.() -> Unit) {
    Box(Modifier.fillMaxSize().padding(28.dp), contentAlignment = Alignment.Center) {
        Column(horizontalAlignment = Alignment.CenterHorizontally, content = content)
    }
}