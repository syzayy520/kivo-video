package com.kivo.cinema.ui.compose

import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.playback.subtitle.SubtitleImportStore
import com.kivo.cinema.subtitle.AiSubtitleController
import com.kivo.cinema.ui.compose.attachImported
import com.kivo.cinema.ui.compose.player.subtitle.applySubtitleProbeWrites

/**
 * 取证钩子（253-P6-2 字幕 / mpv 选项 A/B / 缩略图探针）。p215 压测在 PlayerProbeStress.kt。
 *
 * 【为什么与播放页 UI 分开】这两段都由 intent extra 驱动、只在取证时跑，
 * 不是用户功能。混在 Activity 里会让"组合根"读起来像业务代码。
 */

/**
 * 按 intent 指定轨道选字幕，并可 seek + 冻结 + 截图。
 *
 * 用法：
 * ```
 * am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
 *   --es kivo.sourceId <id> --es kivo.smbPath <path> \
 *   --ei kivo.subtitleId 2 --ei kivo.seekMs 14014 --ez kivo.freeze true
 * ```
 */

// ---- AI 冒烟的 Controller 单例（Activity 级）----
// 两次首帧就绪 = 两个探针 Thread 各 new 一个 Controller = 两个
// LocalAiEngineClient = 第二个的 loadModel 会销毁第一个正在用的 C++ 引擎
//（g_engine.reset()）→ 第一个 run 死在 JOB_CREATED 后（2026-11-21 冒烟
// journal 实测 JOB_CREATED ×2 且 seq 均=1）。单例消除双实例竞态。
private var aiControllerInstance: AiSubtitleController? = null
private val aiControllerLock = Any()

internal fun PlayerComposeActivity.aiSubtitleController(): AiSubtitleController =
    synchronized(aiControllerLock) {
        aiControllerInstance ?: AiSubtitleController(this).also { aiControllerInstance = it }
    }

internal fun PlayerComposeActivity.applyProbeSubtitle(session: MpvPlaybackSession) {
    val subtitleId = intent.getIntExtra("kivo.subtitleId", -1)
    val seekSec = intent.getIntExtra("kivo.seekSec", -1)
    // PGS 是位图字幕，一个包只覆盖几十毫秒（实测中文轨 14.014–14.056，42ms）。
    // 整秒 seek 会落在包外 ⇒ 必然拍到"没有字幕"，看起来像渲染坏了。
    val seekMs = intent.getIntExtra("kivo.seekMs", -1)
    val freeze = intent.getBooleanExtra("kivo.freeze", false)

    // 【顺序不可换】mpv 切字幕轨会做一次轨道刷新（日志 `refresh seek to 3.000000`），
    // 把位置打回该轨自己的起点。先 seek 后选轨 ⇒ 位置从 14s 被打回 3s，
    // 而字幕 13.639s 才开始 ⇒ "字幕不上屏"其实是取证落在了没有字幕的时刻。
    // 所以：先选轨 → 等落定 → 再 seek → 最后暂停。
    val selectDelayMs = 1_500L
    val seekDelayMs = selectDelayMs + 1_200L

    if (subtitleId >= 0) {
        android.os.Handler(mainLooper).postDelayed({
            val ok = session.selectSubtitle(subtitleId)
            smbTrace("probe subtitle id=$subtitleId ok=$ok")
            statusText = if (ok) "字幕轨 #$subtitleId 已选中"
            else "字幕轨 #$subtitleId 选中失败：${session.lastError}"
            // 外挂 SRT 走 libass 文本通路，PGS 走位图通路 —— 挂一条 SRT 就能一刀切开：
            // SRT 能显示 ⇒ 合成通路正常，问题在位图解码；SRT 也不显示 ⇒ 整个字幕层有问题。
            // P9-F 取证：支持逗号分隔挂多个外挂（第一个当主字幕，其余留在
            // 次字幕列表里做 dual-subtitle 选择验证）。
            val subFiles = intent.getStringExtra("kivo.subFile")
                ?.split(',')?.map { it.trim() }?.filter { it.isNotEmpty() } ?: emptyList()
            var firstAdded: Int? = null
            for (name in subFiles) {
                val file = java.io.File(filesDir, name)
                if (!file.isFile) {
                    smbTrace("probe sub-add $name NOT FOUND")
                    continue
                }
                val added = session.loadExternalSubtitle(file)
                smbTrace("probe sub-add $name added=$added")
                if (added && firstAdded == null) {
                    firstAdded = session.tracks.externalSubtitleIds().firstOrNull()
                }
            }
            // mpv 不会自动选外挂轨，挂上后必须选中。
            val pick = firstAdded
            if (pick != null) {
                val sel = session.selectSubtitle(pick)
                smbTrace("probe sub-select id=$pick ok=$sel")
                statusText = "外挂字幕 #$pick 已选中"
            }
        }, selectDelayMs)
    }

    val targetMs = if (seekMs >= 0) seekMs.toLong() else if (seekSec > 0) seekSec * 1000L else -1L
    if (targetMs < 0) return
    android.os.Handler(mainLooper).postDelayed({
        session.seekTo(targetMs * 1000L)
        if (!freeze) return@postDelayed
        // 等落位完成再停 —— 立刻 pause 可能停在跳转途中。
        android.os.Handler(mainLooper).postDelayed({
            session.setPaused(true)
            smbTrace("probe freeze at ${targetMs}ms")
            probeScreenshot(session)
        }, 2_500)
    }, seekDelayMs)
}

/**
 * 取证钩子：播放中**运行时**改 mpv 选项，做 A/B 取证。
 *
 * 用法：
 * ```
 * am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
 *   --es kivo.playurl <url> --es kivo.mpvOpt "demuxer-max-bytes=8MiB,hwdec=mediacodec"
 * ```
 *
 * 【为什么要有这个钩子】内存/卡顿这类问题要归因到**某一个**选项，
 * 而每次改代码重编装机一轮要好几分钟。把改选项做成运行时下发，
 * 一次装机就能把候选组合逐个跑完，每组都有回执与内存读数。
 *
 * 【不许静默】每项 setProperty 的回执都写进 trace —— 改不动的选项要看得见。
 */
internal fun PlayerComposeActivity.applyProbeMpvOptions(session: MpvPlaybackSession) {
    val spec = intent.getStringExtra("kivo.mpvOpt")?.trim().orEmpty()
    if (spec.isEmpty()) return
    val items = spec.split(',').map { it.trim() }.filter { it.isNotEmpty() }
    if (items.isEmpty()) return
    // 起播后再下发：内核 initialize 之前设置会走 option 通道（受回执检查约束），
    // 这里走的是 property 通道，用于**已经跑起来**的会话。
    android.os.Handler(mainLooper).postDelayed({
        items.forEach { item ->
            val name = item.substringBefore('=').trim()
            val value = item.substringAfter('=', "").trim()
            val ok = runCatching { session.native().setProperty(name, value) }.getOrDefault(false)
            smbTrace("probe mpv-opt $name=$value ok=$ok")
        }
    }, 3_000)
}




internal fun PlayerComposeActivity.applyProbeSubtitleCodec() {
    // 取证钩子：`--es kivo.subCodec BIG5` —— 走**与面板选中完全同一条** applySubtitleEncoding。
    // 延迟默认 3s；`--ei kivo.subCodecDelayMs` 可推后（A5 暂停态取证：kivo.freeze 的 pause
    // 发生在 onCreate+5.2s，切换探针必须排在它之后才能测到 paused=true 的分支）。
    val cp = intent.getStringExtra("kivo.subCodec")?.trim().orEmpty()
    if (cp.isEmpty()) return
    val delayMs = intent.getIntExtra("kivo.subCodecDelayMs", 3_000).coerceAtLeast(0).toLong()
    android.os.Handler(mainLooper).postDelayed({
        smbTrace("probe sub-codec=$cp")
        applySubtitleEncoding(cp)
    }, delayMs)
}

internal fun PlayerComposeActivity.applyProbeSubtitleUrl() {
    // 取证钩子：`--es kivo.subUrl "http://192.168.5.142:8899/remote.srt"`。
    // 与产品路径 importSubtitleUrl 共用同一段代码 —— 本机 MIUI 没有 `cmd clipboard`，
    // 剪贴板无法用 adb 设值，只能从"执行入口"这一层取证。
    val url = intent.getStringExtra("kivo.subUrl")?.trim().orEmpty()
    if (url.isEmpty()) return
    smbTrace("probe sub-url $url")
    importSubtitleUrl(url)
}

// ---- AI 字幕翻译端到端冒烟（T-B，2026-11-21）：`--ez kivo.aiSmoke true` ----
// 流程：ModelStore 发现模型 → 引擎加载 → 内置测试 SRT（3 条：对白/[Music]/对白）
// → TranslationJob.run（装配→切块→逐块翻译→journal）→ trace 取证。
// UI 正式入口行（T-C）落地前，这是端到端链路的唯一触发通道 —— 与既有探针
// 架构一致（intent 驱动、取证时跑，不是用户功能）。
internal fun PlayerComposeActivity.applyProbeAiSmoke() {
    // 【进入即打】区分「函数未被调用」与「extra 未传到」——首帧就绪链路的
    // 执行与否本身就是要验证的事实（2026-11-21 冒烟 trace 无行排查）。
    val smokeExtra = intent.getBooleanExtra("kivo.aiSmoke", false)
    smbTrace("ai-smoke probe-enter extra=$smokeExtra")
    if (!smokeExtra) return
    val delayMs = intent.getIntExtra("kivo.aiSmokeDelayMs", 5_000).coerceAtLeast(0).toLong()
    // mode：full（默认，挂载+自动翻译）/ mount（只挂英文 SRT——面板行点击成为
    // 唯一翻译触发，消除探针自动翻译与面板行单任务互斥的时序冲突，tc9 根因）。
    val mode = intent.getStringExtra("kivo.aiSmokeMode")?.trim().orEmpty().ifBlank { "full" }
    android.os.Handler(mainLooper).postDelayed({
        Thread {
            // 【可读性】外部目录日志 —— adb shell cat 直接可读，不绕 run-as
            //（SELinux 拒了 run-as 跨目录拷贝，2026-11-21 冒烟排查实测）。
            val smokeLog = java.io.File(getExternalFilesDir(null), "ai-smoke.log")
            fun aiTrace(msg: String) {
                smbTrace(msg)
                runCatching { smokeLog.appendText("${System.currentTimeMillis()} $msg\n") }
            }
            val controller = aiSubtitleController()
            if (!controller.isNativeAvailable()) {
                aiTrace("ai-smoke FAIL libkivo_ai.so not available")
                statusText = "AI 冒烟：引擎库不可用"
                return@Thread
            }
            val model = com.kivo.cinema.subtitle.AiSubtitleModelStore.findModel(this)
            aiTrace("ai-smoke model=${model?.absolutePath ?: "MISSING"}")
            if (model == null) {
                aiTrace("ai-smoke guidance=${controller.modelGuidance()}")
                statusText = "AI 冒烟：未找到模型"
                return@Thread
            }
            val loaded = controller.ensureModelLoaded()
            aiTrace("ai-smoke loaded=$loaded err=${if (loaded) "" else controller.modelGuidance()}")
            if (!loaded) {
                statusText = "AI 冒烟：模型加载失败"
                return@Thread
            }
            // 【英文字幕就位】内置测试 SRT 写入 files/ 并挂载（面板行点击路径
            // 需要 sid 有值——T-C 真机验证补全，2026-11-21）。
            val enSrt = java.io.File(filesDir, "e2e-test-en.srt")
            val enSrtText = "1" + "\n" + "00:00:01,000 --> 00:00:03,000" + "\n" +
                "I thought you were leaving." + "\n\n" +
                "2" + "\n" + "00:00:04,000 --> 00:00:06,000" + "\n" +
                "Stop right there." + "\n"
            enSrt.writeText(enSrtText, Charsets.UTF_8)
            // 英文字幕走**产品导入通道**（importFile → attachImported：登记 +
            // sub-add + 去重 + panelRevision 全套——评审「产品正确性优先」：
            // 探针裸调 loadExternalSubtitle 的「挂载 vs 面板渲染不同步」层被绕开，
            // 2026-11-21；attachImported 的 mpv 挂载失败不阻断 mount-only 登记）。
            val store = SubtitleImportStore(this) { aiTrace(it) }
            val imported = store.importFile(enSrt, "e2e-test-en")
            if (imported == null) {
                aiTrace("ai-smoke mount import FAILED")
                statusText = "AI 冒烟：英文字幕导入失败"
                return@Thread
            }
            val mpv = mpvNetSession
            if (mpv != null) {
                val failure = attachImported(imported, mpv)
                aiTrace("ai-smoke mount product-channel failure=$failure")
                runOnUiThread { panelRevision++ }
            } else {
                aiTrace("ai-smoke mount mpv-missing (import registered only)")
            }
            aiTrace("ai-smoke mount-only done")
            statusText = "AI 冒烟：英文字幕已挂载，可在面板点击 AI 中文字幕"
            if (mode == "mount") { return@Thread }
            statusText = "AI 冒烟：翻译中"
            aiTrace("ai-smoke pre-start")
            // 内置测试 SRT：对白 + [Music]（规则映射零 token）+ 对白。
            val srt = buildString {
                append("1\n00:00:01,000 --> 00:00:03,000\nI thought you were leaving.\n\n")
                append("2\n00:00:04,000 --> 00:00:06,000\n[Music]\n\n")
                append("3\n00:00:07,000 --> 00:00:09,000\nStop right there.\n")
            }
            aiTrace("ai-smoke calling-start")
            controller.startTranslation(
                srt,
                playheadMs = 0,
                onProgress = { progress ->
                    aiTrace("ai-smoke ${progress.state} ${progress.processedCues}/${progress.totalCues}")
                },
                onDone = { result ->
                    smbTrace(
                        "ai-smoke done path=${result.translationPath ?: "null"} " +
                            "llm=${result.llmTranslated} rule=${result.ruleTranslated} " +
                            "keep=${result.intentionalKeep} failed=${result.failedFallbacks} " +
                            "total=${result.totalCues} cancelled=${result.cancelled}",
                    )
                    statusText = if (result.translationPath != null) {
                        "AI 冒烟完成：${result.llmTranslated + result.ruleTranslated}/${result.totalCues} 条已译"
                    } else {
                        "AI 冒烟失败"
                    }
                },
            )
            aiTrace("ai-smoke start-returned")
        }.start()
    }, delayMs)
}

internal fun PlayerComposeActivity.applyProbeSubtitleWrites(session: MpvPlaybackSession) {
    val spec = intent.getStringExtra("kivo.subWrite")?.trim().orEmpty()
    if (spec.isEmpty()) return
    android.os.Handler(mainLooper).postDelayed({
        val native = session.nativeOrNull() ?: return@postDelayed
        applySubtitleProbeWrites(spec, native)
    }, 2_000)
}

/**
 * 让 mpv 自己截"合成后的画面"。
 *
 * 【为什么必须在播放页而不是 instrumentation】`screenshot-to-file ... window` 走
 * MODE_FULL_WINDOW，尺寸取自 `osd_get_vo_res`。instrumentation 用 SurfaceTexture(0)
 * 虚拟纹理，没有消费者 ⇒ VO 退化成 1×1 ⇒ 截出 166 字节空图。只有真实 Surface 才有意义。
 *
 * 【两张图回答什么】window 有字幕 ⇒ 内部渲染正常，问题在送 Surface；
 * window 没有但 subtitles 有 ⇒ 生成了没合成；两张都没有 ⇒ 内部就没生成。
 */
private fun PlayerComposeActivity.probeScreenshot(session: MpvPlaybackSession) {
    val shot = intent.getStringExtra("kivo.shot")
    if (shot.isNullOrBlank()) return
    // 诊断用：把字幕往上顶。出现 ⇒ 位置问题；仍不出现 ⇒ 位图没进合成。
    val subPos = intent.getStringExtra("kivo.subPos")
    if (!subPos.isNullOrBlank()) {
        val ok = session.native().setProperty("sub-pos", subPos)
        smbTrace("probe sub-pos=$subPos ok=$ok")
    }
    val out = java.io.File(filesDir, "p253-shots").apply { mkdirs() }
    val windowShot = java.io.File(out, shot)
    val subOnly = java.io.File(out, shot.replace(".png", "-subs.png"))
    windowShot.delete()
    val okWindow = session.command("screenshot-to-file", "${windowShot.absolutePath} window")
    val okSubs = session.command("screenshot-to-file", "${subOnly.absolutePath} subtitles")
    Thread.sleep(1_200)
    smbTrace(
        "probe shot $shot window=$okWindow(${if (windowShot.isFile) windowShot.length() else -1}) " +
            "subs=$okSubs(${if (subOnly.isFile) subOnly.length() else -1})",
    )
    statusText = "截图：window ${if (windowShot.isFile) windowShot.length() else -1}B" +
        " / subs ${if (subOnly.isFile) subOnly.length() else -1}B"
}

// ---- 273-E 取证：`--es kivo.thumbOpt "name=value,name=value"`` 追加预览内核的 mpv 选项 ----
// 预览实例"取一帧却读了 38 MiB"（真机 273-E）需要归因到**某一个**选项，
// 而每次改代码重编装机一轮要好几分钟。把改选项做成运行时下发，
// 一次装机就能把候选组合逐个跑完（每组都有回执与下载量读数）。
internal fun PlayerComposeActivity.probeThumbOptions(): List<Pair<String, String>> {
    val spec = intent.getStringExtra("kivo.thumbOpt")?.trim().orEmpty()
    if (spec.isEmpty()) return emptyList()
    return spec.split(',')
        .map { it.trim() }
        .filter { it.isNotEmpty() }
        .mapNotNull { item ->
            val name = item.substringBefore('=').trim()
            val value = item.substringAfter('=', "").trim()
            if (name.isEmpty()) null else name to value
        }
}

// ---- v3.1 取证：`--ei kivo.thumbDumpSec N` 在 N 秒后落缩略图证据（SMB 拖动预览用）----

internal fun PlayerComposeActivity.scheduleThumbDump() {
    val delaySec = intent.getIntExtra("kivo.thumbDumpSec", -1)
    if (delaySec < 0) return
    android.os.Handler(mainLooper).postDelayed({
        val reader = thumbnailReader ?: return@postDelayed
        // evidenceJson 要等解码线程最多 2s：放主线程就是取证自己造卡顿，
        // 且解码被 SMB 慢读占住时曾返回空串 → JSONObject 抛异常闪退。
        Thread {
            val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
            val root = org.json.JSONObject()
            root.put("schema", "kivo.v31.thumbnail/1")
            root.put("generated_at_unix_ms", System.currentTimeMillis())
            root.put("source", intent.getStringExtra("kivo.smbPath").orEmpty().ifBlank { currentMediaPath() })
            root.put("thumbnail", org.json.JSONObject(reader.evidenceJson()))
            java.io.File(dir, "v31-thumb.json").writeText(root.toString(2))
            smbTrace("v31 thumb evidence written")
        }.start()
    }, delaySec * 1000L)
}

// ---- v3.1 取证：`--ei kivo.thumbProbeSec S` 确定性预览探针 ----
// 自动化拖动手势难稳定命中轨道（视频层横滑会被当成 seek 手势吃掉）。探针直接
// seekTo(S) 并把控制条+浮窗强制可见，给截屏留确定窗口；grabs[] 的 px0 同时
// 证明 EGL→ImageReader 传输是否真的落到非黑帧（红清屏二分后的验收口径）。
internal fun PlayerComposeActivity.scheduleThumbProbe() {
    val sec = intent.getIntExtra("kivo.thumbProbeSec", -1)
    if (sec < 0) return
    android.os.Handler(mainLooper).postDelayed({
        val reader = thumbnailReader ?: return@postDelayed
        controlsVisible = true
        lastTouchMs = System.currentTimeMillis()
        val posMs = sec * 1000L
        previewMs = posMs
        previewRequestedMs = posMs
        if (!scrub.active) scrub.begin(posMs) else scrub.update(posMs)
        reader.seekTo(posMs * 1000)
        android.os.Handler(mainLooper).postDelayed({ previewMs = -1L; scrub.cancel() }, 5000)
    }, 6000)
}

// ---- v3.1d 取证：`--ei kivo.thumbScan N` 网络预览 N 点拖动扫描 ----
// p215 只跑本地 ThumbnailReader；SMB 的 MpvThumbnailReader 需要自己的扫描。
// 250ms 间距模拟连续拖动，latest-wins 自然丢弃被超越的请求；warm 分位数即
// "手指停下后画面多久跟上"。末尾补一次已缓存位置证明 cache-hit 通道瞬时。
internal fun PlayerComposeActivity.scheduleThumbScan() {
    val points = intent.getIntExtra("kivo.thumbScan", 0)
    if (points <= 0) return
    android.os.Handler(mainLooper).postDelayed({ runThumbScan(0, points) }, 8000)
}

internal fun PlayerComposeActivity.runThumbScan(index: Int, points: Int) {
    val reader = thumbnailReader ?: return
    if (index >= points) {
        // 回到扫描中段（必已缓存）：cache-hit 通道应瞬时出帧。
        if (durationMs > 0) {
            val posMs = durationMs / 2
            previewMs = posMs
            previewRequestedMs = posMs
            scrub.update(posMs)
            reader.seekTo(posMs * 1000)
        }
        android.os.Handler(mainLooper).postDelayed({ writeThumbScanEvidence() }, 2500)
        return
    }
    if (durationMs > 0) {
        val posMs = ((index + 0.5) / points * durationMs).toLong()
        previewMs = posMs
        previewRequestedMs = posMs
        if (!scrub.active) scrub.begin(posMs) else scrub.update(posMs)
        controlsVisible = true
        lastTouchMs = System.currentTimeMillis()
        reader.seekTo(posMs * 1000)
    }
    android.os.Handler(mainLooper).postDelayed({ runThumbScan(index + 1, points) }, 250)
}

internal fun PlayerComposeActivity.writeThumbScanEvidence() {
    val reader = thumbnailReader ?: return
    Thread {
        val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
        val root = org.json.JSONObject()
        root.put("schema", "kivo.v31.thumbnail/1")
        root.put("generated_at_unix_ms", System.currentTimeMillis())
        root.put("source", intent.getStringExtra("kivo.smbPath").orEmpty())
        root.put("scan_points", intent.getIntExtra("kivo.thumbScan", 0))
        root.put("thumbnail", org.json.JSONObject(reader.evidenceJson()))
        java.io.File(dir, "v31-thumb.json").writeText(root.toString(2))
        smbTrace("v31 thumb scan evidence written")
    }.start()
}

