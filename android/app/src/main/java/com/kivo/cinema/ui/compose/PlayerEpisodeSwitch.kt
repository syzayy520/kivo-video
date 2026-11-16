package com.kivo.cinema.ui.compose

import com.kivo.cinema.net.NetDiag

/**
 * 317 · **在同一 Activity 内无缝换集**（内核级，不重启界面）。
 *
 * ## 为什么需要它
 * 连播第一版走的是"新 Activity + finish 旧的"，切集时**黑 1~2 秒**。
 * SenPlayer 是内核 `loadfile` 无缝切 —— 要做到"持平或更好"，就必须留在同一个 Activity 里换源。
 *
 * ## 为什么这样能成立（三件事都已有）
 * 1. `prepareUrlSource()` 读的是 `intent.getStringExtra("kivo.playurl")`，
 *    而 `Activity.intent` 是**同一个对象** ⇒ `putExtra` 之后会立刻生效；
 * 2. `closeNetworkPlayback()` 能清掉旧会话（含 `NetworkPlaybackSession` 与内核），
 *    并且刚加了完成回调 ⇒ **等旧会话停稳再起新的**，不会两条会话抢同一条代理/内核路径；
 * 3. `startUrlPlayback()` 自带 `generation` 取消机制，本来就是按可重入设计的。
 *
 * ## 顺序为什么不能换
 * **先清会话 → 再关旧源 → 再重建源 → 最后起播**。
 * 反过来（先关源）会让仍在跑的旧会话读到已关闭的源，表现为"切集瞬间报一堆读失败"。
 */
internal fun PlayerComposeActivity.switchEpisodeImmediately(url: String, label: String): Boolean {
    val holder = lastSurfaceHolder
    if (holder == null) {
        NetDiag.log("[autonext] 没有 SurfaceHolder，无法无缝换集 ⇒ 回落到重启 Activity")
        // 不该发生：能播到结束就说明 Surface 曾就绪。留日志便于追。
        return false
    }
    NetDiag.log("[autonext] 无缝换集开始：$label")
    closeNetworkPlayback {
        if (isFinishing || isDestroyed) return@closeNetworkPlayback
        // ① 改 intent，让所有"读 intent 拿 URL"的路径（prepareUrlSource / 指标 / 观看记录键）看到新片
        intent.putExtra("kivo.playurl", url)
        intent.putExtra("kivo.label", label)
        // ② 关旧源（此时旧会话已停，读不到它了）
        runCatching { urlPrep?.source?.close() }
        urlPrep = null
        // ③ 重建源（新 URL 的新连接/预热）
        urlPrep = prepareUrlSource()
        // ④ 重新起播
        startUrlPlayback(holder, url, label)
        autoNextScheduled = false
        NetDiag.log("[autonext] 无缝换集已提交起播")
    }
    return true
}
