package com.kivo.cinema.ui.design.home.trailer

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import com.kivo.cinema.BuildConfig
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.trailer.TrailerResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File

/** Poster remains underneath until the hosted player confirms actual playback. */
@Composable
internal fun HeroTrailer(
    item: TmdbHome.Item,
    enabled: Boolean,
    posterReady: Boolean,
    trailerFor: (TmdbHome.Item) -> TrailerResult,
    proxyAddress: String?,
    modifier: Modifier,
    onPlayingChanged: (Boolean) -> Unit,
) {
    val context = LocalContext.current
    val lifecycle = LocalLifecycleOwner.current.lifecycle
    var resumed by remember { mutableStateOf(lifecycle.currentState.isAtLeast(Lifecycle.State.RESUMED)) }
    DisposableEffect(lifecycle) {
        val observer = LifecycleEventObserver { _, _ ->
            resumed = lifecycle.currentState.isAtLeast(Lifecycle.State.RESUMED)
        }
        lifecycle.addObserver(observer)
        onDispose { lifecycle.removeObserver(observer) }
    }
    var attempt by remember(item.id, item.isTv) { mutableIntStateOf(0) }
    var result by remember(item.id, item.isTv, attempt) { mutableStateOf<TrailerResult?>(null) }
    var due by remember { mutableStateOf(false) }
    var playing by remember { mutableStateOf(false) }
    var ended by remember { mutableStateOf(false) }
    var reason by remember { mutableStateOf<String?>(null) }
    var muted by remember { mutableStateOf(true) }
    var player by remember { mutableStateOf<YoutubeTrailerView?>(null) }
    val eligible = enabled && resumed && posterReady
    val provider by rememberUpdatedState(trailerFor)
    val report by rememberUpdatedState(onPlayingChanged)
    fun event(name: String, detail: String? = null) {
        if (BuildConfig.DEBUG) runCatching {
            val file = File(context.filesDir, "evidence/home-preview.jsonl")
            file.parentFile?.mkdirs()
            file.appendText(JSONObject().put("at_ms", android.os.SystemClock.elapsedRealtime())
                .put("media_id", item.id).put("is_tv", item.isTv).put("event", name)
                .put("reason", detail ?: JSONObject.NULL).toString() + "\n")
        }
    }
    LaunchedEffect(eligible, attempt) {
        due = false; playing = false; ended = false; reason = null; muted = true
        if (eligible) {
            event("poster_visible")
            delay(2_000)
            due = true
            event("preview_due")
        }
    }
    LaunchedEffect(eligible, attempt) {
        if (eligible) result = withContext(Dispatchers.IO) { provider(item) }
    }
    val video = result?.video
    LaunchedEffect(eligible, due, result) {
        if (eligible && due && result != null && video == null) {
            reason = result?.reason ?: "暂无可播放的预告片"
            event("unavailable", reason)
        }
    }
    val mount = eligible && due && video != null && reason == null && !ended
    LaunchedEffect(mount) {
        if (mount) {
            delay(20_000)
            if (!playing) { reason = "预告片加载超时，点击重试"; event("timeout", reason) }
        }
    }
    LaunchedEffect(playing, eligible) { report(playing && eligible) }
    Box(modifier) {
        if (mount) AndroidView(
            modifier = Modifier.fillMaxSize().graphicsLayer { alpha = if (playing) 1f else 0f },
            factory = {
                YoutubeTrailerView(it, video!!, proxyAddress,
                    onPlaying = { playing = true; event("playing", video.youtubeId) },
                    onEnded = { ended = true; playing = false; event("ended") },
                    onFailure = { reason = it; playing = false; event("error", it) },
                    onLayoutReport = { event("layout", it) })
                    .also { view -> player = view; view.start() }
            },
            update = { it.setMuted(muted) },
            onRelease = { it.release(); player = null; event("released") },
        )
        // Let pager and vertical list own gestures even over the hosted view.
        if (mount) Box(Modifier.fillMaxSize().clickable(indication = null,
            interactionSource = remember { androidx.compose.foundation.interaction.MutableInteractionSource() }) {})
        if (playing) Text(if (muted) "静音" else "有声", color = Color.White, fontSize = 11.sp,
            modifier = Modifier.align(Alignment.TopEnd).padding(top = 54.dp, end = 16.dp)
                .clip(CircleShape).background(Color.Black.copy(alpha = .28f))
                .clickable { muted = !muted; player?.setMuted(muted) }.padding(horizontal = 10.dp, vertical = 7.dp))
        if (eligible && due && reason != null) Text(reason!!, color = Color.White, fontSize = 10.sp,
            modifier = Modifier.align(Alignment.TopEnd).padding(top = 54.dp, end = 16.dp)
                .clip(CircleShape).background(Color.Black.copy(alpha = .38f))
                .clickable { result = null; attempt++ }.padding(horizontal = 10.dp, vertical = 6.dp))
    }
}
