package com.kivo.cinema.ui.detail

import android.content.Intent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.evidence.IoTrace
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.library.tech.MediaTechInfo
import com.kivo.cinema.library.tech.MediaTechProbe
import com.kivo.cinema.privatezone.PrivateStore
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * D12 · 本地文件来源在 `DetailPage` 里的附加区块（旧 `DetailActivity` 独有能力的落点）。
 *
 * 只做"本地事实"：技术徽章/私密区/文件信息/技术明细/同目录。
 * 播放按钮、收藏/待看、相似作品由 `DetailPage` 主链路负责，不在这里重复。
 * 探测与同目录查询都在 IO 线程（MKV 线性扫不能进组合期）。
 */
@Composable
internal fun LocalDetailBlocks(
    facts: LocalFileFacts,
    fg: Color,
    fgSub: Color,
    openPage: (Intent) -> Unit,
) {
    val context = LocalContext.current
    val tech by produceState<MediaTechInfo?>(null, facts.path) {
        value = withContext(Dispatchers.IO) {
            IoTrace.measure("detail.localTech") { MediaTechProbe.probe(facts.path) }
        }
    }
    val related by produceState(emptyList<MediaItem>(), facts.path) {
        value = withContext(Dispatchers.IO) {
            IoTrace.measure("detail.localRelated") { queryRelatedVideos(context, facts.path) }
        }
    }
    Column(Modifier.fillMaxWidth().padding(horizontal = DesignTokens.screenPad.dp)) {
        if (tech != null) {
            Spacer(Modifier.height(8.dp))
            TechBadges(tech, fg = fgSub)
        }
        Spacer(Modifier.height(12.dp))
        PrivateToggleRow(facts.item, fg)
        Spacer(Modifier.height(12.dp))
        DetailFileInfoCard(
            width = facts.item.width,
            height = facts.item.height,
            durationMs = facts.item.durationMs,
            path = facts.item.path,
        )
        DetailTechRows(tech)
        DetailRelatedVideos(items = related) { target ->
            openPage(MovieDetailActivity.localIntent(context, target))
        }
        Spacer(Modifier.height(DesignTokens.screenPad.dp))
    }
}

/** 私密区键是 MediaStore `_ID`：推送进设备的文件没有稳定 id，诚实不渲染而不是写错键。 */
@Composable
private fun PrivateToggleRow(item: MediaItem, fg: Color) {
    if (item.id <= 0L) return
    val context = LocalContext.current
    val store = remember { PrivateStore(context) }
    var epoch by remember { mutableStateOf(0) }
    val inside = remember(epoch) { store.contains(item.id.toString()) }
    GlassSurface(
        Modifier.fillMaxWidth().clickable {
            if (inside) store.remove(item.id.toString()) else store.add(item.id.toString())
            epoch++
        },
    ) {
        Text(
            text = if (inside) "移出私密区" else "加入私密区",
            fontSize = 15.sp,
            color = fg,
            modifier = Modifier.fillMaxWidth().padding(vertical = 14.dp),
            textAlign = TextAlign.Center,
        )
    }
}
