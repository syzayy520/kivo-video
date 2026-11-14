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
    /**
     * 274：**播放时用哪个 MediaSource 版本**（null = 不带参数，保持旧行为）。
     *
     * 【为什么与展示解耦】同一个条目常有 1080p / 4K 多个版本，
     * 4K 版本码率 6.51 MB/s 远超本机取数能力（1.10–2.16 MB/s）⇒ 播起来 1.5 fps。
     * 但详情页的技术规格仍取 `MediaSources[0]`（片源本来面目，不动 UI），
     * **只有播放 URL 换版本**。选源规则见 [EmbySourcePicker]。
     */
    val playMediaSourceId: String? = null,
    /**
     * 274：**播放那个版本的文件大小**（0 = 未知，回退到展示用的 `media.sizeBytes`）。
     *
     * 【为什么必须跟着版本走】`kivo.playSize` 用于起播时的片尾预取窗口计算；
     * 若用 4K 的总长（57.5 GB）去算 1080p（4.3 GB）的窗口，预取会落到片外。
     */
    val playSizeBytes: Long = 0L,
)