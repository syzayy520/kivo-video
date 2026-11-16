package com.kivo.cinema.hls

/**
 * HLS **master playlist** 的解析与裁剪（纯文本，无 IO）。
 *
 * ## 为什么需要裁剪
 * 310 实测：真实 HLS master（Apple / Netflix / Emby fMP4）里每个码率档是**独立的
 * `#EXT-X-STREAM-INF` + 独立 `#EXT-X-MEDIA` 音轨组**，FFmpeg 会把它们解析成
 * **N 个 program**，而 **mpv 没有 `--program` 选项** ⇒ 一次只能播**第一个** program
 * （Apple 流 = 960x540，不是最高档）。
 *
 * ⇒ 想让内核播"我们选中的那一档"，唯一可行的办法是**在代理层把 master 裁成只剩一档**，
 * 内核看到的 master 就只有一个 variant，不存在 program 选择问题。
 *
 * ## 为什么是纯函数
 * 裁剪逻辑一旦与网络/文件纠缠就没法复算。这里只吃字符串、吐字符串，
 * 单测可以直接覆盖"24 档裁成 1 档""`AVERAGE-BANDWIDTH` 不能被当成 `BANDWIDTH`"这类坑。
 *
 * ## 裁剪时保留什么
 * · 保留**所有非 `#EXT-X-STREAM-INF` 行**（`#EXTM3U`、`#EXT-X-VERSION`、
 *   `#EXT-X-MEDIA`、`#EXT-X-SESSION-KEY`、`#EXT-X-INDEPENDENT-SEGMENTS` …）
 *   —— 音频组、字幕组、加密都挂在这些标签上，删了就把流弄坏；
 * · 只保留**一个** `#EXT-X-STREAM-INF` 及其紧随的 URI 行。
 *
 * 多余的 `#EXT-X-MEDIA`（未被保留的 variant 引用的音频组）**故意不删**：
 * 它们只是声明，不影响播放，删反而容易误伤。
 */
object HlsMaster {

    private const val PREFIX = "#EXT-X-STREAM-INF:"

    /** 这段文本是不是 master（含 `#EXT-X-STREAM-INF`）。 */
    fun isMaster(text: String): Boolean = text.contains(PREFIX)

    /**
     * 解析出所有 variant（顺序即 master 里的出现顺序）。
     *
     * 容错：`#EXT-X-STREAM-INF` 与 URI 之间若混入空行/注释，跳过它们继续找 URI。
     */
    fun parse(text: String): List<HlsVariant> {
        val lines = text.lines()
        val out = ArrayList<HlsVariant>(8)
        var i = 0
        while (i < lines.size) {
            val line = lines[i]
            if (!line.startsWith(PREFIX)) {
                i++
                continue
            }
            val attrs = line.substring(PREFIX.length)
            val uriIdx = nextUriLine(lines, i + 1)
            if (uriIdx < 0) {
                i++
                continue
            }
            val res = attrValue(attrs, "RESOLUTION").orEmpty()
            out += HlsVariant(
                index = out.size,
                uri = lines[uriIdx].trim(),
                bandwidth = attrValue(attrs, "BANDWIDTH")?.toLongOrNull() ?: 0L,
                width = res.substringBefore('x').toIntOrNull() ?: 0,
                height = res.substringAfter('x', "").toIntOrNull() ?: 0,
                codecs = attrValue(attrs, "CODECS").orEmpty(),
            )
            i = uriIdx + 1
        }
        return out
    }

    /**
     * 只保留第 [keepIndex] 个 variant，返回新的 master 文本。
     *
     * [keepIndex] 越界时**原样返回**（宁可让内核自己处理，不要产出坏清单）。
     */
    fun keepOnly(text: String, keepIndex: Int): String {
        if (keepIndex < 0) return text
        val lines = text.lines()
        // 先确认这个下标存在，避免产出一个没有任何 variant 的清单。
        if (parse(text).none { it.index == keepIndex }) return text

        val out = StringBuilder(text.length)
        var idx = -1
        var i = 0
        while (i < lines.size) {
            val line = lines[i]
            if (!line.startsWith(PREFIX)) {
                out.appendLine(line)
                i++
                continue
            }
            idx++
            val uriIdx = nextUriLine(lines, i + 1)
            if (uriIdx < 0) {
                // 畸形条目：原样保留（不替内核做决定）
                out.appendLine(line)
                i++
                continue
            }
            if (idx == keepIndex) {
                out.appendLine(line)
                out.appendLine(lines[uriIdx].trim())
            }
            i = uriIdx + 1
        }
        return out.toString()
    }

    /** 从 [from] 起找第一条"既非空行、也不是标签"的行（= URI 行）；找不到返回 -1。 */
    private fun nextUriLine(lines: List<String>, from: Int): Int {
        var j = from
        while (j < lines.size) {
            val t = lines[j].trim()
            if (t.isNotEmpty() && !t.startsWith("#")) return j
            // 空行可以跳过；但遇到**另一个标签**说明这条 STREAM-INF 没有 URI，判为畸形。
            if (t.startsWith("#")) return -1
            j++
        }
        return -1
    }

    /**
     * 取属性值。
     *
     * 【必须做边界检查】`BANDWIDTH=` 是 `AVERAGE-BANDWIDTH=` 的子串 ——
     * 直接 `indexOf("BANDWIDTH=")` 会把"平均带宽"当成"峰值带宽"，
     * 选档就会偏一个数量级。所以要求匹配位置在**行首或逗号之后**。
     */
    private fun attrValue(attrs: String, key: String): String? {
        val needle = "$key="
        var searchFrom = 0
        while (true) {
            val idx = attrs.indexOf(needle, searchFrom)
            if (idx < 0) return null
            val boundaryOk = idx == 0 || attrs[idx - 1] == ','
            if (!boundaryOk) {
                searchFrom = idx + 1
                continue
            }
            val start = idx + needle.length
            if (start >= attrs.length) return ""
            return if (attrs[start] == '"') {
                val end = attrs.indexOf('"', start + 1)
                if (end < 0) attrs.substring(start + 1) else attrs.substring(start + 1, end)
            } else {
                val end = attrs.indexOf(',', start)
                if (end < 0) attrs.substring(start) else attrs.substring(start, end)
            }
        }
    }
}
