package com.kivo.cinema.tmdb.artwork

import org.json.JSONObject

/** Shared hero artwork policy. Language-neutral metadata is a hint, not a text-free guarantee. */
internal object ArtworkSelection {
    fun portrait(images: JSONObject?): String? {
        val posters = images?.optJSONArray("posters") ?: return null
        return (0 until posters.length()).map { posters.getJSONObject(it) }
            .filter { it.isNull("iso_639_1") && it.optString("file_path").isNotEmpty() }
            .maxWithOrNull(compareBy<JSONObject> { it.optDouble("vote_average") }
                .thenBy { it.optInt("vote_count") })?.optString("file_path")
    }
}
