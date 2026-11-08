package com.kivo.cinema.ui.detail

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.PosterPalette
import com.kivo.cinema.ui.design.RatingBadges
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.TmdbImageCache
import com.kivo.cinema.ui.design.home.HomeAtmosphere
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 影片详情页（240 §1/§5）。
 *
 * 色场完全复用首页机制（239 §6 / HomeAtmosphere）：艺术图 → PosterPalette.ambientOf
 * 取环境色 → 页面底色直接画成 HomeAtmosphere.background(环境色, 滚动dp) → 幕布下缘
 * 用同一色场按首页曲线盖满，于是图与页面同色、无缝；滚动时色场过渡到页面浅色。
 *
 * 数据 = TmdbDetail 一次 append_to_response；无数据时给出原因（G9），不渲染空壳节。
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
        setDarkStatusIcons(false)   // 首屏是暗色场 → 状态栏用浅色图标
        val id = intent.getIntExtra(EXTRA_ID, 0)
        val isTv = intent.getBooleanExtra(EXTRA_TV, false)
        val detail = TmdbDetail(TmdbClient(this))
        setContent {
            DetailPage(
                id = id, isTv = isTv,
                load = { detail.details(isTv, id) },
                onBack = { finish() },
                onDarkStatusIcons = { setDarkStatusIcons(it) },
            )
        }
    }

    private fun setDarkStatusIcons(dark: Boolean) {
        WindowCompat.getInsetsController(window, window.decorView)
            .isAppearanceLightStatusBars = dark
    }

    @Composable
    private fun DetailPage(id: Int, isTv: Boolean, load: () -> TmdbDetail.Details?,
                           onBack: () -> Unit, onDarkStatusIcons: (Boolean) -> Unit) {
        val context = LocalContext.current
        var details by remember { mutableStateOf<TmdbDetail.Details?>(null) }
        var error by remember { mutableStateOf<String?>(null) }
        // 与首页同名同义：环境色（由艺术图下部实测得来）
        var fieldColor by remember { mutableStateOf(DesignTokens.pageBackground) }
        LaunchedEffect(id, isTv) {
            val d = withContext(Dispatchers.IO) { load() }
            if (d == null) {
                error = "加载失败，请检查网络后重试"
            } else {
                details = d
                d.posterUrl.takeIf { it.isNotEmpty() }?.let { url ->
                    val bmp = withContext(Dispatchers.IO) { TmdbImageCache.load(url) }
                    bmp?.let { fieldColor = PosterPalette.ambientOf(it) }
                }
            }
        }
        val listState = rememberLazyListState()
        val density = LocalDensity.current
        // 首页同款：色场 = 环境色 →（滚动 390dp）→ 页面浅色
        val scrimColor: State<Color> = remember {
            derivedStateOf {
                val scrollDp = if (listState.firstVisibleItemIndex == 0)
                    listState.firstVisibleItemScrollOffset / density.density else 390f
                androidx.compose.ui.graphics.lerp(
                    fieldColor, DesignTokens.pageBackground, (scrollDp / 390f).coerceIn(0f, 1f))
            }
        }
        val chromeDark by remember {
            derivedStateOf { HomeAtmosphere.lightForeground(scrimColor.value) }
        }
        LaunchedEffect(chromeDark) { onDarkStatusIcons(!chromeDark) }
        val fg = if (chromeDark) Color.White else DesignTokens.textMain
        val fgSub = if (chromeDark) Color.White.copy(alpha = .72f) else DesignTokens.textSub

        Box(Modifier.fillMaxSize().drawBehind { drawRect(scrimColor.value) }) {
            LazyColumn(
                state = listState,
                modifier = Modifier.fillMaxSize(),
                contentPadding = PaddingValues(bottom = 60.dp),
            ) {
                item(key = "hero") {
                    Box(Modifier.fillMaxWidth().height(DETAIL_HERO_DP.dp)) {
                        HeroArt(details, error, fg, scrimColor)
                        val h = details
                        if (h != null) {
                            Box(Modifier.fillMaxWidth().offset(y = DETAIL_META_TOP_DP.dp)) {
                                MetaLine(h, fg)
                            }
                            Box(Modifier.fillMaxWidth().offset(y = DETAIL_ACTIONS_TOP_DP.dp)) {
                                ActionRow(isTv, context)
                            }
                        }
                    }
                }
                val d = details
                if (d != null) {
                    if (d.overview.isNotBlank()) item(key = "overview") { Overview(d.overview, fg) }
                    if (d.rating > 0f) item(key = "badges") {
                        Box(Modifier.fillMaxWidth().padding(top = 14.dp)) {
                            RatingBadges(listOf("TMDB" to d.rating),
                                modifier = Modifier.fillMaxWidth(), alignCenter = true,
                                valueColor = fg)
                        }
                    }
                    // 无本地资源 → 空态块（240 §6.1）；有本地文件时此处改为 播放资源 + 媒体信息卡
                    item(key = "empty") {
                        Column {
                            Spacer(Modifier.height(70.dp))
                            EmptyResourceBlock(fg)
                        }
                    }
                    d.studioLogo?.let { url ->
                        item(key = "studio") { StudioLogo(url, fg) }
                    }
                    if (d.cast.isNotEmpty()) item(key = "cast") {
                        Column {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("演职员", fg)
                            Spacer(Modifier.height(14.dp))
                            CastRow(d.cast, fg, fgSub)
                        }
                    }
                    if (d.certifications.isNotEmpty() || d.contentNotes.isNotEmpty()) {
                        item(key = "guide") {
                            Column {
                                Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                                SectionHeader("家长指南", fg)
                                Spacer(Modifier.height(20.dp))
                                ParentalGuide(d.certifications, d.contentNotes, fg, fgSub)
                            }
                        }
                    }
                    if (d.genres.isNotEmpty()) item(key = "genres") {
                        Column {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("分类", fg)
                            Spacer(Modifier.height(12.dp))
                            GenreChips(d.genres)
                        }
                    }
                    d.trailerKey?.let { key ->
                        item(key = "trailer") {
                            Column {
                                Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                                SectionHeader("预告片", fg)
                                Spacer(Modifier.height(12.dp))
                                TrailerCard(key, "预告片", onClick = {
                                    context.startActivity(Intent(Intent.ACTION_VIEW,
                                        Uri.parse("https://www.youtube.com/watch?v=$key")))
                                })
                            }
                        }
                    }
                    if (d.stills.isNotEmpty()) item(key = "stills") {
                        Column {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("剧照", fg)
                            Spacer(Modifier.height(12.dp))
                            StillsRow(d.stills)
                        }
                    }
                    if (d.similar.isNotEmpty()) item(key = "similar") {
                        Column {
                            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                            SectionHeader("相似作品", fg)
                            Spacer(Modifier.height(10.dp))
                            SimilarList(d.similar.take(8), fg, fgSub, onOpen = {})
                        }
                    }
                } else if (error != null) {
                    item(key = "error") {
                        Text(error!!, fontSize = 14.sp, color = fgSub,
                            modifier = Modifier.padding(start = DETAIL_PAD_DP.dp, top = 16.dp))
                    }
                }
            }
            BackCircle(onBack)
            // 问题反馈（G9：入口常驻，能力未接入时点击给出可见原因，不静默）
            Box(Modifier.align(Alignment.BottomCenter).padding(bottom = 24.dp)) {
                FeedbackPill {
                    Toast.makeText(context,
                        "问题反馈将在设置页统一提供", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    /** 幕布：507dp 艺术图原色铺满，**不铺遮罩**——页面底色即艺术图下部色，天然无缝（240 §6）。 */
    @Composable
    private fun HeroArt(details: TmdbDetail.Details?, error: String?, fg: Color,
                        scrimColor: State<Color>) {
        Box(Modifier.fillMaxSize()) {
            val d = details
            if (d != null && d.posterUrl.isNotEmpty()) {
                TmdbImage(d.posterUrl, d.title, Modifier.fillMaxSize())
            }
            // 下缘色场（首页同款机制，起点下移到 55% 以保住海报标题区）
            Box(Modifier.fillMaxSize().drawBehind {
                val c = scrimColor.value
                drawRect(Brush.verticalGradient(
                    0f to c.copy(alpha = 0f),
                    .72f to c.copy(alpha = 0f),
                    .85f to c.copy(alpha = .55f),
                    .94f to c.copy(alpha = .88f),
                    1f to c))
            })
            if (d == null && error != null) {
                Text(error, fontSize = 13.sp, color = fg,
                    textAlign = TextAlign.Center,
                    modifier = Modifier.align(Alignment.Center).padding(horizontal = 24.dp))
            }
        }
    }

    /** meta 行（240 §5.2）：日期 · 地区 · 片长/集数，居中 17sp。 */
    @Composable
    private fun MetaLine(d: TmdbDetail.Details, fg: Color) {
        Text(
            text = d.metaLine(),
            fontSize = 13.sp, lineHeight = 13.sp,
            color = fg, textAlign = TextAlign.Center,
            modifier = Modifier.fillMaxWidth()
                .padding(horizontal = DETAIL_PAD_DP.dp),
        )
    }

    /** 简介：左对齐 15sp/行高 14，最多 2 行（240 §5.2 E4）。 */
    @Composable
    private fun Overview(text: String, fg: Color) {
        Text(
            text = text,
            fontSize = 15.sp, lineHeight = 14.sp,
            color = fg.copy(alpha = 0.82f),
            maxLines = 2, overflow = TextOverflow.Ellipsis,
            modifier = Modifier.fillMaxWidth()
                .padding(start = DETAIL_PAD_DP.dp, end = DETAIL_PAD_DP.dp, top = 16.dp),
        )
    }

    /** 主操作行（240 §6.2 E6）：玻璃材质（白 α0.28 / α0.147）+ 发丝描边 + 真图标。 */
    @Composable
    private fun ActionRow(isTv: Boolean, context: Context) {
        val reason = "未加载到相关资源"
        val hair = Color.White.copy(alpha = .10f)
        val glyph = Color.White.copy(alpha = .95f)
        Row(
            Modifier.fillMaxWidth().padding(horizontal = DETAIL_PAD_DP.dp),
            horizontalArrangement = Arrangement.spacedBy(9.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Box(
                Modifier.weight(1f).height(45.dp)
                    .clip(RoundedCornerShape(50))
                    .background(Color.White.copy(alpha = .28f))
                    .border(1.dp, hair, RoundedCornerShape(50))
                    .clickable {
                        Toast.makeText(context, reason, Toast.LENGTH_SHORT).show()
                    },
                contentAlignment = Alignment.Center,
            ) {
                Text(reason, fontSize = 15.sp, fontWeight = FontWeight.Medium,
                    color = glyph, maxLines = 1, overflow = TextOverflow.Ellipsis)
            }
            Row(
                Modifier.width(101.dp).height(45.dp)
                    .clip(RoundedCornerShape(50))
                    .background(Color.White.copy(alpha = .147f))
                    .border(1.dp, hair, RoundedCornerShape(50)),
                horizontalArrangement = Arrangement.Center,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                CollectionPlusIcon(glyph, Modifier.size(22.dp))
                Spacer(Modifier.width(15.dp))
                HeartIcon(glyph, Modifier.size(20.dp))
            }
            Box(Modifier.size(45.dp).clip(CircleShape)
                .background(Color.White.copy(alpha = .147f))
                .border(1.dp, hair, CircleShape),
                contentAlignment = Alignment.Center) {
                Canvas(Modifier.size(20.dp, 4.dp)) {
                    val r = 2.0.dp.toPx()
                    val gap = (size.width - r * 2) / 2f
                    for (i in 0..2) {
                        drawCircle(Color.White.copy(alpha = .95f), r,
                            Offset(r + gap * i, size.height / 2f))
                    }
                }
            }
        }
    }

    /** 返回钮（240 §5.2 E14）：Ø42 半透明白 + 黑色人字，start16/top37。 */
    @Composable
    private fun BackCircle(onBack: () -> Unit) {
        Box(
            Modifier.padding(start = DETAIL_PAD_DP.dp, top = 42.dp)
                .size(42.dp)
                .clip(CircleShape)
                .background(Color(0x8CFFFFFF))
                .clickable { onBack() },
            contentAlignment = Alignment.Center,
        ) {
            Canvas(Modifier.size(11.dp, 17.dp)) {
                val p = Path().apply {
                    moveTo(size.width, 0f)
                    lineTo(0f, size.height / 2f)
                    lineTo(size.width, size.height)
                }
                drawPath(p, Color(0xFF1A1A1A), style = Stroke(
                    width = 2.2.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
            }
        }
    }
}

/** 幕布高（240 §5.2 实测 507dp；与首页 hero 的 585 解耦，互不影响）。 */
private const val DETAIL_HERO_DP = 589.5f   // 2:3 海报满宽（393×1.5）
/** meta 叠在海报下缘（430pt 基准 558.5 → ×393/430）。 */
private const val DETAIL_META_TOP_DP = 510.5f
/** 主操作行叠在海报下缘。 */
private const val DETAIL_ACTIONS_TOP_DP = 541f
/** 正文左右缩进（帧实测 20dp）；各节组件共用。 */
internal const val DETAIL_PAD_DP = 20f
/** 详情页节标题字号（240 §5.5：同串墨高实测 18sp，非首页的 22sp）。 */
internal const val DETAIL_SECTION_TITLE_SP = 18f
/** 节与节之间（帧实测徽章底 668.7 → 下节标题 720.7）。 */
private const val DETAIL_SECTION_GAP_DP = 42f

/** meta 行：日期 · 地区 · 片长/集数（240 §5.2 E3）。 */
private fun TmdbDetail.Details.metaLine(): String {
    val parts = mutableListOf<String>()
    dateCn()?.let { parts.add(it) }
    region?.let { parts.add(it) }
    if (isTv) episodes?.let { parts.add("${it}集") }
    else runtimeMin?.let { parts.add(runtimeText(it)) }
    return parts.joinToString(" · ")
}

/** "2026-09-16" → "2026年9月16日"（帧实测中文日期格式）。 */
private fun TmdbDetail.Details.dateCn(): String? {
    if (date.isEmpty()) return null
    val p = date.split("-")
    if (p.size != 3) return date
    val m = p[1].toIntOrNull() ?: return date
    val d = p[2].toIntOrNull() ?: return date
    return "${p[0]}年${m}月${d}日"
}

/** 160 → "2小时40分钟"；45 → "45分钟"。 */
private fun runtimeText(min: Int): String {
    if (min < 60) return "${min}分钟"
    val h = min / 60
    val m = min % 60
    return if (m == 0) "${h}小时" else "${h}小时${m}分钟"
}
