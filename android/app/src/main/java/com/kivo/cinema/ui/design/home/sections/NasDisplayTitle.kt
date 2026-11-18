package com.kivo.cinema.ui.design.home.sections

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.NasTitleStore
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 卡片标题：优先 TMDB 的**当前语言译名**，拿不到就保持文件名解析结果。
 *
 * 行为分两段，都不阻塞渲染：
 * 1. 首帧同步读本地缓存 —— 命中就直接是中文（第二次冷启动起基本全命中）；
 * 2. 未命中则丢到 IO 线程解析，回来后重组成中文。
 *
 * 换不到中文时**保持原名**，不显示空白也不显示"未知"。
 */
@Composable
internal fun rememberNasTitle(entry: HomeEntry): String {
    val context = LocalContext.current
    // Emby 的标题已经由服务器按用户语言返回，不需要再搜一遍 TMDB。
    // 注意：判断放在内部而不是函数开头 return —— Composable 不能条件性跳过 remember。
    val fromEmby = entry.kind == HomeEntry.Kind.Emby
    var title by remember(entry.key) {
        mutableStateOf(
            if (fromEmby) entry.title
            else NasTitleStore.peek(context, entry.title, entry.year) ?: entry.title,
        )
    }
    LaunchedEffect(entry.key, entry.title, entry.year, fromEmby) {
        if (fromEmby) return@LaunchedEffect
        if (title == entry.title) {
            val localized = withContext(Dispatchers.IO) {
                NasTitleStore.resolve(context, entry.title, entry.year)
            }
            if (!localized.isNullOrBlank()) title = localized
        }
    }
    return title
}
