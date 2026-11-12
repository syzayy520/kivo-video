package com.kivo.cinema.ui.detail

import com.kivo.cinema.emby.EmbyDetails
import com.kivo.cinema.tmdb.TmdbDetail

/**
 * Emby 兜底（254 D2；255 接图）：无 TMDB key / 无匹配时，把 Emby 真值映射成详情数据域。
 *
 * 图（海报/幕布/logo/演职员照）挂 Emby URL，由详情页的 token 加载器按域路由取图
 * （[EmbyResource] loadImage）；无加载器时 TmdbImage 显占位，不假装能加载。
 * 评分用 CommunityRating（>0 才显徽章）；预告/剧照/分级/相似 TMDB 才有，兜底留空。
 */
internal object EmbyFallback {

    fun map(e: EmbyDetails, isTv: Boolean = false): TmdbDetail.Details = TmdbDetail.Details(
        id = 0,
        title = e.name,
        date = e.premiereDate,
        genres = e.genres,
        overview = e.overview,
        rating = e.rating?.toFloat() ?: 0f,
        ratingSource = "Emby",
        posterUrl = e.posterUrl,
        backdropUrl = e.backdropUrl.ifBlank { null },
        heroArtworkUrl = e.backdropUrl.ifBlank { e.posterUrl },
        logoUrl = e.logoUrl.ifBlank { null },
        isTv = isTv,
        region = e.countries.take(2).joinToString("/").ifBlank { null },
        episodes = null,
        runtimeMin = e.runtimeMinutes.toInt().takeIf { it > 0 },
        trailerKey = null,
        cast = e.people.take(12).map {
            TmdbDetail.CastMember(it.name, it.role.ifBlank { it.type }, it.imageUrl.ifBlank { null })
        },
        stills = emptyList(),
        studioLogo = null,
        certifications = emptyList(),
        contentNotes = emptyList(),
        similar = emptyList(),
    )
}