package com.kivo.cinema.media.marks

data class FilmKey(val id: Int, val isTv: Boolean) {
    init { require(id > 0) { "影片 ID 无效" } }
    val storageKey: String get() = "${if (isTv) "tv" else "movie"}:$id"
}

enum class FilmMark { FAVORITE, WATCHLIST, WATCHED }

data class FilmMarks(
    val favorite: Boolean = false,
    val watchlist: Boolean = false,
    val watched: Boolean = false,
) {
    fun contains(mark: FilmMark): Boolean = when (mark) {
        FilmMark.FAVORITE -> favorite
        FilmMark.WATCHLIST -> watchlist
        FilmMark.WATCHED -> watched
    }

    fun with(mark: FilmMark, enabled: Boolean): FilmMarks = when (mark) {
        FilmMark.FAVORITE -> copy(favorite = enabled)
        FilmMark.WATCHLIST -> copy(watchlist = enabled)
        FilmMark.WATCHED -> copy(watched = enabled)
    }
}

data class FilmMarksSnapshot(
    val entries: Map<String, FilmMarks> = emptyMap(),
    val ready: Boolean = false,
    val error: String? = null,
) {
    operator fun get(key: FilmKey): FilmMarks = entries[key.storageKey] ?: FilmMarks()
}
