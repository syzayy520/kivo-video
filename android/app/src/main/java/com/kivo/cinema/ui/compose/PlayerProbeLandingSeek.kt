package com.kivo.cinema.ui.compose

/**
 * 273 v35 取证钩子：**可脚本化的落点 seek**。
 *
 * ## 为什么需要它（不是"为了测试"，是"原路走不通"）
 *
 * 前助手 v26 声称"落点定向预取已收口"，但真机复算报「落点预取 **0 次**」——
 * 而那条代码路径**从来没被触发过**。原因不是代码写错，是**没人能可靠地触发它**：
 *
 *  · `adb shell input swipe` —— 走的是系统注入，**不经过 App 的手势分发**，
 *    Compose 收不到 `MotionEvent` ⇒ 拖动手势根本没发生（实测：内核 0 次 seek）。
 *  · `adb shell input motionevent DOWN/MOVE/UP` —— 同样是系统级注入，
 *    不产生给 App 的真实触摸序列 ⇒ 同样进不去手势层（实测：内核 0 次 seek）。
 *  · SMB 链路的 `applyProbeSubtitle` 带 `kivo.seekMs` —— 但它挂在
 *    `PlayerSmbStart.kt:175`，**URL(emby) 链路没有挂任何探针**。
 *
 * ⇒ 结果是：**Emby 链路的落点行为无法被脚本复现**，
 * 而 v26 的证据目录（`v26-*`）也从未产出 ⇒ 那一版是**未经任何真机验证**就写进规格的。
 *
 * ## 本钩子做什么
 *
 * 起播后按 `--ei kivo.landSeekSec N` 在**指定秒数**上走一次**完整的落点路径**：
 * `seekTo(ms)` ⇒ 落点预取（`prefetchSeek(force=true)`）⇒ 内核跳读。
 *
 * 它走的是**与用户拖动松手完全相同的代码路径**（`PlayerComposeActivity.seekTo`），
 * 所以它测的就是真机上的落点那一跳，不是另造一条平行路径。
 *
 * ## 用法
 *
 * ```
 * adb shell am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
 *   --es kivo.playurl "$URL" --es kivo.embyToken "$TOKEN" \
 *   --el kivo.playSize 1935985234 \
 *   --ei kivo.landSeekSec 4932          # 起播后第 N 秒跳一次
 *   --ei kivo.landSeekDelayMs 8000      # 起播后等多久跳（默认 8000）
 * ```
 *
 * 可重复给多组（逗号分隔）：`--es kivo.landSeekSecs "4932,1200,8000"`。
 *
 * ## 为什么单独一个文件
 *
 * `PlayerProbeHooks.kt` 已 300 行（**正好压在铁线上**）——
 * 再加就要拆。而"落点 seek"与"字幕探针 / mpv 选项探针"是**三种变化原因**：
 * 改字幕探针不该动落点探针。故独立成文件。
 */
internal fun PlayerComposeActivity.applyProbeLandingSeek() {
    // 280 · 275 §6.2 的「连续 20 次 seek 无旧请求覆盖新目标」压测模式。
    //
    // 【为什么需要单独一个模式】`kivo.landSeekSecs` 会 distinct+sorted 去重，
    // 且每组之间至少隔一个 delayMs ⇒ 测的是"慢速、目标互不相同"的落点，
    // **测不出**"上一次 seek 还没落位又来了下一次"这种覆盖竞争。
    // 压测模式用固定间隔连发，目标循环选取，专门压这条路径。
    //
    // 用法：--ei kivo.seekStress 20 [--ei kivo.seekStressIntervalMs 1200]
    val stress = intent.getIntExtra("kivo.seekStress", 0).coerceIn(0, 100)
    if (stress > 0) {
        val interval = intent.getIntExtra("kivo.seekStressIntervalMs", 1_200).coerceIn(300, 10_000)
        val startDelay = intent.getIntExtra("kivo.seekStressDelayMs", 8_000).coerceIn(1_000, 60_000)
        val handler = android.os.Handler(mainLooper)
        repeat(stress) { i ->
            handler.postDelayed({
                if (finishRequested || isDestroyed) return@postDelayed
                val totalSec = durationMs / 1000
                if (totalSec <= 0) return@postDelayed
                // 在 [10s, 片尾-10s] 之间循环取目标（步长 37 是质数，避免与间隔形成周期）
                val span = (totalSec - 20).coerceAtLeast(10)
                val target = 10 + (i.toLong() * 37) % span
                smbTrace("probe seek-stress ${i + 1}/$stress → ${target}s")
                seekTo(target * 1000L, waitLandingFirstSlice = true)
            }, (startDelay + i * interval).toLong())
        }
        return
    }

    val single = intent.getIntExtra("kivo.landSeekSec", -1)
    val many = intent.getStringExtra("kivo.landSeekSecs").orEmpty()
    val delayMs = intent.getIntExtra("kivo.landSeekDelayMs", 8_000).coerceIn(1_000, 120_000)

    // 单值与多值合并（多值优先）。
    val seconds = buildList {
        many.split(',').map { it.trim().toIntOrNull() }.filterNotNull().forEach(::add)
        if (single > 0) add(single)
    }.distinct().sorted()
    if (seconds.isEmpty()) return

    val handler = android.os.Handler(mainLooper)
    seconds.forEachIndexed { index, sec ->
        // 多组时每组之间也隔开 delayMs ⇒ 每一组都是独立的"起播后一次落点"。
        val at = delayMs + index * delayMs
        handler.postDelayed({
            if (finishRequested || isDestroyed) return@postDelayed
            val ms = sec * 1000L
            smbTrace("probe landing-seek $ms ms（起播后 ${at}ms）")
            // 走**与用户拖动松手完全相同**的入口（含 v42 的"等第一片就绪"）。
            seekTo(ms, waitLandingFirstSlice = true)
        }, at.toLong())
    }
}
