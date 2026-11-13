package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.AppearancePrefs

/** 导航钻入（267）：1 真 3 灰开关 + 主 Tab 只读四行。不放假拖拽柄（266 教训）。 */
@Composable
internal fun SettingsAppearanceNav(back: () -> Unit) {
    val context = LocalContext.current
    LaunchedEffect(Unit) { AppearancePrefs.bind(context) }
    SubPageScaffold("导航", back) {
        PlainCard {
            SwitchRow("自定义菜单", false, enabled = false,
                subtitle = "Tab 集合固定，自定义菜单未接入。", onChange = {})
            RowDivider()
            SwitchRow("增强图标视觉", false, enabled = false,
                subtitle = "仅一套默认线框图标，无增强视觉包。", onChange = {})
            RowDivider()
            SwitchRow("隐藏标题", AppearancePrefs.hideNavLabels,
                onChange = { AppearancePrefs.updateHideNavLabels(it) })
            RowDivider()
            SwitchRow("首页图标颜色自适应", false, enabled = false,
                subtitle = "导航图标固定深色，内容取色未接入。", onChange = {})
        }
        Spacer(Modifier.height(14.dp))
        SectionCard("主 Tab") {
            listOf("发现", "我的", "资源库", "设置").forEachIndexed { index, label ->
                if (index > 0) RowDivider()
                ValueRow(label, "")
            }
        }
        Footnote("顺序固定，拖拽排序未接入。")
    }
}