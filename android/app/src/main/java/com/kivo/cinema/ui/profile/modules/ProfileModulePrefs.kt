package com.kivo.cinema.ui.profile.modules

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.State
import androidx.compose.runtime.mutableStateOf
import com.kivo.cinema.ui.profile.model.ProfileModule

/**
 * D10 · 模块显隐真值（唯一持有者）。
 *
 * 单职责：把 [ProfileModule] 的开关状态落盘，并把「区块 key → 模块」映射提供给「我的」页。
 * 不画 UI、不取数、不复活已隔离组件。
 *
 * 默认值取 [ProfileModule.defaultOn]（当前 12 项全开 = 已验收基线）。
 * `bind` 在 MainActivity.onCreate 调用一次；页面只观察 [enabled]，切换经 [setEnabled] 同步内存与磁盘。
 *
 * 已知边界（如实记录）：`Subscription` / `Download` 两项在当前「我的」页**没有对应渲染区块**
 * （基准截图里也没有）。它们的开关会真实落盘并进入 [enabled]，但当前版本无 UI 可驱动；
 * 不新增区块（UI 已定稿），也不从列表删项（视觉基线不变）。
 */
object ProfileModulePrefs {
    private const val STORE = "profile_modules"
    private const val PREFIX = "module_"

    private var sp: SharedPreferences? = null
    private val _enabled = mutableStateOf<Set<ProfileModule>>(emptySet())

    /** Compose 直接 `by` 观察；切换后页面立即重组。 */
    val enabled: State<Set<ProfileModule>> get() = _enabled

    /** 有渲染区块的项才进映射；未列出的 item key（环境头/热门/管理胶囊）恒显。 */
    private val blockModules = mapOf(
        "week" to ProfileModule.Calendar,
        "today" to ProfileModule.Overview,
        "history-title" to ProfileModule.History,
        "history" to ProfileModule.History,
        "watchlist" to ProfileModule.Watchlist,
        "collections" to ProfileModule.Collection,
        "watching" to ProfileModule.Watching,
        "monthly" to ProfileModule.Monthly,
        "genre-title" to ProfileModule.Genre,
        "genres" to ProfileModule.Genre,
    )

    fun bind(context: Context) {
        if (sp != null) return
        val p = context.applicationContext.getSharedPreferences(STORE, Context.MODE_PRIVATE)
        sp = p
        _enabled.value = ProfileModule.values().filter { module ->
            p.getBoolean(PREFIX + module.name, module.defaultOn)
        }.toSet()
    }

    fun setEnabled(module: ProfileModule, value: Boolean) {
        _enabled.value = if (value) _enabled.value + module else _enabled.value - module
        sp?.edit()?.putBoolean(PREFIX + module.name, value)?.apply()
    }

    /** 区块显隐：无映射的 key 恒显，保证「管理模块」胶囊永远可点回来。 */
    fun visibleBlock(key: String): Boolean {
        val module = blockModules[key] ?: return true
        return module in _enabled.value
    }

    /** 趋势卡按数据顺序映射：0 → Trend1，1 → Trend2；更多条恒显。 */
    fun visibleTrend(index: Int): Boolean = when (index) {
        0 -> ProfileModule.Trend1 in _enabled.value
        1 -> ProfileModule.Trend2 in _enabled.value
        else -> true
    }
}
