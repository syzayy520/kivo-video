package com.kivo.cinema.emby

import android.app.Activity
import android.app.Instrumentation
import android.os.Bundle
import android.util.Log
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
            },
        )
    }

    private fun source(id: String, name: String, bps: Long, size: Long): JSONObject =
        JSONObject().put("Id", id).put("Name", name).put("Bitrate", bps).put("Size", size)

    private companion object {
        const val TAG = "EmbyPicker"
    }
}
