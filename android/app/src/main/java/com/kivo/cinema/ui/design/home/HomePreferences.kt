package com.kivo.cinema.ui.design.home

import android.content.Context

/** Autoplay is opt-in; absence of the preference always means images only. */
internal class HomePreferences(context: Context) {
    private val preferences = context.getSharedPreferences("home_display", Context.MODE_PRIVATE)
    var autoplay: Boolean
        get() = preferences.getBoolean("preview_autoplay", false)
        set(value) { preferences.edit().putBoolean("preview_autoplay", value).apply() }
}
