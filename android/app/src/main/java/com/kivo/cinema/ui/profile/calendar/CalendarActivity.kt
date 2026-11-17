package com.kivo.cinema.ui.profile.calendar

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.core.view.WindowCompat

/**
 * 日历页外壳：只做「读 intent → 交给 [CalendarPage] → 返回即 finish」，
 * 没有任何可变字段，页面状态全部活在 [CalendarPage] 内部（与 MovieDetailActivity 同款壳/页分工）。
 *
 * 窗口沿用 MainActivity 的 edge-to-edge 口径（透明状态栏 + 透明导航栏），
 * 图标固定深色 —— 日历页底色是浅色画布（`DesignTokens.pageBackground`），不存在深色环境。
 *
 * 不做 FLAG_SECURE：日历只呈现观影统计，与「我的」页同级，不属于私密区
 * （私密区的截图保护在 `privatezone.PrivateActivity`）。
 */
class CalendarActivity : ComponentActivity() {

    companion object {
        /**
         * `yyyy-MM-dd`（与 `CalendarDay.date` 同格式）。缺省或非法值由 [CalendarPage] 自己回落到当天，
         * 本类不解析日期、不改写参数 —— 解析口径只允许存在一份。
         */
        const val EXTRA_DATE = "calendar.date"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        if (android.os.Build.VERSION.SDK_INT >= 29) window.isNavigationBarContrastEnforced = false
        window.navigationBarColor = android.graphics.Color.TRANSPARENT
        WindowCompat.getInsetsController(window, window.decorView).apply {
            isAppearanceLightStatusBars = true
            isAppearanceLightNavigationBars = true
        }
        val initialDate = intent.getStringExtra(EXTRA_DATE)
        setContent {
            CalendarPage(
                context = this,
                initialDate = initialDate,
                onBack = { finish() },
            )
        }
    }
}