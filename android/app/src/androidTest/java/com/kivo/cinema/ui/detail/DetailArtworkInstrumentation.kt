package com.kivo.cinema.ui.detail

import android.app.Activity
import android.app.Instrumentation
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Paint
import android.os.Bundle
import android.graphics.Rect
import android.view.accessibility.AccessibilityNodeInfo
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.layout.boundsInRoot
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.dp
import com.kivo.cinema.MainActivity
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.TmdbImageCache
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.util.concurrent.ConcurrentHashMap

/** Component evidence only: no online title overrides, persistent settings, or user cache edits. */
class DetailArtworkInstrumentation : Instrumentation() {
    private var label = "probe"
    override fun onCreate(arguments: Bundle?) {
        super.onCreate(arguments)
        label = arguments?.getString("label") ?: label
        require(label.matches(Regex("[a-z0-9-]+")))
        start()
    }

    override fun onStart() {
        val folder = File(targetContext.cacheDir, "detail-artwork-probe/$label").apply { mkdirs() }
        var activity: MainActivity? = null
        try {
            activity = startActivitySync(Intent(targetContext, MainActivity::class.java)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) as MainActivity
            val host = activity
            val goodLogo = "fixture://logo-success-$label"
            val bitmap = Bitmap.createBitmap(480, 100, Bitmap.Config.ARGB_8888)
            Canvas(bitmap).drawText("KIVO TEST", 12f, 76f, Paint().apply {
                color = android.graphics.Color.BLACK; textSize = 70f; isAntiAlias = true
            })
            TmdbImageCache.put(goodLogo, bitmap)
            val longTitle = "这是用于检查长片名换行和缺图回退的隔离测试作品：穿越星空之后仍然继续的漫长旅程"
            val cases = listOf(
                Triple("no-logo-short", "测试片名", null),
                Triple("no-logo-long", longTitle, null),
                Triple("failed-logo", "图片失败仍应显示的测试片名", "fixture://failed-logo-$label"),
                Triple("successful-logo", "成功图片测试片名", goodLogo),
                Triple("large-long", longTitle, null),
                Triple("missing-art", "无封面测试片名", null),
                Triple("failed-art", "封面失败测试片名", null),
            )
            val report = JSONArray()
            for ((name, title, logo) in cases) {
                val geometry = ConcurrentHashMap<String, List<Float>>()
                val model = TmdbDetail.Details(0, title, "2026-01-01", emptyList(), "", 0f,
                    "", null, if (name == "failed-art") "fixture://failed-art-$label" else "",
                    logo, false, "测试地区", null, 90, null, emptyList(), emptyList(),
                    null, emptyList(), emptyList(), emptyList())
                val scale = if (name == "large-long") 1.5f else 1f
                runOnMainSync {
                    host.setContent {
                        val deviceDensity = LocalDensity.current
                        CompositionLocalProvider(LocalDensity provides Density(deviceDensity.density, scale)) {
                            key(name) {
                                Box(Modifier.fillMaxSize().background(Color(0xFFF2F2F8))) {
                                    if (name.endsWith("art")) DetailBackdrop(model,
                                        remember { mutableStateOf(Color(0xFFF2F2F8)) }, { 0f })
                                    Column(Modifier.fillMaxSize().statusBarsPadding().verticalScroll(rememberScrollState())) {
                                        Text("隔离组件测试 · $name · $scale", color = Color.Black)
                                        Spacer(Modifier.height(300.dp))
                                        Box(Modifier.onGloballyPositioned {
                                            val r = it.boundsInRoot()
                                            geometry["title"] = listOf(r.left, r.top, r.right, r.bottom)
                                        }) { TitleLayer(model, Color.Black) }
                                        Box(Modifier.onGloballyPositioned {
                                            val r = it.boundsInRoot()
                                            geometry["meta"] = listOf(r.left, r.top, r.right, r.bottom)
                                        }) { MetaLine(model, Color.Black) }
                                    }
                                }
                            }
                        }
                    }
                }
                Thread.sleep(2200)
                waitForIdleSync()
                val nodes = JSONArray()
                fun visit(n: AccessibilityNodeInfo) {
                    if (!n.refresh()) return
                    val r = Rect(); n.getBoundsInScreen(r)
                    if (n.text != null || n.contentDescription != null) nodes.put(JSONObject().apply {
                        put("text", n.text?.toString().orEmpty())
                        put("description", n.contentDescription?.toString().orEmpty())
                        put("bounds", JSONArray(listOf(r.left, r.top, r.right, r.bottom)))
                    })
                    for (i in 0 until n.childCount) n.getChild(i)?.let { visit(it) }
                }
                uiAutomation.rootInActiveWindow?.let { visit(it) }
                val screenshot = checkNotNull(uiAutomation.takeScreenshot())
                File(folder, "$name.png").outputStream().use { screenshot.compress(Bitmap.CompressFormat.PNG, 100, it) }
                screenshot.recycle()
                report.put(JSONObject().apply {
                    put("case", name); put("fixture_title", title); put("font_scale", scale)
                    put("geometry", JSONObject(geometry.mapValues { JSONArray(it.value) }))
                    put("nodes", nodes)
                })
            }
            File(folder, "report.json").writeText(report.toString(2))
            finish(Activity.RESULT_OK, Bundle().apply { putString("stream", "7 fixture captures saved: $label\n") })
        } catch (e: Throwable) {
            finish(Activity.RESULT_CANCELED, Bundle().apply { putString("stream", e.stackTraceToString()) })
        } finally {
            activity?.let { runOnMainSync { it.finish() } }
        }
    }
}
