package com.kivo.cinema.ui.detail

import android.content.Intent
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.tmdb.TmdbHome

/**
 * 336 R2 · 详情页预览数据（单职责：把 intent 带来的"列表页已有字段"翻译成详情头部数据）。
 *
 * 【为什么] 列表卡片本来就有标题/简介/评分/海报（TmdbHome.Item），点进详情时这些数据早在本机，
 * 但旧版 intent 只传 id ⇒ 详情页从零发网络请求，产生 0.4~0.6s 空白。这里让首帧就用已知数据渲染，
 * 完整详情（演员/剧照/相似/片源）再由 load() 后台补上并整体替换。
 *
 * 【边界] 预览体只填"列表快照里真实存在的字段"，没有的（片源匹配/剧照/演员）留空，
 * 不伪造、不降级提示；[preview] 标记让 DetailPage 暂不显示尚未解析的片源/下方节。
 */
internal object DetailPreview {

    const val EXTRA_TITLE = "kivo.pv.title"
    const val EXTRA_DATE = "kivo.pv.date"
    const val EXTRA_GENRES = "kivo.pv.genres"
    const val EXTRA_OVERVIEW = "kivo.pv.overview"
    const val EXTRA_RATING = "kivo.pv.rating"
    const val EXTRA_POSTER = "kivo.pv.poster"
    const val EXTRA_HERO = "kivo.pv.hero"
    const val EXTRA_BACKDROP = "kivo.pv.backdrop"
    const val EXTRA_REGION = "kivo.pv.region"
    const val EXTRA_SOURCE = "kivo.pv.source"   // "tmdb" / "emby" → ratingSource 徽章口径

    /** 列表项 → intent 附加字段（TMDB 首页/搜索）。幕布用 w780 竖版海报，与详情"优先竖版"一致。 */
    fun fill(i: Intent, item: TmdbHome.Item): Intent = i.apply {
        putExtra(EXTRA_TITLE, item.title)
        putExtra(EXTRA_DATE, item.date)
        putStringArrayListExtra(EXTRA_GENRES, ArrayList(item.genres))
        putExtra(EXTRA_OVERVIEW, item.overview)
        putExtra(EXTRA_RATING, item.rating)
        putExtra(EXTRA_POSTER, item.posterHeroUrl)
        putExtra(EXTRA_HERO, item.posterHeroUrl)
        putExtra(EXTRA_BACKDROP, item.backdropUrl)
        putExtra(EXTRA_SOURCE, "tmdb")
    }

    /**
     * 从 intent 还原预览详情体；**标题缺失**（如深链只带 id）时返回 null，调用方退回原加载流程。
     *
     * 【D366 为什么放宽幕布要求】原来要求「有幕布图」才给预览，于是**没有 Primary 图的条目**
     * （如「继续观看」里的单集）点进来就是纯转圈等 2 秒。而页面本来就有"暂无封面"的既有态
     * （`DetailBackdrop` 在 `heroArtworkUrl` 为空时用 field 色 + 一行"暂无封面"），
     * 所以只要有标题就先上屏 —— 用户先看到片名/简介/评分，完整详情后台替换，不再白屏。
     */
    fun from(i: Intent): TmdbDetail.Details? {
        val title = i.getStringExtra(EXTRA_TITLE)?.takeIf { it.isNotBlank() } ?: return null
        val hero = i.getStringExtra(EXTRA_HERO).orEmpty()
        val poster = i.getStringExtra(EXTRA_POSTER).orEmpty().ifBlank { hero }
        val date = i.getStringExtra(EXTRA_DATE).orEmpty()
        val genres = i.getStringArrayListExtra(EXTRA_GENRES) ?: emptyList()
        return TmdbDetail.Details(
            id = i.getIntExtra(MovieDetailActivity.EXTRA_ID, 0),
            title = title,
            date = date,
            genres = genres,
            overview = i.getStringExtra(EXTRA_OVERVIEW).orEmpty(),
            rating = i.getFloatExtra(EXTRA_RATING, 0f),
            posterUrl = poster,
            backdropUrl = i.getStringExtra(EXTRA_BACKDROP)?.takeIf { it.isNotBlank() },
            heroArtworkUrl = hero,
            logoUrl = null,
            isTv = i.getBooleanExtra(MovieDetailActivity.EXTRA_TV, false),
            region = i.getStringExtra(EXTRA_REGION)?.takeIf { it.isNotBlank() },
            episodes = null, runtimeMin = null, trailerKey = null, studioLogo = null,
            cast = emptyList(), stills = emptyList(), similar = emptyList(),
            certifications = emptyList(), contentNotes = emptyList(),
            ratingSource = if (i.getStringExtra(EXTRA_SOURCE) == "emby") "Emby" else "TMDB",
            posterWidth = 780, backdropWidth = 780,
        )
    }
}
