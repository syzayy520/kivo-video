package com.kivo.cinema.mpv

/**
 * 253-P6-3：内核轨道目录 —— **只做一件事**：把 mpv 的轨道列表变成 Kotlin 可用的结构。
 *
 * ## 它解决什么
 * 换内核后，"有哪几条音轨、哪几条字幕轨、当前选的是哪条"这套信息
 * 只能从内核问出来。mpv 的 `track-list` 是一个**属性树**：
 *
 * ```
 * track-list/count              → 轨道总数
 * track-list/<i>/id             → 轨道 id（选轨时用这个，不是下标）
 * track-list/<i>/type           → video / audio / sub
 * track-list/<i>/title          → 轨道标题
 * track-list/<i>/lang           → 语言标签
 * track-list/<i>/selected       → 当前是否选中
 * track-list/<i>/default        → 是否是默认轨
 * track-list/<i>/codec          → 编码名
 * track-list/<i>/external       → 是否外挂
 * ```
 *
 * 【为什么用属性路径逐项读，而不是让 native 把 track-list 解析成 JSON】
 * 属性路径是 mpv 自己定义并保证的语法；自己写一遍 JSON 序列化，
 * 就多一处**格式会漂移**的地方（mpv 改字段名我们就跟着坏）。
 * 逐项读的代价是多几次 JNI 调用 —— 一条片源通常 3~6 条轨，
 * 几十次调用可以忽略。
 *
 * ## 为什么选轨用 id 而不是下标
 * `track-list/<i>` 的**下标会变**（换文件、外部字幕加入后都会重排），
 * 而 `id` 是 mpv 为这条轨道分配的稳定标识。
 * 选轨必须用 id —— 界面上记的"用户选了哪条"要能跨会话还原。
 */
class MpvTrackCatalog(private val session: MpvSession) {

    /** 一条轨道。字段与 mpv 的 track-list 子项一一对应。 */
    data class Track(
        val id: Int,
        val type: String,
        val title: String,
        val lang: String,
        val selected: Boolean,
        val isDefault: Boolean,
        val codec: String,
        val external: Boolean,
    ) {
        /** 界面上显示的名字：有标题用标题，否则用语言，否则用类型 + 序号。 */
        fun displayName(fallbackIndex: Int): String = when {
            title.isNotBlank() -> title
            lang.isNotBlank() -> lang
            else -> "$type #$fallbackIndex"
        }
    }

    /** 全部轨道。读不出来时返回空列表（媒体还没加载时就是这个状态）。 */
    fun tracks(): List<Track> {
        val count = session.property("track-list/count")?.toIntOrNull() ?: return emptyList()
        if (count <= 0) return emptyList()
        val result = ArrayList<Track>(count)
        for (index in 0 until count) {
            val id = session.property("track-list/$index/id")?.toIntOrNull() ?: continue
            result += Track(
                id = id,
                type = session.property("track-list/$index/type").orEmpty(),
                title = session.property("track-list/$index/title").orEmpty(),
                lang = session.property("track-list/$index/lang").orEmpty(),
                selected = session.property("track-list/$index/selected") == "yes",
                isDefault = session.property("track-list/$index/default") == "yes",
                codec = session.property("track-list/$index/codec").orEmpty(),
                external = session.property("track-list/$index/external") == "yes",
            )
        }
        return result
    }

    /** 某一类轨道（"video" / "audio" / "sub"）。 */
    fun tracksOfType(type: String): List<Track> = tracks().filter { it.type == type }

    /** 音轨列表。 */
    fun audioTracks(): List<Track> = tracksOfType("audio")

    /** 字幕轨列表。 */
    fun subtitleTracks(): List<Track> = tracksOfType("sub")

    /**
     * 选中一条轨道。
     *
     * @param type mpv 的轨道类型名（"audio" / "sub" / "video"）。
     * @param id 轨道 id（来自 [tracks]）。
     * @return true = 选中成功。
     *
     * 【为什么按类型分别设，而不是统一设一个属性】
     * mpv 的选轨属性是 `aid` / `sid` / `vid` 三个独立属性，
     * 分别对应三类轨的 id。写 `aid=3` 只影响音轨。
     */
    fun selectTrack(type: String, id: Int): Boolean {
        val property = when (type) {
            "audio" -> "aid"
            "sub" -> "sid"
            "video" -> "vid"
            else -> return false
        }
        return session.setProperty(property, id.toString())
    }

    /**
     * 关闭某一类轨道（例如关掉字幕）。
     *
     * mpv 里"关闭"的写法是把该类型的属性设成 `no`。
     */
    fun disableTrack(type: String): Boolean {
        val property = when (type) {
            "audio" -> "aid"
            "sub" -> "sid"
            "video" -> "vid"
            else -> return false
        }
        return session.setProperty(property, "no")
    }

    /** 当前选中的轨道 id。读不到返回 -1。 */
    fun currentTrackId(type: String): Int {
        val property = when (type) {
            "audio" -> "aid"
            "sub" -> "sid"
            "video" -> "vid"
            else -> return -1
        }
        return session.property(property)?.toIntOrNull() ?: -1
    }

    /**
     * 挂载一条外挂字幕（.srt / .ass）。
     *
     * @param path 本地文件路径。mpv 自己去读这个文件。
     * @return true = 命令下发成功。
     *
     * 【为什么不自己解析字幕再画】
     * mpv 已经把字幕画进视频帧了（vo=libmpv → osd → libass）。
     * 我们若再解析一遍再画一遍，就是两套字幕同时显示、两个位置。
     * 外挂字幕的正确接法就是把文件交给 mpv（`sub-add` 命令）。
     */
    fun addExternalSubtitle(path: String): Boolean =
        session.command("sub-add", path)

    /** 卸载外挂字幕。 */
    fun removeExternalSubtitle(id: Int): Boolean =
        session.command("sub-remove", id.toString())

    /** 当前外挂字幕的 id 列表（mpv 的 `sid` 列表里 external=yes 的那些）。 */
    fun externalSubtitleIds(): List<Int> =
        subtitleTracks().filter { it.external }.map { it.id }
}
