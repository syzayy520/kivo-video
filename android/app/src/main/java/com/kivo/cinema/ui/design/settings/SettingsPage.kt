package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.ui.design.home.HomePreviewSetting
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 设置根页（268 IA 重排）：空间占用 hero → 播放体验 2×2 入口 → 资源库概览双卡 → 使用偏好/安全关于分组 → 版本脚注。
 * 只组合行原语与入口卡，不堆业务；每行绑定真实持久化/真实导航。原 10 个入口全部保留（hero=缓存，grid=播放/音画/操控/AI，
 * 分组=autoplay/外观/数据/私密/关于），无对应后端者不放。autoplay 跨页状态由 MainActivity 持有，经 onPreviewChanged 回写。
 */
@Composable
fun SettingsPage(
    previewEnabled: Boolean,
    onPreviewChanged: (Boolean) -> Unit,
    onOpenSources: () -> Unit,
    onOpenPrivate: () -> Unit,
) {
    val context = LocalContext.current
    val store = remember { SettingsStore(context) }
    val repo = remember { SourceRepository(SourceStore(context)) }
    val imageBytes = remember { store.imageCacheBytes() }
    val ratingBytes = remember { store.ratingCacheBytes() }
    var embyCount by remember { mutableStateOf(0) }
    var smbCount by remember { mutableStateOf(0) }
    LaunchedEffect(Unit) {
        val records = withContext(Dispatchers.IO) { repo.read() }.getOrElse { emptyList() }
        val visible = records.filter { !it.hidden }
        embyCount = visible.count { !it.isSmb }
        smbCount = visible.count { it.isSmb }
    }
    SettingsNavHost(
        root = { push ->
            SettingsScaffold("设置") {
                SettingsStorageHero(imageBytes, ratingBytes) { push(SettingsRoute.CACHE) }
                Spacer(Modifier.height(SettingsVisuals.cardGap.dp))
                SettingsEntryGrid { push(it) }
                Spacer(Modifier.height(SettingsVisuals.cardGap.dp))
                SettingsStatusTiles(embyCount, smbCount) { push(SettingsRoute.DATA) }
                Spacer(Modifier.height(14.dp))
                SectionCard("使用偏好") {
                    HomePreviewSetting(previewEnabled, onPreviewChanged)
                    RowDivider()
                    ChevronRow("外观与交互", "主题 · 封面 · 预告片 · 触感 · 图标",
                        icon = SettingsIcons.brush, iconColor = SettingsVisuals.purple) { push(SettingsRoute.APPEARANCE) }
                    RowDivider()
                    ChevronRow("数据与资源", "语言 · 聚合 · 来源 · TMDB · 排序",
                        icon = SettingsIcons.drive, iconColor = SettingsVisuals.indigo) { push(SettingsRoute.DATA) }
                }
                Spacer(Modifier.height(SettingsVisuals.cardGap.dp))
                SectionCard("安全与关于") {
                    ChevronRow("应用内数字密码", "私密区 · 隐藏源 · 改密",
                        icon = SettingsIcons.lock, iconColor = SettingsVisuals.green, onClick = onOpenPrivate)
                    RowDivider()
                    ChevronRow("版本信息", "版本号 · 构建 · 私密安全",
                        icon = SettingsIcons.info, iconColor = SettingsVisuals.gray) { push(SettingsRoute.ABOUT) }
                }
                Spacer(Modifier.height(8.dp))
                VersionFootnote(store.versionLabel)
            }
        },
        sub = { route, back ->
            when (route) {
                SettingsRoute.PLAYBACK -> SettingsPlaybackSub(back)
                SettingsRoute.CONTROLS -> SettingsControlsSub(back)
                SettingsRoute.APPEARANCE -> SettingsAppearanceSub(previewEnabled, onPreviewChanged, back)
                SettingsRoute.AV -> SettingsAvSub(back)
                SettingsRoute.AI -> SettingsAiSub(back)
                SettingsRoute.DATA -> SettingsDataSub(store, onOpenSources, back)
                SettingsRoute.CACHE -> SettingsCacheSub(store, back)
                SettingsRoute.ABOUT -> SettingsAboutSub(store, onOpenPrivate, back)
            }
        },
    )
}