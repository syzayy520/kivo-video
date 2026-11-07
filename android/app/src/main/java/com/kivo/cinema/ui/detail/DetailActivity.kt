package com.kivo.cinema.ui.detail

import android.database.Cursor
import android.os.Bundle
import android.provider.MediaStore as MS
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.ui.design.AnimatedGradientBg
import com.kivo.cinema.ui.design.GlassSurface
import com.kivo.cinema.ui.design.RatingBadges
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 影片详情页（237 方案 P3）：点海报先进详情再播放（参照产品 §6.4 结构）。
 * 22 张 detail/movie 帧未逐帧测量——本页取结构级对齐（海报 hero + 元信息 + 评分行 +
 * 主播放钮），参数级对位待测量任务。评分 = TMDB（无 key/无匹配则行隐藏）。
 */
class DetailActivity : ComponentActivity() {

    private var item by mutableStateOf<MediaItem?>(null)
    private var rating by mutableStateOf<Pair<String, Float>?>(null)
    private val tmdb by lazy { com.kivo.cinema.tmdb.TmdbClient(this) }
    private val privStore by lazy { com.kivo.cinema.privatezone.PrivateStore(this) }
    private var privEpoch by mutableStateOf(0)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val path = intent.getStringExtra("kivo.media") ?: ""
        item = queryItem(path)
        setContent { DetailScreen() }
        val it0 = item
        Thread {
            it0?.let { i ->
                val r = tmdb.ratingFor(i.title, null)
                rating = r?.let { it.source to it.score }
            }
        }.start()
    }

    private fun queryItem(path: String): MediaItem? {
        val projection = arrayOf(
            MS.Video.Media._ID, MS.Video.Media.DISPLAY_NAME, MS.Video.Media.DATA,
            MS.Video.Media.DURATION, MS.Video.Media.WIDTH, MS.Video.Media.HEIGHT,
            MS.Video.Media.DATE_ADDED,
        )
        runCatching {
            contentResolver.query(
                MS.Video.Media.EXTERNAL_CONTENT_URI, projection, null, null, null
            )?.use { c: Cursor ->
                while (c.moveToNext()) {
                    if ((c.getString(2) ?: "") == path) {
                        return MediaItem(
                            id = c.getLong(0), title = c.getString(1) ?: "",
                            path = path, durationMs = c.getLong(3),
                            width = c.getInt(4), height = c.getInt(5),
                            addedSec = c.getLong(6),
                        )
                    }
                }
            }
        }
        // MediaStore 外的文件（推送的测试片等）：用传入字段兜底
        val name = path.substringAfterLast('/')
        val dur = intent.getLongExtra("kivo.durationMs", 0L)
        return MediaItem(id = 0, title = name, path = path, durationMs = dur,
            width = 0, height = 0, addedSec = 0)
    }

    @Composable
    private fun DetailScreen() {
        val it0 = item ?: return
        AnimatedGradientBg {
            Column(Modifier.fillMaxSize()) {
                // hero：缩略图全出血 + 底部渐变 + 左上返回
                Box(
                    Modifier
                        .fillMaxWidth()
                        .height(260.dp)
                ) {
                    val bmp = remember(it0.id) {
                        runCatching {
                            android.media.ThumbnailUtils.createVideoThumbnail(
                                it0.path, MS.Video.Thumbnails.MINI_KIND)
                        }.getOrNull()
                    }
                    if (bmp != null) {
                        Image(bmp.asImageBitmap(), it0.title,
                            contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
                    } else {
                        Box(Modifier.fillMaxSize().background(Color(0xFF24242A)))
                    }
                    Box(
                        Modifier
                            .fillMaxSize()
                            .background(Brush.verticalGradient(
                                listOf(Color.Transparent, Color.Black.copy(alpha = 0.55f))))
                    )
                    GlassSurface(
                        modifier = Modifier
                            .padding(13.dp)
                            .size(DesignTokens.circleButton.dp)
                            .clip(CircleShape)
                            .clickable { finish() },
                        radius = DesignTokens.circleButton,
                    ) {
                        Box(contentAlignment = Alignment.Center) {
                            Text("‹", fontSize = 22.sp, color = DesignTokens.textMain)
                        }
                    }
                    Text(it0.title, fontSize = 24.sp, fontWeight = FontWeight.Bold,
                        color = Color.White, maxLines = 1,
                        modifier = Modifier
                            .align(Alignment.BottomStart)
                            .padding(13.dp))
                }
                Spacer(Modifier.height(12.dp))
                Column(Modifier.padding(horizontal = DesignTokens.screenPad.dp)) {
                    // 元信息行（帧语义：类型·日期；本地数据 = 分辨率·日期）
                    Text("${it0.width}×${it0.height} · ${fmtDate(it0.addedSec)} · ${it0.formatDuration()}",
                        fontSize = 15.sp, color = DesignTokens.textSub)
                    Spacer(Modifier.height(8.dp))
                    RatingBadges(if (rating != null) listOf(rating!!) else emptyList())
                    Spacer(Modifier.height(8.dp))
                    // 简介位（TMDB 简介 v2 接入——明示占位）
                    Text("简介：接入 TMDB 数据源后自动补全（237 方案 F2 v2）。",
                        fontSize = 14.sp, color = DesignTokens.textSub, lineHeight = 20.sp)
                    Spacer(Modifier.height(16.dp))
                    // 主操作：直接播放（全 app 唯一强 tint 处，参照规格语义）
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .height(50.dp)
                            .clip(RoundedCornerShape(13.dp))
                            .background(DesignTokens.accentGreen)
                            .clickable {
                                startActivity(android.content.Intent(
                                    this@DetailActivity,
                                    com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
                                    .putExtra("kivo.media", it0.path))
                            },
                        contentAlignment = Alignment.Center
                    ) {
                        Text("▶  直接播放", fontSize = 17.sp, fontWeight = FontWeight.Bold,
                            color = Color.White)
                    }
                    Spacer(Modifier.height(GAP))
                    // 次操作：私密区标记（231）
                    val isPriv = remember(privEpoch) { privStore.contains(it0.id.toString()) }
                    GlassSurface(Modifier.fillMaxWidth().clickable {
                        if (isPriv) privStore.remove(it0.id.toString())
                        else privStore.add(it0.id.toString())
                        privEpoch++
                    }) {
                        Text(
                            if (isPriv) "移出私密区" else "加入私密区",
                            fontSize = 15.sp, color = DesignTokens.textMain,
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 14.dp),
                            textAlign = androidx.compose.ui.text.style.TextAlign.Center,
                        )
                    }
                    Spacer(Modifier.height(8.dp))
                    Text("简介/演员/相似作品位随 TMDB 数据源扩展（237 §六）。",
                        fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub)
                }
            }
        }
    }

    private companion object {
        val GAP = 10.dp
        fun fmtDate(addedSec: Long): String {
            val d = java.util.Calendar.getInstance()
            d.timeInMillis = addedSec * 1000L
            return "%04d-%02d-%02d".format(d.get(java.util.Calendar.YEAR),
                d.get(java.util.Calendar.MONTH) + 1, d.get(java.util.Calendar.DAY_OF_MONTH))
        }
    }
}
