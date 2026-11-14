package com.kivo.cinema.ui.detail

import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.lazy.LazyListScope
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.AppearancePrefs

/**
 * 详情页下半屏的章节列表（从 DetailPage 抽出，控行数）。
 * 唯一职责 = 把详情字段铺成列表条目（出品方/演职员/家长指南/分类/预告片/剧照/相似作品）；
 * 每节有数据才渲染（不渲染空壳），字段拼装仍归详情域。
 */
internal fun LazyListScope.detailLowerSections(
    d: TmdbDetail.Details,
    fg: Color,
    fgSub: Color,
    tmdbIsTv: Boolean,
    loader: (suspend (String) -> Bitmap?)?,
    openPage: (Intent) -> Unit,
) {
    d.studioLogo?.let { url ->
        item(key = "studio") { Column(Modifier.fillMaxWidth()) { StudioLogo(url, fg) } }
    }
    if (d.cast.isNotEmpty()) item(key = "cast") {
        Column(Modifier.fillMaxWidth()) {
            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
            SectionHeader("演职员", fg)
            Spacer(Modifier.height(14.dp))
            CastRow(d.cast, fg, fgSub, loader)
        }
    }
    if (AppearancePrefs.showContentNotes &&
        (d.certifications.isNotEmpty() || d.contentNotes.isNotEmpty())) {
        item(key = "guide") {
            Column(Modifier.fillMaxWidth()) {
                Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                SectionHeader("家长指南", fg)
                Spacer(Modifier.height(20.dp))
                ParentalGuide(d.certifications, d.contentNotes)
            }
        }
    }
    if (d.genres.isNotEmpty()) item(key = "genres") {
        Column(Modifier.fillMaxWidth()) {
            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
            SectionHeader("分类", fg)
            Spacer(Modifier.height(12.dp))
            GenreChips(d.genres)
        }
    }
    d.trailerKey?.let { key ->
        item(key = "trailer") {
            Column(Modifier.fillMaxWidth()) {
                Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
                SectionHeader("预告片 · YouTube", fg)
                Spacer(Modifier.height(12.dp))
                TrailerCard(key, "预告片", onClick = {
                    openPage(Intent(Intent.ACTION_VIEW,
                        Uri.parse("https://www.youtube.com/watch?v=$key")))
                })
            }
        }
    }
    if (AppearancePrefs.showStills && d.stills.isNotEmpty()) item(key = "stills") {
        Column(Modifier.fillMaxWidth()) {
            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
            SectionHeader("剧照", fg)
            Spacer(Modifier.height(12.dp))
            StillsRow(d.stills)
        }
    }
    if (d.similar.isNotEmpty()) item(key = "similar") {
        val context = LocalContext.current
        Column(Modifier.fillMaxWidth()) {
            Spacer(Modifier.height(DETAIL_SECTION_GAP_DP.dp))
            SectionHeader("相似作品", fg)
            Spacer(Modifier.height(10.dp))
            SimilarList(d.similar.filter { it.id > 0 && it.id != d.id }.take(8), fg, fgSub,
                onOpen = { similar ->
                    openPage(Intent(context, MovieDetailActivity::class.java)
                        .putExtra(MovieDetailActivity.EXTRA_ID, similar.id)
                        .putExtra(MovieDetailActivity.EXTRA_TV, tmdbIsTv))
                })
        }
    }
}
