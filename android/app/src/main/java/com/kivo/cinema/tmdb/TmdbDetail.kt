package com.kivo.cinema.tmdb

import org.json.JSONObject
import com.kivo.cinema.tmdb.artwork.ArtworkSelection

/**
 * 影片详情数据域（240 §1/§5）：一次 append_to_response 拉详情+预告+演职员+相似+图片。
 * 全部 TMDB 公开 API，key/代理复用 TmdbClient 既有链路；网络失败返回 null（G9 由 UI 呈现）。
 *
 * 字段口径按 240 §5.2：meta 行 = 日期 · 地区 · 片长/集数（不含类型）；地区取 ISO 码译中文。
 */
class TmdbDetail(private val client: TmdbClient) {

    data class Details(
        val id: Int,
        val title: String,
        val date: String,          // yyyy-MM-dd，空=未定档（中文格式由 UI 转换）
        val genres: List<String>,  // 中文名（详情接口自带 zh-CN 名称）
        val overview: String,
        val rating: Float,
        val posterUrl: String,     // w780 竖版海报（**含烧死片名**，仅作无剧照时的兜底）
        val backdropUrl: String?,  // w1280 横版剧照（幕布主图：不含片名，片名由 logo 图层承担）
        val heroArtworkUrl: String, // Prefer language-neutral portrait artwork for the detail backdrop.
        val logoUrl: String?,      // 片名 logo（images.logos，zh 优先）
        val isTv: Boolean,
        val region: String?,       // 制作地区中文名，多国以「/」连接（「中国大陆/中国香港」）
        val episodes: Int?,        // 剧集集数（电影为 null）
        val runtimeMin: Int?,      // 片长分钟（剧集为 null）
        val trailerKey: String?,   // YouTube key（官方 Trailer，zh 优先）
        val cast: List<CastMember>,
        val stills: List<String>,  // 剧照 w780（/images backdrops）
        val studioLogo: String?,   // 片源/出品方 logo（production_companies.logo_path）
        val certifications: List<Pair<String, String>>, // 分级（地区中文名, 级别），最多 3
        val contentNotes: List<String>,                 // 内容提示（release_dates.descriptors 中文化）
        val similar: List<SimilarItem>,
        val ratingSource: String = "TMDB", // 徽章来源：兜底路径是 Emby CommunityRating，不得标 TMDB
        val originalLanguage: String = "", // 原产语言（zh/en/ko…）；logo 中文规则据此分流
        val zhLogoUrl: String? = null,     // 仅中文 logo（w500），无中文则 null
        val posterWidth: Int = 0,          // 选中海报有效像素宽（封顶 w780），供库图/TMDB 择优
        val backdropWidth: Int = 0,        // 选中幕布有效像素宽（封顶 w1280）
    )

    data class CastMember(val name: String, val role: String, val profileUrl: String?)

    data class SimilarItem(val id: Int, val title: String, val date: String,
                           val rating: Float, val posterUrl: String, val overview: String)

    fun details(isTv: Boolean, id: Int): Details? {
        val kind = if (isTv) "tv" else "movie"
        val json = get("/$kind/$id?language=zh-CN&" +
            "append_to_response=videos,credits,similar,images,release_dates" +
            "&include_image_language=zh,en,null").first ?: return null
        val title = json.optString("title", json.optString("name", ""))
        if (title.isEmpty()) return null
        val genres = json.optJSONArray("genres")?.let { arr ->
            (0 until arr.length()).map { arr.getJSONObject(it).optString("name") }
                .filter { it.isNotEmpty() }
        } ?: emptyList()
        val date = json.optString("release_date", json.optString("first_air_date", ""))
        val cast = mutableListOf<CastMember>()
        val credits = json.optJSONObject("credits")
        // 导演位（240 §5.2 E7 口径）：电影取 crew 的 Director，剧集取 created_by
        if (isTv) {
            json.optJSONArray("created_by")?.optJSONObject(0)?.optString("name")
                ?.takeIf { it.isNotEmpty() }
                ?.let { cast.add(CastMember(it, "导演", null)) }
        } else {
            credits?.optJSONArray("crew")?.let { arr ->
                for (i in 0 until arr.length()) {
                    val o = arr.getJSONObject(i)
                    if (o.optString("job") == "Director") {
                        val n = o.optString("name")
                        if (n.isNotEmpty()) {
                            val p = o.optString("profile_path").takeIf { it.isNotEmpty() }
                            cast.add(CastMember(n, "导演", p?.let { "${TmdbHome.IMG}/w300$it" }))
                        }
                        break
                    }
                }
            }
        }
        credits?.optJSONArray("cast")?.let { arr ->
            var added = 0
            for (i in 0 until arr.length()) {
                if (added >= 10) break
                val o = arr.getJSONObject(i)
                val name = o.optString("name")
                if (name.isEmpty() || cast.any { it.name == name }) continue
                val profile = o.optString("profile_path").takeIf { it.isNotEmpty() }
                cast.add(CastMember(name, o.optString("character", ""),
                    profile?.let { "${TmdbHome.IMG}/w300$it" }))
                added++
            }
        }
        val similar = mutableListOf<SimilarItem>()
        json.optJSONObject("similar")?.optJSONArray("results")?.let { arr ->
            for (i in 0 until arr.length()) {
                val o = arr.getJSONObject(i)
                val t = o.optString("title", o.optString("name", ""))
                val poster = o.optString("poster_path")
                if (t.isEmpty() || poster.isEmpty()) continue
                similar.add(SimilarItem(o.optInt("id"), t,
                    o.optString("release_date", o.optString("first_air_date", "")),
                    o.optDouble("vote_average", 0.0).toFloat().takeIf { it > 0f } ?: 0f,
                    "${TmdbHome.IMG}/w500$poster",
                    o.optString("overview", "")))
            }
        }
        val stills = mutableListOf<String>()
        json.optJSONObject("images")?.optJSONArray("backdrops")?.let { arr ->
            for (i in 0 until arr.length().coerceAtMost(10)) {
                val p = arr.getJSONObject(i).optString("file_path")
                if (p.isNotEmpty()) stills.add("${TmdbHome.IMG}/w780$p")
            }
        }
        val images = json.optJSONObject("images")
        val posterPick = bestImage(images?.optJSONArray("posters"), neutralFirst = false)
        val backdropPick = bestImage(images?.optJSONArray("backdrops"), neutralFirst = true)
        val posterPath = posterPick?.first ?: json.optString("poster_path").takeIf { it.isNotEmpty() }
        val backdropPath = backdropPick?.first ?: json.optString("backdrop_path").takeIf { it.isNotEmpty() }
        return Details(
            id = id,
            title = title,
            date = date,
            genres = genres,
            overview = json.optString("overview", ""),
            rating = json.optDouble("vote_average", 0.0).toFloat(),
            // 幕布优先竖版海报（240 §5.4：海报自带标题烙印），无海报才退横版剧照
            // 255 #30：语言+分辨率择优；original 在真机线路要数分钟，封顶实用档（海报 w780/幕布 w1280）
            posterUrl = posterPath?.let { "${TmdbHome.IMG}/w780$it" }
                ?: backdropPath?.let { "${TmdbHome.IMG}/w1280$it" } ?: "",
            backdropUrl = backdropPath?.let { "${TmdbHome.IMG}/w1280$it" },
            heroArtworkUrl = artworkOf(json, posterPath, backdropPath),
            logoUrl = logoOf(json),
            isTv = isTv,
            region = TmdbDetailLocale.regionOf(json),
            episodes = if (isTv) json.optInt("number_of_episodes").takeIf { it > 0 } else null,
            runtimeMin = if (isTv) null else json.optInt("runtime").takeIf { it > 0 },
            trailerKey = trailerOf(json),
            cast = cast,
            stills = stills,
            studioLogo = studioOf(json),
            certifications = certificationsOf(json),
            contentNotes = contentNotesOf(json),
            similar = similar,
            originalLanguage = json.optString("original_language"),
            zhLogoUrl = zhLogoOf(json),
            posterWidth = posterPick?.second?.coerceAtMost(780) ?: 0,
            backdropWidth = backdropPick?.second?.coerceAtMost(1280) ?: 0,
        )
    }

    /** 片源 logo：production_companies 里第一个带 logo 的公司（w300）。 */
    private fun studioOf(json: JSONObject): String? {
        val arr = json.optJSONArray("production_companies") ?: return null
        for (i in 0 until arr.length()) {
            val p = arr.getJSONObject(i).optString("logo_path")
            if (p.isNotEmpty()) return "${TmdbHome.IMG}/w300$p"
        }
        return null
    }

    /** Neutral portrait first; no title-specific IDs or image paths. */
    private fun artworkOf(json: JSONObject, poster: String?, backdrop: String?): String {
        val neutral = ArtworkSelection.portrait(json.optJSONObject("images"))
        return when {
            neutral != null -> "${TmdbHome.IMG}/w780$neutral"
            backdrop != null -> "${TmdbHome.IMG}/w1280$backdrop"
            poster != null -> "${TmdbHome.IMG}/w780$poster"
            else -> ""
        }
    }

    /** 255 #30：语言优先 + 同级像素宽最大。neutralFirst=true 用于幕布（避烧字），false 用于海报（中文优先）。 */
    private fun bestImage(arr: org.json.JSONArray?, neutralFirst: Boolean): Pair<String, Int>? {
        if (arr == null || arr.length() == 0) return null
        val items = (0 until arr.length()).map { arr.getJSONObject(it) }
            .filter { it.optString("file_path").isNotEmpty() }
        if (items.isEmpty()) return null
        val picked = items.sortedWith(compareBy<JSONObject> {
            val l = it.optString("iso_639_1")
            if (neutralFirst) when (l) { "null", "" -> 0; "zh" -> 1; "en" -> 2; else -> 3 }
            else when (l) { "zh" -> 0; "null", "" -> 1; "en" -> 2; else -> 3 }
        }.thenByDescending { it.optInt("width") }).first()
        return picked.optString("file_path") to picked.optInt("width")
    }

    /** 仅中文 logo（w500）；无中文 logo 返回 null，由调用方决定退路。 */
    private fun zhLogoOf(json: JSONObject): String? {
        val arr = json.optJSONObject("images")?.optJSONArray("logos") ?: return null
        val picked = (0 until arr.length()).map { arr.getJSONObject(it) }
            .filter { it.optString("iso_639_1") == "zh" &&
                it.optString("file_path").endsWith(".png", ignoreCase = true) }
            .maxByOrNull { it.optInt("width") } ?: return null
        return "${TmdbHome.IMG}/w500${picked.optString("file_path")}"
    }

    /** Transparent PNG logos, Chinese first, then neutral and English; votes break ties. */
    private fun logoOf(json: JSONObject): String? {
        val arr = json.optJSONObject("images")?.optJSONArray("logos") ?: return null
        val picked = (0 until arr.length()).map { arr.getJSONObject(it) }
            .filter { it.optString("file_path").endsWith(".png", ignoreCase = true) }
            .sortedWith(compareBy<JSONObject> {
                when (it.optString("iso_639_1")) { "zh" -> 0; "null", "" -> 1; "en" -> 2; else -> 3 }
            }.thenByDescending { it.optInt("vote_count") }.thenByDescending { it.optDouble("vote_average") })
            .firstOrNull() ?: return null
        return "${TmdbHome.IMG}/w500${picked.optString("file_path")}"
    }

    /** 分级：release_dates 的 certification，按常用地区优先取 3 个。 */
    private fun certificationsOf(json: JSONObject): List<Pair<String, String>> {
        val arr = json.optJSONObject("release_dates")?.optJSONArray("results") ?: return emptyList()
        val byIso = LinkedHashMap<String, String>()
        for (i in 0 until arr.length()) {
            val c = arr.getJSONObject(i)
            val iso = c.optString("iso_3166_1")
            if (iso.isEmpty() || byIso.containsKey(iso)) continue
            val rd = c.optJSONArray("release_dates") ?: continue
            for (j in 0 until rd.length()) {
                val cert = rd.getJSONObject(j).optString("certification").trim()
                if (cert.isNotEmpty()) { byIso[iso] = cert; break }
            }
        }
        if (byIso.isEmpty()) return emptyList()
        val order = listOf("US", "GB", "CN", "HK", "TW", "JP", "DE", "FR", "KR")
        val picked = order.filter { byIso.containsKey(it) }.map { it to byIso[it]!! }
            .ifEmpty { byIso.entries.take(3).map { it.key to it.value } }
        return picked.take(2).map { (TmdbDetailLocale.REGION_ZH[it.first] ?: it.first) to it.second }
    }

    /** 内容提示：release_dates.descriptors 去重中文化（TMDB 仅少量条目有）。 */
    private fun contentNotesOf(json: JSONObject): List<String> {
        val arr = json.optJSONObject("release_dates")?.optJSONArray("results") ?: return emptyList()
        val out = LinkedHashSet<String>()
        for (i in 0 until arr.length()) {
            val rd = arr.getJSONObject(i).optJSONArray("release_dates") ?: continue
            for (j in 0 until rd.length()) {
                val d = rd.getJSONObject(j).optJSONArray("descriptors") ?: continue
                for (k in 0 until d.length()) {
                    val raw = d.optString(k).trim()
                    if (raw.isNotEmpty()) out.add(TmdbDetailLocale.descriptorZh(raw))
                }
            }
        }
        return out.toList()
    }

    /** 预告：YouTube 官方 Trailer > Trailer > Teaser。 */
    private fun trailerOf(json: JSONObject): String? {
        val videos = json.optJSONObject("videos")?.optJSONArray("results") ?: return null
        var key: String? = null
        var best = Int.MAX_VALUE
        for (i in 0 until videos.length()) {
            val v = videos.getJSONObject(i)
            if (v.optString("site") != "YouTube") continue
            val rank = when {
                v.optString("type") == "Trailer" && v.optBoolean("official") -> 0
                v.optString("type") == "Trailer" -> 1
                v.optString("type") == "Teaser" -> 2
                else -> 3
            }
            if (rank < best && v.optString("key").isNotEmpty()) {
                best = rank
                key = v.optString("key")
            }
        }
        return key
    }

    private fun get(path: String): Pair<JSONObject?, String?> {
        if (client.apiKey().isEmpty()) return null to "offline"
        return try {
            val url = "${TmdbClient.API_BASE}$path" +
                (if (path.contains("?")) "&api_key=${client.apiKey()}"
                 else "?api_key=${client.apiKey()}")
            val conn = client.openConnection(url)
            conn.connectTimeout = 8000
            conn.readTimeout = 15000
            val code = conn.responseCode
            val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
                ?.bufferedReader()?.readText() ?: ""
            if (code !in 200..299) null to "server" else JSONObject(body) to null
        } catch (e: Exception) {
            null to "lost"
        }
    }
}
