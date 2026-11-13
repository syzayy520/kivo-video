package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/** 资源卡片 Badge 钻入（267）：全灰+原因。不放假输入框/假预览/假计数（267 D7）。 */
@Composable
internal fun SettingsAppearanceBadge(back: () -> Unit) {
    SubPageScaffold("资源卡片 Badge", back) {
        PlainCard {
            InfoRow("添加 Badge 包",
                "Badge 包下载与规则校验未接入：无法验证 URL、无法保存规则包，故不提供输入框与添加按钮。")
        }
        Footnote("只有下载和规则校验全部成功后才会保存；当前两项能力均未接入。")
        Spacer(Modifier.height(14.dp))
        PlainCard {
            SwitchRow("黑白图标自适应", false, enabled = false,
                subtitle = "逐张检测未接入：无法判定纯黑/纯白图标，保持原样。", onChange = {})
        }
    }
}