package com.kivo.cinema.ui.design.home

import android.content.Context
import com.kivo.cinema.library.NasCategory

/** Autoplay is opt-in; absence of the preference always means images only. */
internal class HomePreferences(context: Context) {
    private val preferences = context.getSharedPreferences("home_display", Context.MODE_PRIVATE)
    var autoplay: Boolean
        get() = preferences.getBoolean("preview_autoplay", false)
        set(value) { preferences.edit().putBoolean("preview_autoplay", value).apply() }
    var previewSound: Boolean
        get() = preferences.getBoolean("preview_sound", false)
        set(value) { preferences.edit().putBoolean("preview_sound", value).apply() }

    /** 首页「本地片库」某个分类是否显示（默认见 [NasCategory.defaultVisible]）。 */
    fun categoryVisible(category: NasCategory): Boolean =
        preferences.getBoolean("nas_cat_${category.key}", category in NasCategory.defaultVisible)

    fun setCategoryVisible(category: NasCategory, visible: Boolean) {
        preferences.edit().putBoolean("nas_cat_${category.key}", visible).apply()
    }
}
