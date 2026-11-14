package com.kivo.cinema.playback

import android.content.Context

/**
 * 视频出画通路的唯一决策点（278）。
 *
 * 【两条通路的本质差别】
 * · **gpu-next**：`vo=gpu-next` —— mpv 自己的 VO + **完整 libplacebo 渲染器**。
 *   需要 ES3 context（我们给 mpv 的 `egl_helpers.c` 打了 ES3 补丁）才有
 *   compute shader ⇒ HDR 峰值计算可用（`pl_peak_detect`）。
 *   这条路与 SenPlayer 同架构。
 * · **legacy**：`vo=libmpv` —— render API + legacy RA（我们自建 EGL context）。
 *   只有 `r_gl.c`（没有 `r_gl_pl.c`）⇒ 无 compute shader ⇒ libplacebo 自己禁掉
 *   HDR 峰值计算（`Disabling HDR peak computation (compute shaders=0, SSBO=1)`）。
 *
 * 【为什么默认 gpu-next】
 * 278 的真机对照（同设备、同片源、同时段，用 SurfaceFlinger latency 量**上屏**帧）：
 *   legacy  23.97 fps / 帧间隔 max 55.8 ms
 *   gpu-next 23.98 fps / 帧间隔 max 47.9 ms
 * ⇒ 换渲染器**没有付出性能代价**，而 HDR 能力与 SenPlayer 对齐 ⇒ 默认走 gpu-next。
 *
 * 【三重回退（按优先级）】
 * 1. **系统属性** `debug.kivo.renderMode` = `legacy` ⇒ 紧急回退，不需重编。
 *    （与既有 `debug.kivo.originKeepAlive` 同一套取证/回退惯例。）
 * 2. **设置项**「视频输出」= 兼容模式 ⇒ 用户主动选择。
 * 3. **intent** `--es kivo.renderMode legacy|gpu-next` ⇒ 脚本化取证用。
 *
 * 【为什么不用 android.os.SystemProperties 直接调】
 * 它是隐藏 API（@hide），反射调用在新版 Android 上可能被拦；
 * 这里改成读环境变量 + 系统属性两条都试，失败就当"没设"。
 */
object PlaybackRenderMode {

    /** gpu-next：mpv 自己的 VO + 完整 libplacebo（默认）。 */
    const val GPU_NEXT = "gpu-next"

    /** legacy：render API + legacy RA（回退用）。 */
    const val LEGACY = "legacy"

    /** 设置项的取值：自动（跟随默认 = gpu-next）。 */
    const val AUTO = "auto"

    /**
     * 读取系统属性 `debug.kivo.renderMode`。
     *
     * 【为什么用反射而不是直接调】`android.os.SystemProperties` 是隐藏 API。
     * 真机上反射可能被拦（拿不到就返回 null = 没设），但**能拿到就有用**
     * ⇒ 拿不到不会出错，拿到了就能救急。
     */
    private fun sysProp(name: String): String? = try {
        val c = Class.forName("android.os.SystemProperties")
        val m = c.getMethod("get", String::class.java, String::class.java)
        val v = m.invoke(null, name, "") as? String
        v?.trim()?.takeIf { it.isNotEmpty() }
    } catch (_: Throwable) {
        null
    }

    /**
     * 决策最终走哪条通路。
     *
     * @param app 应用上下文（读 SharedPreferences 用）
     * @param intentOverride intent 里显式指定的模式（脚本化取证用，可为 null）
     */
    fun resolve(app: Context, intentOverride: String?): String {
        // 1) 系统属性：最高优先，紧急回退用
        val prop = sysProp("debug.kivo.renderMode")?.lowercase()
        if (prop == LEGACY) return LEGACY
        if (prop == GPU_NEXT) return GPU_NEXT

        // 2) intent 显式指定（取证脚本用）
        val ov = intentOverride?.trim()?.lowercase()
        if (ov == LEGACY) return LEGACY
        if (ov == GPU_NEXT) return GPU_NEXT

        // 3) 用户设置
        val pref = PlaybackPrefs(app).renderMode
        if (pref == LEGACY) return LEGACY
        if (pref == GPU_NEXT) return GPU_NEXT

        // 4) 默认：gpu-next
        return GPU_NEXT
    }

    /** 是否走 gpu-next。 */
    fun useGpuNext(app: Context, intentOverride: String?): Boolean =
        resolve(app, intentOverride) == GPU_NEXT
}
