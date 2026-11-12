package com.kivo.cinema.tmdb

import org.json.JSONObject

/**
 * 255 #30 从 TmdbDetail 抽出的本地化助手（单文件单职责，控行数）：
 * 地区 ISO 码译中文、分级描述符中文化。纯函数 + 译表，无网络、无 UI。
 */
internal object TmdbDetailLocale {

    /** 地区：production_countries 的 ISO 码译中文，最多两国以「/」连接。 */
    fun regionOf(json: JSONObject): String? {
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

    /** 描述符中文化：先精确匹配，再按关键词包含匹配，最后保留原文。 */
    fun descriptorZh(raw: String): String {
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
    private val DESCRIPTOR_ZH = mapOf(
        "violence" to "暴力", "injury" to "受伤", "blood" to "血腥",
        "language" to "粗口", "nudity" to "裸露", "sex" to "性",
        "drugs" to "毒品", "alcohol" to "酒精", "horror" to "恐怖",
        "fear" to "惊吓", "smoking" to "吸烟", "gore" to "血腥",
    )
}