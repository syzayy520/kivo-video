package com.kivo.cinema.ui.compose.player

/**
 * 清晰度分档的**唯一规则**（v3.1f）。
 *
 * 【为什么单独一个文件】底栏徽章与资源面板 chips 曾各写一份分档：
 * 底栏按宽、面板按高 —— 1920×870 的宽银幕片源底栏报 1080P、
 * 面板报 870P，用户选着 1080P 却看到 870（真机实测投诉）。
 * 取长边分档：宽银幕裁的是高、竖屏裁的是宽，档位都留在长边上。
 *
 * 只返回标准档（4K / 1080P / 720P）；不足 720 返回空串，
 * 由各显示位自决兜底词（底栏沿用上次真值 / 面板报实际高度 / 详情页报 SD）。
 */
internal fun resolutionTier(width: Int, height: Int): String {
    val side = maxOf(width, height)
    return when {
        side >= 3800 -> "4K"
        side >= 1900 -> "1080P"
        side >= 1200 -> "720P"
        else -> ""
    }
}