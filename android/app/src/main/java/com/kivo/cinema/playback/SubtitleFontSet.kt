package com.kivo.cinema.playback

import android.content.Context
import android.system.Os
import com.kivo.cinema.net.NetDiag
import java.io.File

/**
 * 273 v20：字幕字体集最小化 —— **只做一件事**：把内核要用的字体目录从
 * "整个 /system/fonts"缩成"只含必要字体的应用私有目录"。
 *
 * ## 为什么非做不可
 * 我们编的 libass 没有 `CONFIG_FONTCONFIG`（没有字体提供器），mpv 把
 * `sub-fonts-dir` 交给 libass 的 `ass_set_fonts_dir()`，它对该目录里的**每个条目**
 * 调一次 `ass_add_font()`，而 FreeType 的 `FT_New_Face` 会把整个文件读进内存。
 *
 * 真机实测（Redmi K40）：`sub-fonts-dir=/system/fonts` ⇒ 内核日志 **242 条**
 * `Loading font file` ⇒ 239 个字体文件（磁盘合计 **125.6 MB**）全部常驻原生堆，
 * 起播期原生堆从 13 MB 跳到 205 MB（其中 125.6 MB 是字体）。
 *
 * ## 修法
 * 目录里只放**符号链接**（不复制 18.6 MB）：libass 走 `FT_New_Face(path)`，
 * 操作系统会跟着链接走 ⇒ 零拷贝、零重复占用。
 *
 * ## 为什么符号链接一定被 libass 认
 * `/system/fonts` 自己就带符号链接（`DroidSans.ttf -> Roboto-Regular.ttf`），
 * 而真机日志里 **242 条 = 239 个真实文件 + 3 个符号链接** ⇒ libass 逐条目装载，
 * 不跳过符号链接。
 *
 * ## 兜底
 * 一个候选都找不到 ⇒ 退回旧的 `/system/fonts` 行为，并把原因写进
 * `net-diag.log`（**不许静默降级**）。退回后字幕照常显示，只是多占约 125 MB。
 */
internal object SubtitleFontSet {

    /**
     * 准备结果。
     *
     * @param dir 给内核的 `sub-fonts-dir`；空 = 不设字体选项。
     * @param family 给内核的 `osd-font` / `sub-font`。
     * @param linked 目录里实际链接了哪些字体文件（取证用）。
     * @param reason 非空 = 走了兜底或降级，原因必须可查。
     */
    class Prepared(
        val dir: String,
        val family: String,
        val linked: List<String>,
        val reason: String,
    )

    /**
     * 主字体候选表：按优先级取**第一个存在**的。
     *
     * 族名 = 该文件 **0 号 face** 的族名 —— libass 加载 TTC 时只取 0 号 face
     * （libass 的 `ass_font.c`：`FT_Open_Face` 的 index 来自选中的那一张），
     * 而 Android 的 `NotoSansCJK-Regular.ttc` 0 号 face 是 JP。
     * 字形层面无损失：CJK 全字集每个 face 都带，中文照样显示。
     */
    private class Candidate(val path: String, val family: String)

    private val PRIMARY_CANDIDATES = listOf(
        Candidate("/system/fonts/NotoSansCJK-Regular.ttc", "Noto Sans CJK JP"),
        Candidate("/system/fonts/NotoSerifCJK-Regular.ttc", "Noto Serif CJK JP"),
        Candidate("/system/fonts/DroidSansFallback.ttf", "Droid Sans Fallback"),
        Candidate("/system/fonts/DroidSansFallbackFull.ttf", "Droid Sans Fallback"),
        Candidate("/system/fonts/MiSansVF.ttf", "MiSans"),
    )

    /**
     * 附加字体：主字体缺字时由 libass 逐字回退。
     *
     * 只留 emoji 一个：Noto Sans CJK 已含拉丁字母、标点、假名，
     * 唯一真正的缺口是非中日韩字形（表情符号）。
     */
    private val EXTRA_FONTS = listOf("/system/fonts/NotoColorEmoji.ttf")

    /** 兜底目录：一个候选都找不到时才用（旧行为，多占约 125 MB 原生内存）。 */
    private const val FALLBACK_DIR = "/system/fonts"
    private const val FALLBACK_FAMILY = "Noto Sans CJK JP"

    /**
     * 准备最小字体目录。
     *
     * 幂等：重复调用会重建目录里的链接（不会堆积残留）。
     */
    fun prepare(context: Context): Prepared {
        val primary = PRIMARY_CANDIDATES.firstOrNull { File(it.path).exists() }
        if (primary == null) {
            val reason = "系统里没有可用的 CJK 字体文件，退回整目录 $FALLBACK_DIR" +
                "（会多占约 125 MB 原生内存）"
            NetDiag.log("[subfonts] $reason")
            return Prepared(FALLBACK_DIR, FALLBACK_FAMILY, emptyList(), reason)
        }

        val dir = File(context.filesDir, "subfonts")
        if (!dir.exists() && !dir.mkdirs()) {
            val reason = "建字体目录失败：${dir.absolutePath}"
            NetDiag.log("[subfonts] $reason")
            return Prepared(FALLBACK_DIR, FALLBACK_FAMILY, emptyList(), reason)
        }
        clearDirectory(dir)

        val linked = mutableListOf<String>()
        if (!link(dir, primary.path, primary.path.substringAfterLast('/'))) {
            val reason = "字体符号链接失败：${primary.path}"
            NetDiag.log("[subfonts] $reason")
            return Prepared(FALLBACK_DIR, FALLBACK_FAMILY, emptyList(), reason)
        }
        linked += primary.path.substringAfterLast('/')
        for (extra in EXTRA_FONTS) {
            if (!File(extra).exists()) continue
            val name = extra.substringAfterLast('/')
            if (link(dir, extra, name)) linked += name
        }

        val reason = ""
        NetDiag.log("[subfonts] 字体目录=${dir.absolutePath} 族名=${primary.family} 链接=$linked")
        return Prepared(dir.absolutePath, primary.family, linked, reason)
    }

    /** 链接一个字体文件到字体目录（已存在同名链接则先删）。 */
    private fun link(dir: File, target: String, name: String): Boolean {
        val link = File(dir, name)
        if (link.exists() || isSymlink(link)) {
            link.delete()
        }
        return runCatching { Os.symlink(target, link.absolutePath) }.isSuccess
    }

    /** 清掉目录里的旧内容（重建前做，避免上一轮的字体残留）。 */
    private fun clearDirectory(dir: File) {
        val entries = dir.listFiles() ?: return
        for (entry in entries) {
            entry.delete()
        }
    }

    /** 符号链接本身存在但目标没了时，`File.exists()` 返回 false —— 要单独判。 */
    private fun isSymlink(file: File): Boolean =
        runCatching {
            java.nio.file.Files.readAttributes(
                file.toPath(),
                java.nio.file.attribute.BasicFileAttributes::class.java,
                java.nio.file.LinkOption.NOFOLLOW_LINKS,
            ).isSymbolicLink
        }.getOrDefault(false)
}
