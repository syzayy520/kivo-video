package com.kivo.cinema.ui.detail

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.State
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassCircleButton
import com.kivo.cinema.ui.design.RatingBadges
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.HERO_HEIGHT_DP
import com.kivo.cinema.ui.design.META_TOP_DP
import com.kivo.cinema.ui.design.BADGE_TOP_DP
import com.kivo.cinema.ui.design.TmdbImageCache
import com.kivo.cinema.ui.design.home.HomeAtmosphere
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 影片详情页（240 §1/§2）：幕布原色全画 → 环境色遮罩盖外层（与首页同构）
 * → 下缘场过渡到页底 → 主操作行 → 演职员/分类/预告/相似作品各节。
 * 数据 = TmdbDetail 一次 append_to_response；无数据时 meta/简介隐藏并给出原因（G9）。
 */
class MovieDetailActivity : ComponentActivity() {

    companion object {
        const val EXTRA_ID = "kivo.tmdbId"
        const val EXTRA_TV = "kivo.tmdbTv"

        fun intent(context: Context, item: com.kivo.cinema.tmdb.TmdbHome.Item): Intent =
            Intent(context, MovieDetailActivity::class.java)
                .putExtra(EXTRA_ID, item.id)
                .putExtra(EXTRA_TV, item.isTv)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val id = intent.getIntExtra(EXTRA_ID, 0)
        val isTv = intent.getBooleanExtra(EXTRA_TV, false)
        val detail = TmdbDetail(TmdbClient(this))
        setContent {
            DetailPage(id = id, isTv = isTv, load = { detail.details(isTv, id) },
                onBack = { finish() })
        }
    }

    @Composable
    private fun DetailPage(id: Int, isTv: Boolean, load: () -> TmdbDetail.Details?,
                           onBack: () -> Unit) {
        val context = LocalContext.current
        var details by remember { mutableStateOf<TmdbDetail.Details?>(null) }
        var error by remember { mutableStateOf<String?>(null) }
        var ambient by remember { mutableStateOf<Color?>(null) }
        LaunchedEffect(id, isTv) {
            val d = withContext(Dispatchers.IO) { load() }
            if (d == null) error = "加载失败，请检查网络后重试"
            else {
                details = d
                d.posterUrl.takeIf { it.isNotEmpty() }?.let { url ->
                    val bmp = withContext(Dispatchers.IO) { TmdbImageCache.load(url) }
                    ambient = bmp?.let { com.kivo.cinema.ui.design.PosterPalette.ambientOf(it) }
                }
            }
        }
        val listState = rememberLazyListState()
        val density = androidx.compose.ui.platform.LocalDensity.current
        val ambientColor = ambient ?: DesignTokens.bgCream

        // 环境色遮罩/下缘场：底缘色随滚动向页底过渡（与首页同构）
        val fieldColor: State<Color> = remember {
            derivedStateOf {
                val sDp = if (listState.firstVisibleItemIndex == 0)
                    listState.firstVisibleItemScrollOffset / density.density else 0f
                val t = (sDp / 390f).coerceIn(0f, 1f)
                Color(
                    ambientColor.red + (DesignTokens.pageBackground.red - ambientColor.red) * t,
                    ambientColor.green + (DesignTokens.pageBackground.green - ambientColor.green) * t,
                    ambientColor.blue + (DesignTokens.pageBackground.blue - ambientColor.blue) * t)
            }
        }
        val fg = if (ambientColor.luminance() < 0.55f) Color.White else DesignTokens.textMain
        val fgSub = if (fg == Color.White) Color.White.copy(alpha = 0.72f)
                    else DesignTokens.textSub

        Box(Modifier.fillMaxSize().background(DesignTokens.pageBackground)) {
            LazyColumn(
                state = listState,
                modifier = Modifier
                    .fillMaxSize()
                    .drawBehind {
                        // 下缘场：内容 [585, 1100dp] 平色（与遮罩同源），底部 29% α 渐隐
                        val fieldTop = HERO_HEIGHT_DP.dp.toPx() -
                            listState.firstVisibleItemScrollOffset.toFloat()
                        val fieldH = 515f.dp.toPx()
                        if (fieldTop < size.height && fieldH > 0f) {
                            val c = fieldColor.value
                            drawRect(
                                brush = Brush.verticalGradient(
                                    0f to c, 0.71f to c, 1f to c.copy(alpha = 0f),
                                    startY = fieldTop, endY = fieldTop + fieldH),
                                topLeft = androidx.compose.ui.geometry.Offset(0f, fieldTop),
                                size = androidx.compose.ui.geometry.Size(size.width, fieldH),
                            )
                        }
                    },
                contentPadding = PaddingValues(bottom = 60.dp),
            ) {
                item(key = "hero") {
                    Box(Modifier.fillMaxWidth().height(HERO_HEIGHT_DP.dp)) {
                        val d = details
                        if (d != null && d.posterUrl.isNotEmpty()) {
                            TmdbImage(d.posterUrl, d.title, Modifier.fillMaxSize())
                        }
                        // 环境色遮罩（海报外层）：与首页同构
                        Box(Modifier.fillMaxSize().drawBehind {
                            val c = fieldColor.value
                            drawRect(Brush.verticalGradient(
                                0f to c.copy(alpha = 0f),
                                0.55f to c.copy(alpha = 0f),
                                0.65f to c.copy(alpha = 0.95f),
                                1f to c.copy(alpha = 0.97f)))
                        })
                        if (error != null && details == null) {
                            Text(error!!, fontSize = 13.sp, color = Color.White,
                                modifier = Modifier.align(Alignment.Center).padding(16.dp))
                        }
                        val dd = details
                        if (dd != null) {
                            Column(Modifier.fillMaxSize().padding(horizontal = 16.dp)) {
                                Spacer(Modifier.height(META_TOP_DP.dp))
                                Text(dd.metaLine(), fontSize = 17.sp, lineHeight = 17.sp,
                                    color = fg, textAlign = TextAlign.Center,
                                    modifier = Modifier.fillMaxWidth())
                                if (dd.overview.isNotBlank()) {
                                    Text(dd.overview, fontSize = 15.sp, lineHeight = 14.sp,
                                        color = fg.copy(alpha = 0.92f), maxLines = 2,
                                        overflow = TextOverflow.Ellipsis,
                                        textAlign = TextAlign.Center,
                                        modifier = Modifier.fillMaxWidth())
                                }
                            }
                            Box(Modifier
                                .offset(y = BADGE_TOP_DP.dp).fillMaxWidth()) {
                                RatingBadges(
                                    if (dd.rating > 0f) listOf("TMDB" to dd.rating)
                                    else emptyList(),
                                    modifier = Modifier.fillMaxWidth(),
                                    alignCenter = true, valueColor = fg)
                            }
                        }
                    }
                }
                // 主操作行（E6）：观看胶囊钮 + (+/♥) 副钮 + ⋯
                item(key = "actions") {
                    Row(
                        Modifier.padding(horizontal = DesignTokens.screenPad.dp,
                            vertical = 10.dp),
                        horizontalArrangement = Arrangement.spacedBy(10.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Text(
                            if (isTv) "观看第1集" else "观看",
                            fontSize = 15.sp, fontWeight = FontWeight.Bold,
                            color = DesignTokens.textMain,
                            modifier = Modifier
                                .weight(1f)
                                .clip(RoundedCornerShape(50))
                                .background(Color(0xFFD8D2DC))
                                .clickable {
                                    Toast.makeText(context,
                                        "未匹配到可播放的本地资源", Toast.LENGTH_SHORT).show()
                                }
                                .padding(vertical = 14.dp),
                            textAlign = TextAlign.Center,
                        )
                        Row(
                            Modifier.width(120.dp).height(44.dp)
                                .clip(RoundedCornerShape(50))
                                .background(Color(0xFF2A272B)),
                            horizontalArrangement = Arrangement.Center,
                            verticalAlignment = Alignment.CenterVertically,
                        ) {
                            Text("+", color = Color.White, fontSize = 18.sp,
                                modifier = Modifier.padding(horizontal = 14.dp))
                            Text("♥", color = Color.White, fontSize = 15.sp,
                                modifier = Modifier.padding(horizontal = 14.dp))
                        }
                        Box(Modifier.size(44.dp).clip(CircleShape)
                            .background(Color(0xFF2A272B)),
                            contentAlignment = Alignment.Center) {
                            Text("⋯", color = Color.White, fontSize = 16.sp)
                        }
                    }
                }
                // 各节（数据缺 → 整节隐藏）
                val d = details
                if (d != null) {
                    if (d.cast.isNotEmpty()) {
                        item(key = "cast") {
                            Column {
                                Spacer(Modifier.height(16.dp))
                                SectionHeader("演职员")
                                Spacer(Modifier.height(14.dp))
                                CastRow(d.cast)
                            }
                        }
                    }
                    if (d.genres.size >= 2) {
                        item(key = "genres") {
                            Column {
                                Spacer(Modifier.height(20.dp))
                                SectionHeader("分类")
                                Spacer(Modifier.height(12.dp))
                                GenreChips(d.genres)
                            }
                        }
                    }
                    d.trailerKey?.let { key ->
                        item(key = "trailer") {
                            Column {
                                Spacer(Modifier.height(20.dp))
                                SectionHeader("预告片")
                                Spacer(Modifier.height(12.dp))
                                TrailerCard(key, "预告片", onClick = {
                                    context.startActivity(Intent(Intent.ACTION_VIEW,
                                        Uri.parse("https://www.youtube.com/watch?v=$key")))
                                })
                            }
                        }
                    }
                    if (d.similar.isNotEmpty()) {
                        item(key = "similar") {
                            Column {
                                Spacer(Modifier.height(20.dp))
                                SectionHeader("相似作品")
                                Spacer(Modifier.height(8.dp))
                                SimilarList(d.similar.take(8), onOpen = {})
                            }
                        }
                    }
                } else if (error != null) {
                    item(key = "error") {
                        Text(error!!, fontSize = 14.sp, color = DesignTokens.textSub,
                            modifier = Modifier.padding(DesignTokens.screenPad.dp))
                    }
                }
            }
            // 返回钮（E14）
            GlassCircleButton(
                modifier = Modifier
                    .padding(start = DesignTokens.screenPad.dp, top = 28.dp)
                    .align(Alignment.TopStart)
                    .clickable { onBack() },
            ) {
                Text("‹", fontSize = 24.sp, color = DesignTokens.textMain)
            }
        }
    }
}

/** 详情 meta 行（E3）：「类型 类型 · 日期 · 地区/集数」。 */
private fun TmdbDetail.Details.metaLine(): String = buildString {
    if (genres.isNotEmpty()) append(genres.take(2).joinToString(" ") + " · ")
    if (date.isNotEmpty()) append(date)
    region?.let { if (isNotEmpty()) append(" · $it") }
    episodes?.let { append(" ${it}集") }
}
