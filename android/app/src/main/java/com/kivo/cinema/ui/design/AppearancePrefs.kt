package com.kivo.cinema.ui.design

import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.SharedPreferences
import android.os.BatteryManager
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

/**
 * 外观与交互策略（267）：observable 单例，设置页写、首页/详情/播放器读。
 * 默认值＝已验收基线（剧照/家长指南现显、nav 现有标签、触感现有震动）。
 * 单职责：策略读写 + 触感可用性判定；不画任何 UI。
 */
object AppearancePrefs {
    private var sp: SharedPreferences? = null

    var showStills by mutableStateOf(true)
        private set
    var showContentNotes by mutableStateOf(true)
        private set
    var hideNavLabels by mutableStateOf(false)
        private set
    var hapticMode by mutableStateOf("auto")
        private set

    /** 幂等绑定：进程内首次读盘；此后所有写都经本对象，状态与盘一致。 */
    fun bind(context: Context) {
        if (sp != null) return
        val p = context.getSharedPreferences("appearance_interaction", Context.MODE_PRIVATE)
        sp = p
        showStills = p.getBoolean("show_stills", true)
        showContentNotes = p.getBoolean("show_content_notes", true)
        hideNavLabels = p.getBoolean("hide_nav_labels", false)
        hapticMode = p.getString("haptic_mode", "auto") ?: "auto"
    }

    fun updateShowStills(value: Boolean) {
        showStills = value
        sp?.edit()?.putBoolean("show_stills", value)?.apply()
    }

    fun updateShowContentNotes(value: Boolean) {
        showContentNotes = value
        sp?.edit()?.putBoolean("show_content_notes", value)?.apply()
    }

    fun updateHideNavLabels(value: Boolean) {
        hideNavLabels = value
        sp?.edit()?.putBoolean("hide_nav_labels", value)?.apply()
    }

    fun updateHapticMode(value: String) {
        hapticMode = value
        sp?.edit()?.putString("haptic_mode", value)?.apply()
    }

    /** 触感可用性：禁用＝关；自动＝系统报低电量时关（不自设阈值）；始终＝开。 */
    fun hapticAllowed(context: Context): Boolean {
        bind(context)
        return when (hapticMode) {
            "always" -> true
            "disabled" -> false
            else -> !batteryLow(context)
        }
    }

    private fun batteryLow(context: Context): Boolean {
        val intent = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
        return intent?.getBooleanExtra(BatteryManager.EXTRA_BATTERY_LOW, false) ?: false
    }
}