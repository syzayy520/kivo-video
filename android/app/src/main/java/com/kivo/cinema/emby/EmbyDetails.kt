package com.kivo.cinema.emby

/** 254 详情页数据类：字段全来自 Emby REST 实测（4.8），取不到的留空不编造。 */

data class EmbyPerson(
    val id: String,
    val name: String,
    val role: String,
    val type: String,   // Director / Actor
    val imageUrl: String,
)

data class EmbyMediaInfo(
    val container: String,
    val sizeBytes: Long,
    val bitrate: Long,
    val width: Int,
    val height: Int,
    val video: List<String>,
    val audio: List<String>,
    val subtitles: List<String>,
)

data class EmbyDetails(
    val id: String,
    val name: String,
    val year: Int?,
    val overview: String,
    val premiereDate: String,
    val countries: List<String>,
    val runtimeMinutes: Long,
    val genres: List<String>,
    val rating: Double?,
    val studios: List<String>,
    val people: List<EmbyPerson>,
    val media: EmbyMediaInfo?,
    val posterUrl: String,
    val backdropUrl: String,
    val logoUrl: String,
    val isFavorite: Boolean,
    val played: Boolean,
)