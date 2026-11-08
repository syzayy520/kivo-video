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
import androidx.lifecycle.ViewModelProvider
import com.kivo.cinema.emby.ui.EmbyConnectionScreen
import com.kivo.cinema.emby.ui.EmbyLibraryScreen
import com.kivo.cinema.ui.compose.PlayerComposeActivity

/** Emby host: navigation and theme only; session lives across configuration changes. */
class EmbyActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        @Suppress("DEPRECATION")
        window.statusBarColor = android.graphics.Color.rgb(245, 244, 248)
        @Suppress("DEPRECATION")
        window.navigationBarColor = android.graphics.Color.rgb(245, 244, 248)
        androidx.core.view.WindowCompat.getInsetsController(window, window.decorView).apply {
            isAppearanceLightStatusBars = true
            isAppearanceLightNavigationBars = true
        }
        val model = ViewModelProvider(this)[EmbySessionModel::class.java]
        setContent {
            MaterialTheme(colorScheme = lightColorScheme(
                primary = Color(0xFF238A59), onPrimary = Color.White,
                background = Color(0xFFF5F4F8), surface = Color.White,
                onSurface = Color(0xFF20222A), onSurfaceVariant = Color(0xFF70727F)
            )) {
                BackHandler(enabled = model.inLibrary || model.connecting) { model.back() }
                Box(Modifier.fillMaxSize().background(MaterialTheme.colorScheme.background)
                    .safeDrawingPadding().imePadding()) {
                    if (model.inLibrary) EmbyLibraryScreen(model, onBack = { model.back() }) { item ->
                        model.streamUrl(item)?.let { url ->
                            startActivity(Intent(this@EmbyActivity, PlayerComposeActivity::class.java)
                                .putExtra("kivo.playurl", url).putExtra("kivo.label", item.name)
                                .putExtra("kivo.routing", "emby-direct"))
                        }
                    } else EmbyConnectionScreen(model) { model.cancel(); finish() }
                }
            }
        }
    }
}
