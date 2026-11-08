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
        val backdropUrl: String?,  // w780 横版剧照（幕布主图：不含片名，片名由 logo 图层承担）
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
                            cast.add(CastMember(n, "导演", p?.let { "${TmdbHome.IMG}/w185$it" }))
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
                    profile?.let { "${TmdbHome.IMG}/w185$it" }))
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
        val posterPath = json.optString("poster_path").takeIf { it.isNotEmpty() }
        val backdropPath = json.optString("backdrop_path").takeIf { it.isNotEmpty() }
        return Details(
            id = id,
            title = title,
            date = date,
            genres = genres,
            overview = json.optString("overview", ""),
            rating = json.optDouble("vote_average", 0.0).toFloat(),
            // 幕布优先竖版海报（240 §5.4：海报自带标题烙印），无海报才退横版剧照
            posterUrl = posterPath?.let { "${TmdbHome.IMG}/w780$it" }
                ?: backdropPath?.let { "${TmdbHome.IMG}/w780$it" } ?: "",
            backdropUrl = backdropPath?.let { "${TmdbHome.IMG}/w780$it" },
            heroArtworkUrl = artworkOf(json, posterPath, backdropPath),
            logoUrl = logoOf(json),
            isTv = isTv,
            region = regionOf(json),
            episodes = if (isTv) json.optInt("number_of_episodes").takeIf { it > 0 } else null,
            runtimeMin = if (isTv) null else json.optInt("runtime").takeIf { it > 0 },
            trailerKey = trailerOf(json),
            cast = cast,
            stills = stills,
            studioLogo = studioOf(json),
            certifications = certificationsOf(json),
            contentNotes = contentNotesOf(json),
            similar = similar,
        )
    }

    /** 地区：production_countries 的 ISO 码译中文，最多两国以「/」连接。 */
    private fun regionOf(json: JSONObject): String? {
        val codes = mutableListOf<String>()
        json.optJSONArray("production_countries")?.let { arr ->
            for (i in 0 until arr.length().coerceAtMost(3)) {
                val c = arr.getJSONObject(i).optString("iso_3166_1")
                if (c.isNotEmpty()) codes.add(c)
            }
        }
        if (codes.isEmpty()) {
            json.optJSONArray("origin_country")?.let { arr ->
                for (i in 0 until arr.length().coerceAtMost(3)) {
                    val c = arr.optString(i)
                    if (c.isNotEmpty()) codes.add(c)
                }
            }
        }
        if (codes.isEmpty()) return null
        return codes.joinToString("/") { REGION_ZH[it] ?: it }
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
        return (neutral ?: backdrop ?: poster)?.let { "${TmdbHome.IMG}/w780$it" }.orEmpty()
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
        return picked.take(2).map { (REGION_ZH[it.first] ?: it.first) to it.second }
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
                    if (raw.isNotEmpty()) out.add(descriptorZh(raw))
                }
            }
        }
        return out.toList()
    }

    /** 描述符中文化：先精确匹配，再按关键词包含匹配，最后保留原文。 */
    private fun descriptorZh(raw: String): String {
        val k = raw.lowercase()
        DESCRIPTOR_ZH[k]?.let { return it }
        val rules = listOf(
            "language" to "不当用语", "violence" to "暴力", "injur" to "受伤",
            "blood" to "血腥", "gore" to "血腥", "nudit" to "裸露", "sex" to "性",
            "drug" to "毒品", "alcohol" to "酒精", "smok" to "吸烟",
            "horror" to "恐怖", "fear" to "惊吓", "threat" to "威胁",
        )
        for ((key, zh) in rules) if (k.contains(key)) return zh
        return raw
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

    private companion object {
        /** ISO 3166-1 → 中文地区名（地区标注规范：港澳台带中国前缀）。 */
        val REGION_ZH = mapOf(
            "CN" to "中国大陆", "HK" to "中国香港", "TW" to "中国台湾", "MO" to "中国澳门",
            "US" to "美国", "GB" to "英国", "JP" to "日本", "KR" to "韩国",
            "FR" to "法国", "DE" to "德国", "IT" to "意大利", "ES" to "西班牙",
            "CA" to "加拿大", "AU" to "澳大利亚", "NZ" to "新西兰", "RU" to "俄罗斯",
            "IN" to "印度", "TH" to "泰国", "SG" to "新加坡", "MY" to "马来西亚",
            "ID" to "印度尼西亚", "PH" to "菲律宾", "VN" to "越南", "TR" to "土耳其",
            "BR" to "巴西", "MX" to "墨西哥", "AR" to "阿根廷", "CL" to "智利",
            "CO" to "哥伦比亚", "SE" to "瑞典", "NO" to "挪威", "DK" to "丹麦",
            "FI" to "芬兰", "NL" to "荷兰", "BE" to "比利时", "PL" to "波兰",
            "CZ" to "捷克", "HU" to "匈牙利", "GR" to "希腊", "IE" to "爱尔兰",
            "AT" to "奥地利", "CH" to "瑞士", "PT" to "葡萄牙", "UA" to "乌克兰",
            "RO" to "罗马尼亚", "IL" to "以色列", "ZA" to "南非", "EG" to "埃及",
            "IR" to "伊朗", "IS" to "冰岛", "LT" to "立陶宛", "EE" to "爱沙尼亚",
        )

        /** 分级描述符 → 中文（TMDB 仅德/英等少量条目提供）。 */
        val DESCRIPTOR_ZH = mapOf(
            "violence" to "暴力", "injury" to "受伤", "blood" to "血腥",
            "language" to "粗口", "nudity" to "裸露", "sex" to "性",
            "drugs" to "毒品", "alcohol" to "酒精", "horror" to "恐怖",
            "fear" to "惊吓", "smoking" to "吸烟", "gore" to "血腥",
        )
    }
}
