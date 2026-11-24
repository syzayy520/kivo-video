package com.kivo.cinema.ui.subtitles.online

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.subtitle.online.OnlineSubtitle
import com.kivo.cinema.subtitle.online.SubSource
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.history.glassSurface

/** 在线字幕结果行：源/语言/格式/下载量 + 字幕名 + 下载与翻译动作。 */
@Composable
internal fun OnlineSubtitleResultList(
    items: List<OnlineSubtitle>,
    busyId: String?,
    onDownload: (OnlineSubtitle) -> Unit,
    onTranslate: (OnlineSubtitle) -> Unit,
) {
    LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        items(items, key = { "${it.source}:${it.id}" }) { item ->
            val busy = busyId == "${item.source}:${item.id}"
            Row(
                Modifier.fillMaxWidth()
                    .glassSurface(RoundedCornerShape(14.dp))
                    .padding(start = 12.dp, end = 12.dp, top = 10.dp, bottom = 10.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Column(Modifier.weight(1f)) {
                    Text(
                        item.title.ifBlank { item.releaseName },
                        color = DesignTokens.textMain, fontSize = 13.sp, fontWeight = FontWeight.Medium,
                        maxLines = 2,
                    )
                    Row(Modifier.padding(top = 4.dp), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        Text(sourceLabel(item.source), fontSize = 11.sp, color = DesignTokens.textSub)
                        Text("${item.languageLabel} · ${item.format}", fontSize = 11.sp, color = DesignTokens.textSub)
                        if (item.downloads > 0) Text("↓${item.downloads}", fontSize = 11.sp, color = DesignTokens.textSub)
                    }
                }
                Spacer(Modifier.size(8.dp))
                if (busy) {
                    CircularProgressIndicator(Modifier.size(20.dp), strokeWidth = 2.dp, color = DesignTokens.textSub)
                } else {
                    Text(
                        "下载", fontSize = 13.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain,
                        modifier = Modifier.clickable { onDownload(item) }.padding(horizontal = 8.dp, vertical = 6.dp),
                    )
                    if (item.languageLabel == "英语") {
                        Text(
                            "译成中文", fontSize = 13.sp, color = DesignTokens.textMain,
                            modifier = Modifier.clickable { onTranslate(item) }.padding(horizontal = 8.dp, vertical = 6.dp),
                        )
                    }
                }
            }
        }
    }
}

private fun sourceLabel(source: SubSource): String = when (source) {
    SubSource.OpenSubtitles -> "OpenSubtitles"
    SubSource.SubHd -> "SubHD"
}