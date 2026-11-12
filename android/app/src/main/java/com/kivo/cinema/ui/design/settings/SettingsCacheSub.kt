package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 存储与缓存子页（261 设计 §二）：真实缓存字节 + 真实清理动作 + 确认弹窗。
 * 目标 = 图片磁盘缓存（cacheDir/tmdb-images-v2）+ 评分接口缓存（filesDir/tmdb_cache.json）；
 * 清理同时清进程内图缓存（TmdbImageCache.clearMemory），不触碰资源库/观看记录/密码。
 */
@Composable
internal fun SettingsCacheSub(store: SettingsStore, onBack: () -> Unit) {
    var imageBytes by remember { mutableStateOf(store.imageCacheBytes()) }
    var ratingBytes by remember { mutableStateOf(store.ratingCacheBytes()) }
    var confirm by remember { mutableStateOf(false) }
    var freed by remember { mutableStateOf<Long?>(null) }

    SubPageScaffold("存储与缓存", onBack) {
        SectionCard("缓存") {
            ValueRow("图片缓存", formatBytes(imageBytes), subtitle = "TMDB 海报/剧照磁盘缓存")
            ValueRow("评分接口缓存", formatBytes(ratingBytes), subtitle = "TMDB 评分结果缓存")
        }
        Spacer(Modifier.height(14.dp))
        SectionCard("清理") {
            Row(
                Modifier.fillMaxWidth().height(DesignTokens.rowHeight.dp)
                    .clickable(role = Role.Button) { confirm = true },
                horizontalArrangement = Arrangement.Center,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("清理缓存", fontSize = DesignTokens.fontRowTitle.sp,
                    color = DesignTokens.dangerRed, fontWeight = FontWeight.Medium)
            }
        }
        Footnote("清理会删除已缓存的海报与评分结果，下次浏览时重新加载；不影响资源库、观看记录与密码。")
        freed?.let {
            Spacer(Modifier.height(8.dp))
            Text("已释放 ${formatBytes(it)}", fontSize = 13.sp, color = DesignTokens.accentGreen,
                modifier = Modifier.padding(start = 4.dp))
        }
    }

    if (confirm) AlertDialog(
        onDismissRequest = { confirm = false },
        title = { Text("清理缓存？", fontSize = 17.sp) },
        text = { Text("将删除图片缓存与评分接口缓存，下次浏览重新加载。资源库、观看记录与密码不受影响。",
            fontSize = 14.sp, lineHeight = 20.sp) },
        confirmButton = {
            Text("清理", color = DesignTokens.dangerRed, fontSize = 15.sp,
                modifier = Modifier.clickable(role = Role.Button) {
                    val f = store.clearCaches()
                    imageBytes = store.imageCacheBytes()
                    ratingBytes = store.ratingCacheBytes()
                    freed = f
                    confirm = false
                }.padding(8.dp))
        },
        dismissButton = {
            Text("取消", color = DesignTokens.textSub, fontSize = 15.sp,
                modifier = Modifier.clickable(role = Role.Button) { confirm = false }.padding(8.dp))
        },
    )
}