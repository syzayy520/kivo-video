package com.kivo.cinema.ui.shell

import androidx.compose.ui.unit.dp

/**
 * Shell 级避让尺寸的单一来源。
 *
 * [NavReservedHeight] = 页面内容需要为悬浮胶囊预留的底部高度。
 * ◐ 由胶囊 50dp + 距底 22dp + 手势区留白推得，沿用原 `MainActivityShell` 的 84dp，
 * 实拍量高见 `docs/audit/ui-page-parity-audit.md` SHELL-04 / NAV-06。
 */
object ShellInsets {
    val NavReservedHeight = 84.dp
}