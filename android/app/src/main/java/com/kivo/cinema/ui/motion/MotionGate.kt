package com.kivo.cinema.ui.motion

import android.content.Context
import android.provider.Settings
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.platform.LocalContext
import com.kivo.cinema.ui.design.AppearancePrefs

/**
 * 全局"减弱动画效果"门（竞品 G1）。
 *
 * 【为什么需要它】竞品 G1 有两个来源，缺一不可：
 * ① App 内显式开关（`settings.interface.page.reduceMotion`，"减弱动画效果"）；
 * ② 系统级减弱动画（Android 侧对应三个 animator scale，任一为 0 即视为"移除动画"）。
 * 两者取"或"：用户可能在系统里关、也可能只在 App 里关。
 *
 * 【约定】每个动效都必须有降级分支（位移/缩放/旋转归零，只保留 α 或瞬时切换）。
 * 门只回答"是否减弱"，具体怎么降级由各动效自己实现。
 * `reduce()` 是纯函数、不碰 Context，可被 JVM 单测直接覆盖。
 */
object MotionGate {

    /** 系统是否关闭/移除动画：三个 scale 任一为 0 即真（与 Android 无障碍语义一致）。 */
    fun systemReduced(context: Context): Boolean {
        val resolver = context.contentResolver
        return Settings.Global.getFloat(
            resolver, Settings.Global.ANIMATOR_DURATION_SCALE, 1f,
        ) == 0f || Settings.Global.getFloat(
            resolver, Settings.Global.TRANSITION_ANIMATION_SCALE, 1f,
        ) == 0f || Settings.Global.getFloat(
            resolver, Settings.Global.WINDOW_ANIMATION_SCALE, 1f,
        ) == 0f
    }

    /**
     * 合并 App 开关与系统开关——**取或**，与类注释"两者缺一不可"一致。
     *
     * 【为什么不是 App 优先】无障碍的系统设置在语义上是**硬约束**：
     * 用户在系统里开了"移除动画"，App 内的开关关着也必须减弱；
     * 旧实现 `false -> false` 让 App 关覆盖系统设置，违反无障碍契约（审查 P1-4）。
     * - `appEnabled == true` → 减弱（App 内显式开启）；
     * - `systemReduced == true` → 减弱（系统无障碍，无论 App 开关）；
     * - 两者都 false/null → 不减弱。
     */
    fun reduce(appEnabled: Boolean?, systemReduced: Boolean): Boolean =
        appEnabled == true || systemReduced

    /** Compose 侧入口：系统值只在组合建立时读一次，App 开关变化立即生效。 */
    @Composable
    fun rememberReduceMotion(appEnabled: Boolean?): Boolean {
        val context = LocalContext.current
        val system = remember(context) { systemReduced(context) }
        return reduce(appEnabled, system)
    }

    /**
     * 页面级便捷入口：App 侧取「外观与交互 › 减弱动画效果」。
     *
     * `AppearancePrefs.reduceMotion` 是 Compose State，改开关会直接触发重组合，
     * 不需要重启宿主；未 bind 时读默认 `false`（回落系统值）。
     */
    @Composable
    fun rememberAppReduceMotion(): Boolean =
        rememberReduceMotion(AppearancePrefs.reduceMotion)
}