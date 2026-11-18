package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.evidence.IoTrace
import com.kivo.cinema.sources.SourceRepository
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.library.NasCategory
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.home.HomePreferences
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 设置根页（B-3c 行式化）：REX `0374` 的「页面大标题 + 分组白卡 + 46 行」。
 *
 * 入口一个不少（改前 12 / 改后 12，逐一对应）：空间占用、播放、音画、操控、AI 字幕、
 * Emby、SMB、首页预告片自动播放、外观与交互、数据与资源、应用内数字密码、版本信息。
 * 路由与开关回调沿用 268 的既有实现，本卡只换呈现（宫格/hero 卡 → 分组行），不动跳转。
 *
 * 背景：统一冷灰画布 [DesignTokens.pageBackground]（#F2F2F7），与「我的」页同款。
 * 曾采样 REX 铺过粉紫渐变，但那与 [SettingsVisuals] 的「拒粉紫脏底」v3 裁决冲突，已撤除。
 * 深色态未接（全项目暂无 isSystemInDarkTheme 分支）。
 *
 * 偏离：宫格（[SettingsEntryGrid]）/资源卡（[SettingsStatusTiles]）/存储 hero
 * （[SettingsStorageHero]）不再被本页调用，文件保留不删（`SettingsEntryGrid` 按任务要求保留）；
 * 其承载的信息改为行的 `subtitle`/`trailing`（真实缓存字节、Emby/SMB 已配置计数）。
 * 「首页预告片自动播放」是开关而非子页，行式化后以 `trailing` 显示「开 / 关」、整行点击取反，
 * 回调仍是 `onPreviewChanged` 同一个写入路径。
 */
@Composable
fun SettingsPage(
    previewEnabled: Boolean,
    onPreviewChanged: (Boolean) -> Unit,
    onOpenSources: () -> Unit,
    onOpenPrivate: () -> Unit,
    /** 分类显示开关变更后通知宿重新算首页片库行（设置与首页同 Activity，不触发 onResume）。 */
    onCategoryChanged: () -> Unit = {},
) {
    val context = LocalContext.current
    val store = remember { SettingsStore(context) }
    val repo = remember { SourceRepository(SourceStore(context)) }
    val imageBytes by produceState(0L, store) {
        value = withContext(Dispatchers.IO) { IoTrace.measure("settings.imageBytes") { store.imageCacheBytes() } }
    }
    val ratingBytes by produceState(0L, store) {
        value = withContext(Dispatchers.IO) { IoTrace.measure("settings.ratingBytes") { store.ratingCacheBytes() } }
    }
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
            SettingsRootBody(
                previewEnabled = previewEnabled,
                onPreviewChanged = onPreviewChanged,
                onOpenPrivate = onOpenPrivate,
                cacheBytes = imageBytes + ratingBytes,
                embyCount = embyCount,
                smbCount = smbCount,
                version = store.versionLabel,
                push = push,
                onCategoryChanged = onCategoryChanged,
            )
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

/** 根页正文：34sp 大标题 + 五组行式白卡 + 版本脚注。数据全由参数传入，本函数不读 IO。 */
@Composable
private fun SettingsRootBody(
    previewEnabled: Boolean,
    onPreviewChanged: (Boolean) -> Unit,
    onOpenPrivate: () -> Unit,
    cacheBytes: Long,
    embyCount: Int,
    smbCount: Int,
    version: String,
    push: (SettingsRoute) -> Unit,
    onCategoryChanged: () -> Unit,
) {
    val context = LocalContext.current
    val homePrefs = remember { HomePreferences(context) }
    var visibleCats by remember {
        mutableStateOf(NasCategory.ordered.associateWith { homePrefs.categoryVisible(it) })
    }
    Box(Modifier.fillMaxSize().background(DesignTokens.pageBackground)) {
        Column(
            Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(horizontal = SettingsPageSidePad),
        ) {
            Spacer(Modifier.height(10.dp))
            Text(
                "设置", fontSize = DesignTokens.fontPageTitle.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
            )
            Spacer(Modifier.height(16.dp))
            SettingsGroupList(
                "播放体验",
                listOf(
                    SettingsRowSpec("播放") { push(SettingsRoute.PLAYBACK) },
                    SettingsRowSpec("音画") { push(SettingsRoute.AV) },
                    SettingsRowSpec("操控") { push(SettingsRoute.CONTROLS) },
                    SettingsRowSpec("AI 字幕") { push(SettingsRoute.AI) },
                ),
                footer = "调整缓冲、画质、音效与字幕偏好。",
            )
            Spacer(Modifier.height(SettingsGroupGap))
            SettingsGroupList(
                "资源库",
                listOf(
                    SettingsRowSpec("Emby", trailing = sourceLabel(embyCount)) { push(SettingsRoute.DATA) },
                    SettingsRowSpec("SMB", trailing = sourceLabel(smbCount)) { push(SettingsRoute.DATA) },
                ),
                footer = "管理已保存的连接与账号。",
            )
            Spacer(Modifier.height(SettingsGroupGap))
            SettingsGroupList(
                "使用偏好",
                listOf(
                    SettingsRowSpec(
                        title = "首页预告片自动播放",
                        subtitle = "默认显示图片；开启后需要能连接 YouTube",
                        trailing = if (previewEnabled) "开" else "关",
                    ) { onPreviewChanged(!previewEnabled) },
                    SettingsRowSpec("外观与交互") { push(SettingsRoute.APPEARANCE) },
                    SettingsRowSpec("数据与资源") { push(SettingsRoute.DATA) },
                ),
                footer = "开启预告片需连接 YouTube，否则默认显示封面图。",
            )
            Spacer(Modifier.height(SettingsGroupGap))
            SettingsGroupList(
                "安全与关于",
                listOf(
                    SettingsRowSpec("应用内数字密码", onClick = onOpenPrivate),
                    SettingsRowSpec("版本信息") { push(SettingsRoute.ABOUT) },
                ),
                footer = "私密区 · 隐藏源 · 应用安全。",
            )
            Spacer(Modifier.height(SettingsGroupGap))
            SettingsGroupList(
                "存储",
                listOf(
                    SettingsRowSpec("空间占用", trailing = formatBytes(cacheBytes)) {
                        push(SettingsRoute.CACHE)
                    },
                ),
                footer = "图片与评分缓存占用，可一键清理。",
            )
            Spacer(Modifier.height(SettingsGroupGap))
            SettingsGroupList(
                "首页本地片库",
                NasCategory.ordered.map { cat ->
                    SettingsRowSpec(
                        title = cat.label,
                        trailing = if (visibleCats[cat] == true) "开" else "关",
                    ) {
                        val next = !(visibleCats[cat] ?: false)
                        homePrefs.setCategoryVisible(cat, next)
                        visibleCats = visibleCats + (cat to next)
                        onCategoryChanged()
                    }
                },
                footer = "控制首页显示哪些分类；没有内容的分类自动不显示。",
            )
            Spacer(Modifier.height(8.dp))
            VersionFootnote(version)
            Spacer(Modifier.height(24.dp))
        }
    }
}

/** 源计数文案：与 268 概览卡同口径（只报「已配置」条数，不假装在线）。 */
private fun sourceLabel(count: Int): String = if (count > 0) "$count 个已配置" else "未配置"

private val SettingsPageSidePad = 14.dp
private val SettingsGroupGap = 32.dp

@Preview(name = "SettingsPage 分组行式", widthDp = 390, heightDp = 844)
@Composable
private fun SettingsPagePreview() {
    SettingsPage(
        previewEnabled = false,
        onPreviewChanged = {},
        onOpenSources = {},
        onOpenPrivate = {},
    )
}