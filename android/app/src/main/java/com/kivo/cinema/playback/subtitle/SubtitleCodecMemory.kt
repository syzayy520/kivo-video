package com.kivo.cinema.playback.subtitle

import android.content.Context

/**
 * **按字幕文件**记忆用户手选的编码。
 *
 * 【为什么按文件而不按片源】编码是"这一份字幕字节是哪种字符集"，属于文件属性；
 * 按片源记会让同片源的另一条外挂轨被套上别人的编码。键 = 库内文件名去掉扩展名
 * （`<hash12>-<name>`，内容哈希 ⇒ 同一份字幕重导/重开都能读回，换字幕不串味）。
 *
 * 【存储】SharedPreferences 字符串；`"auto"` = 用户主动回到自动探测，等价于没记过，
 * 一律 [clear]，永不落盘（否则下次会被当成"指定了 auto"读出来）。
 */
internal class SubtitleCodecMemory(context: Context) {

    private val sp = context.applicationContext
        .getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    /** 该字幕记住的编码；没记过或键为空 ⇒ null（= 走自动探测）。 */
    fun load(fileKey: String): String? {
        val key = fileKey.takeIf { it.isNotBlank() } ?: return null
        return sp.getString(key, null)
    }

    /** 记住该字幕的编码。`"auto"` / 空 ⇒ 清除记忆。 */
    fun save(fileKey: String, codec: String) {
        val key = fileKey.takeIf { it.isNotBlank() } ?: return
        if (codec.isBlank() || codec == AUTO) sp.edit().remove(key).apply()
        else sp.edit().putString(key, codec).apply()
    }

    /** 忘掉该字幕的编码。空键直接忽略。 */
    fun clear(fileKey: String) {
        val key = fileKey.takeIf { it.isNotBlank() } ?: return
        sp.edit().remove(key).apply()
    }

    private companion object {
        const val PREFS = "subtitle_codec"
        const val AUTO = "auto"
    }
}