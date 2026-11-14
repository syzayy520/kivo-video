package com.kivo.cinema.playback

import android.graphics.PixelFormat
import android.view.Surface
import android.view.SurfaceHolder
import com.kivo.cinema.mpv.MpvNative

/**
 * 278-P2 · **真 HDR 输出的 Android 侧准备**（Surface 的 buffer 格式与 dataspace）。
 *
 * ## 要解决什么
 *
 * 真 HDR 输出（面板收到 PQ/BT.2020 码值）要四层配合，缺一不可：
 *
 * | 层 | 谁做 | 内容 |
 * | --- | --- | --- |
 * | ① | **本文件** | Surface 的 buffer 格式 = **10bit**（`RGBA_1010102`） |
 * | ② | **本文件** | Surface 的 **dataspace** = `ADATASPACE_BT2020_PQ` |
 * | ③ | mpv（补丁） | EGL surface 的 colorspace = `EGL_GL_COLORSPACE_BT2020_PQ` |
 * | ④ | mpv（参数） | libplacebo 输出 PQ 码值（`target-trc=pq` + `target-colorspace-hint`） |
 *
 * ## 为什么 ①② 必须在 EGL 创建 surface **之前**做
 *
 * EGL 会按 Surface 当时的 buffer 格式挑 config 与渲染目标。
 * 格式定成 8bit 之后再改，EGL 仍按 8bit 走 ⇒ HDR 精度就丢了。
 * ⇒ 必须在 `attachSurface` 交给 mpv **之前**设好。
 *
 * ## 为什么不能无条件开 10bit
 *
 * 10bit surface 在部分设备/ROM 的 compositor 上不支持 ⇒ 会黑屏或走软解。
 * ⇒ **必须能退回 8bit**，且退回要**无声**（不崩、不黑屏）。
 * 本文件只负责"申请 + 记录实际结果"，真正能不能用由第 ③ 层（mpv 建 EGL surface）
 * 的成败决定 —— 失败时上层退回 8bit 重建（见 [HdrOutputPlan]）。
 *
 * ## 为什么用 native 的 `ANativeWindow_setBuffersDataSpace` 而不是 `Surface.setDataspace`
 *
 * · `Surface.setDataspace` 是 **API 33+**（我们 minSdk 26 ⇒ 编译不过）；
 * · `ANativeWindow_setBuffersDataSpace` 是 **API 28+**（NDK 稳定 API），
 *   且**正是 SenPlayer 用的那一个**（它的原生库里有这个符号）⇒ 与它对齐。
 *   实现在 `cpp/mpv/mpv_surface_dataspace.cpp`。
 *
 * ## 已知边界（不夸大）
 *
 * · 设备能力**未做运行时探测**：是否支持 10bit + PQ 由第 ③ 层的成败决定。
 * · HDR10+ / 杜比视界的**动态元数据**不在本文件范围（那是第 ④ 层与 ffmpeg 的事）。
 */
internal object HdrSurfaceConfig {

    /** 10bit RGBA（Android 侧 `PixelFormat.RGBA_1010102`）。 */
    private const val FORMAT_10BIT = PixelFormat.RGBA_1010102

    /**
     * `ADATASPACE_BT2020_PQ = 163971072`（android/data_space.h）。
     *
     * 【取值是实测出来的，不是猜的】候选全表探测（intent `--ei kivo.hdrProbe 1`，
     * 真机 K40 / Android 13）：
     * ```
     * UNKNOWN=0            : ok
     * SRGB=142671872       : ok
     * BT2020=147193856     : 失败(EINVAL)
     * BT2020_PQ=163971072  : **ok**   ← 用这个
     * BT2020_ITU_PQ=298188800 : 失败(EINVAL)
     * BT2020_HLG=168165376   : 失败(EINVAL)
     * P3=1432716183          : 失败(EINVAL)
     * ```
     * ⇒ 只有 **PQ** 这一个 HDR 传递函数被这台设备接受（HLG 不接受）。
     */
    private const val DATASPACE_BT2020_PQ = 163971072 // ADATASPACE_BT2020_PQ

    /** 8bit RGBA（退回用）。 */
    private const val FORMAT_8BIT = PixelFormat.RGBA_8888

    /** `ADATASPACE_SRGB = 142671872`（android/data_space.h）。 */
    private const val DATASPACE_SRGB = 142671872

    /**
     * 把 Surface 配成 **10bit + BT2020_PQ**。
     *
     * @return 实际设定的结果（供诊断留痕）；失败不抛异常（**不能因为 HDR 配不了就不播**）。
     */
    fun enableHdrSurface(holder: SurfaceHolder, surface: Surface? = null): String {
        val target = surface ?: runCatching { holder.surface }.getOrNull()
            ?: return "拿不到 Surface"
        return try {
            // ① buffer 格式：10bit
            //   【为什么必须在 EGL 建 surface 之前】EGL 会按当时的格式挑 config 与渲染目标；
            //   事后改就晚了（这正是我们实测到的：改完 format 后 SurfaceFlinger 显示
            //   req fmt:43 = RGBA_1010102，说明 mpv 的 EGL 尊重了这个格式）。
            holder.setFormat(FORMAT_10BIT)
            // ② dataspace：**放到 EGL 建完 surface 之后再设**（见 [applyDataspace]）。
            //   提前设会被内核拒（EINVAL）—— 因为那时 buffer 还是 8bit，
            //   8bit buffer 配 PQ dataspace 是非法组合。
            "已申请 10bit（dataspace 待 EGL 建面后设置）"
        } catch (t: Throwable) {
            "申请 HDR Surface 失败（${t.javaClass.simpleName}）⇒ 保持默认"
        }
    }

    /**
     * 278-P2 · 在 **EGL surface 建好之后**（buffer 已是 10bit）再写 dataspace。
     *
     * 【为什么必须分两步】真机实测（K40）：
     * · 10bit format + dataspace **同时**设 ⇒ dataspace 返回 **EINVAL(-22)**；
     *   原因是那时 EGL 还没建 surface，buffer 仍是 8bit，
     *   而 "8bit buffer + BT2020_PQ" 是非法组合。
     * · 先让 mpv 用 10bit config 建面（`egl-output-format=rgb10_a2`），
     *   再写 dataspace ⇒ 合法。
     *
     * @return 一句话结果（写进诊断）。
     */
    fun applyDataspace(surface: Surface?): String {
        val target = surface ?: return "无 Surface"
        val ds = setDataspace(target, DATASPACE_BT2020_PQ)
        return "dataspace(PQ)=$ds"
    }

    /**
     * 278-P2 · **一次性探测**：把候选 dataspace 逐个试一遍，看哪些被内核接受。
     *
     * 【为什么需要】`ANativeWindow_setBuffersDataSpace` 失败只给 EINVAL，
     * 不说"为什么不支持" ⇒ 只能挨个试。
     * 探测只在显式要求时跑（intent `--ei kivo.hdrProbe 1`）⇒ 线上零开销。
     *
     * 【候选来源】android/data_space.h 里与 HDR / 宽色域相关的常量。
     */
    fun probeDataspaces(surface: Surface?): String {
        val target = surface ?: return "无 Surface"
        val candidates = listOf(
            "UNKNOWN" to 0,
            "SRGB" to 142671872,
            "BT2020" to 147193856,
            "BT2020_PQ" to 163971072,
            "BT2020_ITU_PQ" to 298188800,
            "BT2020_HLG" to 168165376,
            "BT2020_ITU_HLG" to 302383104,
            "BT2020_LINEAR" to 281710848,
            "P3" to 1432716183,
            "SMPTE2084" to 163971072,
        )
        val results = candidates.map { (name, value) ->
            "$name=$value:${setDataspace(target, value)}"
        }
        return "探测：" + results.joinToString(" ")
    }

    /**
     * 把 Surface 退回 **8bit + 默认 dataspace**。
     *
     * 【什么时候用】第 ③ 层（mpv 建 EGL surface）失败，或用户选择"兼容模式"。
     */
    fun disableHdrSurface(holder: SurfaceHolder, surface: Surface? = null): String {
        val target = surface ?: runCatching { holder.surface }.getOrNull()
        return try {
            holder.setFormat(FORMAT_8BIT)
            val ds = if (target != null) setDataspace(target, DATASPACE_SRGB) else "无 Surface"
            "已退回 8bit + SRGB（dataspace=$ds）"
        } catch (t: Throwable) {
            "退回 8bit 失败（${t.javaClass.simpleName}）"
        }
    }

    /**
     * 写 dataspace。
     *
     * 【为什么要 try】`Surface.setDataspace` 在部分 ROM 上对不支持的组合会抛
     * `IllegalArgumentException` ⇒ 必须吞掉，否则整个起播挂掉。
     */
    private fun setDataspace(surface: Surface?, dataspace: Int): String {
        if (surface == null) return "无 Surface"
        return try {
            // native 版：API 28+ 可用，且与 SenPlayer 同一手段。
            val rc = MpvNative.nativeSetSurfaceDataspace(surface, dataspace)
            if (rc == 0) "ok" else "失败(rc=$rc)"
        } catch (t: Throwable) {
            "失败(${t.javaClass.simpleName})"
        }
    }
}
