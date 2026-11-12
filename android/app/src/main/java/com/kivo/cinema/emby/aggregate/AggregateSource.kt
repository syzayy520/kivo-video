package com.kivo.cinema.emby.aggregate

import android.content.Context
import android.content.Intent
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.ui.detail.MovieDetailActivity

/**
 * 一个已连接的聚合源（256）：记录元信息 + client + auth，全内存态，token 绝不落盘。
 * 单一职责：持有"哪个服务器、用什么凭据"，并把结果条目导航到**该源**的详情页。
 */
class AggregateSource(
    val record: SourceRecord,
    val client: EmbyClient,
    val auth: EmbyClient.Auth,
) {
    val id: String get() = record.id
    val name: String get() = record.name

    fun detailIntent(context: Context, item: EmbyItem): Intent =
        MovieDetailActivity.embyIntent(context, id, item, auth.token, auth.userId, name)
}