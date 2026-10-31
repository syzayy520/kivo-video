package com.kivo.cinema.evidence

import java.io.File

/**
 * 证据文件住哪、叫什么（P14-R 从 `PlayerActivity` 里搬出来）。
 *
 * ## 为什么单独一格
 *
 * 文件名是**协议的一部分**：`verify_p12_all.py` 的层 3 靠 `-t\d+\.json$` 这个后缀
 * 区分"长稳中途快照"与"静默失败"，`p12f3-<case>-failure.json` 靠前缀区分四条负向路径。
 * 这些规则原先散在 Activity 的三个方法里，改一处就漏一处 ——
 * 而漏掉的那种错是**证据互相覆盖**，跑完只剩最后一条，目录里看不出少了三条。
 *
 * 收拢之后，"名字怎么来"只在这一格答；"什么时候写"由 [SessionFinalizer] 答。
 */
class EvidencePaths(filesRoot: File) {

    /** 证据目录。`mkdirs()` 在这里做，调用方不必记得。 */
    val dir: File = File(filesRoot, "evidence").apply { mkdirs() }

    /** 主证据（解码侧或节拍侧，取决于这一轮开了什么）。 */
    fun statsFile(name: String): File = File(dir, name)

    /** A/V 合并证据。名字由 `kivo.sync` 决定，缺省是主文件去掉 `.json` 加 `-sync.json`。 */
    fun syncFile(name: String): File = File(dir, name)

    /**
     * 失败证据。**文件名带上用例名**（P12-F3）。
     *
     * 没有用例名时退回旧名 `p12b-failure.json`，保持 P12-B 起的既有约定 ——
     * 那批证据在 `docs/evidence/p12b/` 里，改名等于让判据找不到它们。
     */
    fun failureFile(caseName: String): File =
        File(dir, if (caseName.isBlank()) "p12b-failure.json" else "p12f3-$caseName-failure.json")
}
