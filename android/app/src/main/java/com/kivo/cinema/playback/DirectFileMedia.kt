package com.kivo.cinema.playback

import java.io.File

/**
 * 354 · 本地文件能否直接交给 mpv 读（预检 + 交给内核的形态）。
 *
 * ## 为什么传纯绝对路径，不加 `file://`
 *
 * mpv 的 `loadfile` 参数解析有两条分支：
 * · 含 `scheme://` ⇒ 按 URL 处理（会对 `%xx` 做 unescape）
 * · 否则 ⇒ 按**文件名**处理（不做任何转义还原）
 *
 * 我们要的正是第二条：路径里的 `%`、`#`、`?` 都只是文件名字符，一旦套上 `file://`
 * 反而会被当成转义序列/片段分隔符而打开失败。空格与中文同理 ——
 * `loadfile` 是独立 argv，不经过 shell，不需要引号或转义。
 *
 * ⇒ **唯一要处理的是开头连续的 `/`**：`//foo` 会被 mpv 当成 UNC/网络路径。
 *
 * 真机已验证（354 V2/V3）：`测试 视频 100%.mp4`（中文 + 空格 + `%`）直连正常出画。
 *
 * ## 预检为什么只做一次 `length()`，不做实读
 *
 * 真机实测（K40，421 MB mp4，direct/proxy 各 5 轮交替）：
 * · 直连的 `loadfile→首帧` **稳定快 43 ms**（378 vs 421）——这一跳是真省下来的；
 * · 但预检若做 4 KiB 实读，`kernel_created` **稳定多 20 ms**（58 vs 38），正好抵掉；
 *   64 KiB 实读更差（52–83 ms）。
 *
 * 实读能多防的场景只有"权限位可读但实际读不出"（scoped storage 边界、外置存储弹出）。
 * 那种情况下 mpv 会返回明确的打开错误、会话进入 FAILED 并带 `lastError` ——
 * **不是黑屏**，用户看到的是可读原因。用 20 ms 首帧换一个本来就有错误信息的场景，不划算。
 *
 * ⇒ 保留 `length()`（一次 stat，判掉 不存在 / 空 / 目录），放弃实读。
 * 这是"最强"的判断：**每一项成本都要有对应的收益**。
 */
internal object DirectFileMedia {

    /** 预检结论。失败也要带**可读原因**，因为它会进日志与用户可见状态。 */
    internal sealed interface Plan {
        class Ok(val uri: String, val sizeBytes: Long) : Plan
        class Failed(val reason: String) : Plan
    }

    /**
     * 预检并给出交给 mpv 的路径。
     *
     * @param path 绝对路径（来自 MediaStore 的 `DATA` 列，不是 `content://`）
     */
    fun plan(path: String): Plan {
        val p = normalize(path)
        if (p.isEmpty()) return Plan.Failed("路径为空")
        val f = File(p)
        // 一次 stat：目录与非普通文件的 length 也是 0，与空文件同样处理（都不该直连）。
        val size = f.length()
        if (size > 0L) return Plan.Ok(p, size)
        return Plan.Failed(if (f.exists()) "文件为空或不是普通文件" else "文件不存在")
    }

    /**
     * 折叠开头连续的 `/`。
     *
     * `//storage/...` 在 mpv 的 URL 判定里会被当成 `//host` 形式 ⇒ 走网络分支 ⇒ 打不开。
     * 折叠成一个 `/` 是等价的绝对路径。
     */
    private fun normalize(path: String): String {
        val t = path.trim()
        if (!t.startsWith("/")) return t
        var i = 0
        while (i < t.length && t[i] == '/') i++
        return "/" + t.substring(i)
    }
}
