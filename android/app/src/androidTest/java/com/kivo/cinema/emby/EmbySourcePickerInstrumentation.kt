package com.kivo.cinema.emby

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import android.util.Log
import com.kivo.cinema.playback.Av1HardwareProbe
import org.json.JSONArray
import org.json.JSONObject

/**
 * 274 验收：**选源逻辑**（[EmbySourcePicker]）。
 *
 * ## 为什么用 instrumentation 而不是 JVM 单测
 * 选源读的是 `org.json.JSONArray` —— JVM 单测里它是 stub，测不出真实行为；
 * 跑在设备上才有真正的实现。
 *
 * ## 用例（都来自真机数据）
 * 1. **485 的真实 MediaSources**（顺序刻意把 4K 放前面，复现旧代码取 `[0]` 的坑）：
 *    1080p 4.1 Mbps / 2160p 54.6 Mbps ⇒ 必须选 **1080p**。
 *    真机已证：1080p 版本 23.79 fps、4K 版本 1.53 fps。
 * 2. 全部都超阈值 ⇒ 取**最小**的，且 reason 必须写明"全部超阈值"（不许静默降级）。
 * 3. 单版本 4K ⇒ 仍然选它（没有更好的可选）。
 * 4. 空 / null ⇒ id 为 null（保持旧行为，URL 不带参数）。
 * 6. 多个可播版本 ⇒ 取码率最高的那个（画质优先），与顺序无关。
 */
class EmbySourcePickerInstrumentation : Instrumentation() {

    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        start()
    }

    override fun onStart() {
        val results = mutableListOf<Pair<String, Boolean>>()

        // ---- 1：485 的真实数据（4K 在前，复现旧代码 MediaSources[0] 的坑）
        val real = JSONArray()
            .put(source("f8d2f0a05971cb4a9efdd383a46b78be", "2160p", 54_580_352L, 57_523_754_510L))
            .put(source("aafdd622d6e1e15b27dfef460ffa95c8", "1080p", 4_107_522L, 4_316_184_874L))
        val pick1 = EmbySourcePicker.pick(real)
        results += "1 选 1080p（不是 4K）" to
            (pick1.id == "aafdd622d6e1e15b27dfef460ffa95c8" && pick1.bps == 4_107_522L)
        results += "1b playSize 跟着版本（4.3GB）" to (pick1.sizeBytes == 4_316_184_874L)

        // ---- 2：全部都超阈值 ⇒ 取最小 + 必须写明原因
        val allBig = JSONArray()
            .put(source("big-a", "2160p", 54_580_352L, 57_523_754_510L))
            .put(source("big-b", "1440p", 20_000_000L, 20_000_000_000L))
        val pick2 = EmbySourcePicker.pick(allBig)
        results += "2 全超阈值取最小" to (pick2.id == "big-b")
        results += "2b 原因写明超阈值（不静默）" to pick2.reason.contains("全部超阈值")

        // ---- 3：单版本 4K ⇒ 仍然选它
        val only4k = JSONArray()
            .put(source("only-4k", "2160p", 54_580_352L, 57_523_754_510L))
        val pick3 = EmbySourcePicker.pick(only4k)
        results += "3 单版本仍选它" to (pick3.id == "only-4k")
        results += "3b 原因标注只有一个版本" to pick3.reason.contains("只有一个版本")

        // ---- 4/5：空与 null ⇒ 回退旧行为
        results += "4 空数组回退旧行为" to (EmbySourcePicker.pick(JSONArray()).id == null)
        results += "5 null 回退旧行为" to (EmbySourcePicker.pick(null).id == null)

        // ---- 6：多个可播 ⇒ 取码率最高（1080 优于 720），与顺序无关
        val reordered = JSONArray()
            .put(source("a-4k", "2160p", 54_580_352L, 57_523_754_510L))
            .put(source("a-1080", "1080p", 3_000_000L, 3_000_000_000L))
            .put(source("a-720", "720p", 1_500_000L, 1_500_000_000L))
        results += "6 多个可播取码率最高（1080 优于 720）" to
            (EmbySourcePicker.pick(reordered).id == "a-1080")

        // ---- 7/8/9：301 编码闸门（4K+ AV1 且无硬解 ⇒ 实测 10.87 fps，必须避开）
        //
        // 【为什么这条必须有】4K AV1 的码率（11.9 Mbps）**低于** 12 Mbps 阈值，
        // 旧逻辑会把它当成"可播版本里画质最好的"选中，结果只有 10 fps。
        val av1VsHevc = JSONArray()
            .put(sourceWithVideo("av1-4k", "2160p AV1", 11_904_908L, 5_229_112_152L, "av1", 2160))
            .put(sourceWithVideo("hevc-1080", "1080p HEVC", 8_000_000L, 4_000_000_000L, "hevc", 1080))

        val pick7 = EmbySourcePicker.pick(av1VsHevc, av1Hw = false)
        results += "7 无 AV1 硬解 ⇒ 跳过 4K AV1，选 1080p HEVC" to (pick7.id == "hevc-1080")
        results += "7b 原因写明跳过了 4K AV1" to pick7.reason.contains("已跳过")

        // 有硬解就不该拦 —— 否则是另一种静默降权。
        val pick8 = EmbySourcePicker.pick(av1VsHevc, av1Hw = true)
        results += "8 有 AV1 硬解 ⇒ 按码率选 4K AV1（不误伤）" to (pick8.id == "av1-4k")

        // 全部都是 4K AV1 且无硬解 ⇒ 只能选最小，且原因不许留空。
        val allAv1 = JSONArray()
            .put(sourceWithVideo("av1-a", "2160p A", 11_904_908L, 5_229_112_152L, "av1", 2160))
            .put(sourceWithVideo("av1-b", "2160p B", 9_000_000L, 4_000_000_000L, "av1", 2160))
        val pick9 = EmbySourcePicker.pick(allAv1, av1Hw = false)
        results += "9 全 4K AV1 无硬解 ⇒ 选最小码率" to (pick9.id == "av1-b")
        results += "9b 原因写明无 AV1 硬解" to pick9.reason.contains("无 AV1 硬解")

        // 1080p AV1 实测 60.44 fps 可播 ⇒ 任何情况下都不该被拦。
        val av1_1080 = JSONArray()
            .put(sourceWithVideo("av1-1080", "1080p AV1", 8_000_000L, 4_000_000_000L, "av1", 1080))
            .put(sourceWithVideo("av1-1080b", "1080p AV1 b", 6_000_000L, 3_000_000_000L, "av1", 1080))
        results += "10 1080p AV1 不拦（实测 60.44fps）" to
            (EmbySourcePicker.pick(av1_1080, av1Hw = false).id == "av1-1080")

        // 本机事实：K40 应报"无 AV1 硬解"。这只影响上面分支走哪边，不影响结论正确性。
        val av1Hw = Av1HardwareProbe.hasHardwareAv1()
        Log.i(TAG, "本机 hasHardwareAv1=$av1Hw")

        val pass = results.count { it.second }
        results.forEach { (name, ok) ->
            Log.i(TAG, (if (ok) "PASS  " else "FAIL  ") + name)
        }
        Log.i(TAG, "==== $pass/${results.size} PASS ====")
        val allPass = pass == results.size
        finish(
            if (allPass) Activity.RESULT_OK else Activity.RESULT_CANCELED,
            Bundle().apply {
                putInt("total", results.size)
                putInt("pass", pass)
                putBoolean("av1Hardware", av1Hw)
            },
        )
    }

    private fun source(id: String, name: String, bps: Long, size: Long): JSONObject =
        JSONObject().put("Id", id).put("Name", name).put("Bitrate", bps).put("Size", size)

    /** 带 `MediaStreams`（Video 的 Codec/Height）的版本 —— 301 编码闸门要用。 */
    private fun sourceWithVideo(
        id: String,
        name: String,
        bps: Long,
        size: Long,
        codec: String,
        height: Int,
    ): JSONObject = source(id, name, bps, size).put(
        "MediaStreams",
        JSONArray().put(
            JSONObject().put("Type", "Video").put("Codec", codec).put("Height", height),
        ),
    )

    private companion object {
        const val TAG = "EmbyPicker"
    }
}
