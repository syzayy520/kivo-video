package com.kivo.cinema.smb.scan

/**
 * NAS 片库扫描的准入规则。
 *
 * 单一职责：只回答"这个文件算不算影片""这个目录要不要进"——纯字符串判断，不碰 IO。
 *
 * 【为什么需要它】真机摸过 NAS 结构：`12T/` 下混着 `电影` `剧集` `动漫` `REMUX`
 * 与 `字幕` `学习` `ipa备份` `BT源` `#recycle` 等一堆非影视目录。全盘扫会把它们一起拖进来，
 * 既慢又脏，所以目录与文件都要先过这道滤网。
 */
object VideoFileFilter {

    /** 能当影片播的容器；不含 iso（原盘镜像抽帧/播放另走一路）。 */
    private val VIDEO_EXT = setOf(
        "mkv", "mp4", "m4v", "avi", "ts", "m2ts", "mov", "wmv", "flv", "rmvb", "rm", "webm", "mpg", "mpeg",
    )

    /** 一定不是影视库的目录名（真机实测存在）。命中即整棵跳过。 */
    private val SKIP_DIRS = setOf(
        "#recycle", "字幕", "学习", "ipa备份", "PTP发种", "BT源", "yt-dl", "zzz", "临时", "其他", "特效",
        "web", "web_packages", "home", "homes", "CloudNAS", "Download", "PlexMediaServer", "Bitwarden密码库",
        "lost+found", "System Volume Information",
    )

    fun isVideo(name: String): Boolean =
        name.substringAfterLast('.', "").lowercase() in VIDEO_EXT

    /** 样片/花絮/隐藏文件：体积小又杂，进首页只会稀释内容。 */
    fun isNoise(name: String): Boolean {
        val lower = name.lowercase()
        return name.startsWith(".") || lower.contains("sample") || lower.contains("trailer") ||
            lower.startsWith("www.") || lower.endsWith(".url") || lower.endsWith(".txt")
    }

    fun skipDir(name: String): Boolean =
        name in SKIP_DIRS || name.startsWith(".") || name.startsWith("#") || name.startsWith("@")
}
