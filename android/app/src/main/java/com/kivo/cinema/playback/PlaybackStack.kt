package com.kivo.cinema.playback

import android.content.Context
import android.content.Intent

/**
 * 280 · **播放栈的唯一决策入口**。
 *
 * ## 为什么要有这个文件
 *
 * 在它之前，"这一场播放到底走哪条路"是**两个地方各算一遍**的：
 *
 * | 调用方 | 算什么 | 输入 |
 * | --- | --- | --- |
 * | `PlayerComposeActivity.prepareHdrSurface` | Surface 要不要配成 10bit | `intent.getStringExtra("kivo.renderMode")` |
 * | `MpvStartSequence.prepare` | 内核要不要 `egl-output-format=rgb10_a2` | `intentRenderMode()`（**还兼容 `kivo.gpuNext`**） |
 *
 * 两处的输入**不等价**，于是产生一种会咬人的组合：
 * `--ez kivo.gpuNext true`（277 时代的取证开关）+ 设置项 `renderMode=legacy`
 * ⇒ Activity 侧算成 LEGACY（Surface 留 **8bit**），
 *    内核侧算成 GPU_NEXT（要 **10bit** EGL config）。
 * 本机被 [Hdr10BitProbe] 能力闸门挡住所以没炸；在支持 10bit 的设备上，
 * 这正是"`ANativeWindow_setBuffersGeometry(10bit)` 卡死 VO 初始化"的形态。
 *
 * ## 职责边界（为什么不把三个开关合并进来）
 *
 * 本文件**只做聚合**，不做判断：
 * · [PlaybackEngine]  —— 选内核（mpv / legacy），读 `debug.kivo.engine`
 * · [PlaybackRenderMode] —— 选出画通路（gpu-next / render API），三层优先级
 * · [HdrOutputSwitch] —— 真 HDR 输出闸门（含设备能力探测）
 *
 * 四个文件各自单一职责；调用方**只认本文件的 [Decision]**，不许再自己拼。
 *
 * ## 用法
 *
 * ```kotlin
 * val stack = PlaybackStack.resolve(this, intent)   // 只算一次
 * prepareHdrSurface(holder, stack)                  // 用同一份
 * MpvStartSequence(..., stack).prepare(...)         // 还是同一份
 * ```
 */
object PlaybackStack {

    /**
     * 一场播放的完整技术选型。**一次算好，全程传递**。
     *
     * @param engine 内核（mpv / legacy）。legacy 时其余字段仅供日志参考。
     * @param renderMode 出画通路（[PlaybackRenderMode.GPU_NEXT] / [PlaybackRenderMode.LEGACY]）。
     * @param hdrOutput 是否尝试真 HDR 输出（10bit Surface + PQ）。
     *   已包含设备能力闸门：设备不支持时这里就是 false，调用方不必再判。
     * @param vo 交给 mpv 的 `vo` 值。由 [renderMode] 派生 ——
     *   放在这里是为了让调用方不必各写一遍 `if (...) "gpu-next" else "libmpv"`。
     */
    data class Decision(
        val engine: PlaybackEngine.Engine,
        val renderMode: String,
        val hdrOutput: Boolean,
        val vo: String,
    ) {
        /** 是否走 mpv 内核（false = 自研 legacy 内核，出画通路的选型无意义）。 */
        val usesMpv: Boolean get() = engine == PlaybackEngine.Engine.MPV

        /** 一行诊断（写 net-diag 用）。 */
        fun describe(): String =
            "内核=${if (usesMpv) "mpv" else "legacy"} 出画=$renderMode 真HDR=${if (hdrOutput) "开" else "关"}"
    }

    /**
     * 算一次决策。
     *
     * @param context 应用上下文（读 SharedPreferences / 系统属性）。**可以**是
     *   `applicationContext` —— 这里不读 intent。
     * @param intent 宿主 Activity 的 intent（可为 null）。**必须显式传入**：
     *   `MpvStartSequence` 之类的类内部持有的往往是 `applicationContext`，
     *   在里面 `(app as? Activity)?.intent` 恒为 null（这个坑已踩过两次）。
     */
    fun resolve(context: Context, intent: Intent?): Decision {
        val renderMode = PlaybackRenderMode.resolve(context, intentRenderMode(intent))
        return Decision(
            engine = PlaybackEngine.select(),
            renderMode = renderMode,
            hdrOutput = HdrOutputSwitch.isEnabled(context, renderMode),
            vo = if (renderMode == PlaybackRenderMode.GPU_NEXT) "gpu-next" else "libmpv",
        )
    }

    /**
     * 从 intent 读显式的出画通路覆盖（取证脚本用；null = 没指定）。
     *
     * 【兼容 277 时代】`--ez kivo.gpuNext true` 仍按 gpu-next 处理 ——
     * 那批取证脚本还在用，且它表达的意思与 `--es kivo.renderMode gpu-next` 相同。
     * **这条兼容只写在这里**，其它地方不许再各写一遍。
     */
    private fun intentRenderMode(intent: Intent?): String? {
        intent?.getStringExtra("kivo.renderMode")?.let { return it }
        if (intent?.getBooleanExtra("kivo.gpuNext", false) == true) {
            return PlaybackRenderMode.GPU_NEXT
        }
        return null
    }
}
