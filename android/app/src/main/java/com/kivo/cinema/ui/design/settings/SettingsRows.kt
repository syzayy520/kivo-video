package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface

/**
 * 设置行原语（261 设计 §三）：inset 分组卡 59pt 行 + 节头 + 多行脚注"敢解释"。
 * 退役自 Pages.kt 的 GroupSection/GroupRow，复用 GlassSurface/DesignTokens，不新建重复组件。
 * 按压反馈用 Material ripple（对齐 #15 已验收口径）；开关弹簧由 HomePreviewSetting 承载。
 */
internal val settingsPad = DesignTokens.screenPad.dp

/** 节头小字灰 + inset 分组卡（set_a 帧）。 */
@Composable
internal fun SectionCard(header: String, content: @Composable ColumnScope.() -> Unit) {
    Text(header, fontSize = 13.sp, color = DesignTokens.textSub,
        modifier = Modifier.padding(start = 4.dp, bottom = 6.dp))
    GlassSurface(Modifier.fillMaxWidth()) { Column { content() } }
}

/** 组下多行说明脚注。 */
@Composable
internal fun Footnote(text: String) {
    Text(text, fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub,
        lineHeight = 15.sp, modifier = Modifier.padding(start = 4.dp, end = 4.dp, top = 8.dp))
}

/** chevron 行：标题 + 可选副标题 + 右 ›，按压进子页/外部导航。 */
@Composable
internal fun ChevronRow(title: String, subtitle: String? = null, onClick: () -> Unit) {
    Row(
        Modifier.fillMaxWidth().height(DesignTokens.rowHeight.dp)
            .clickable(role = Role.Button, onClick = onClick).padding(horizontal = 16.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(Modifier.weight(1f).padding(end = 12.dp)) {
            Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
            if (subtitle != null)
                Text(subtitle, fontSize = 11.sp, color = DesignTokens.textSub, maxLines = 1)
        }
        Text("›", fontSize = 24.sp, color = DesignTokens.textSub)
    }
}

/** 只读取值行：标题（+可选副标题）+ 右侧真实值。无开关、不可点。 */
@Composable
internal fun ValueRow(title: String, value: String, subtitle: String? = null) {
    Row(
        Modifier.fillMaxWidth().height(DesignTokens.rowHeight.dp).padding(horizontal = 16.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(Modifier.weight(1f).padding(end = 12.dp)) {
            Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
            if (subtitle != null)
                Text(subtitle, fontSize = 11.sp, color = DesignTokens.textSub, maxLines = 1)
        }
        Text(value, fontSize = 15.sp, color = DesignTokens.textSub)
    }
}

/** 只读说明行：标题 + 多行策略说明（无右值，给「敢解释」留宽度）。 */
@Composable
internal fun InfoRow(title: String, detail: String) {
    Column(
        Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 12.dp),
    ) {
        Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
        Spacer(Modifier.height(3.dp))
        Text(detail, fontSize = 12.sp, color = DesignTokens.textSub, lineHeight = 17.sp)
    }
}

/** 根页骨架：大标题 28pt Bold + 竖向滚动 inset 卡列表。 */
@Composable
internal fun SettingsScaffold(title: String, content: @Composable ColumnScope.() -> Unit) {
    Column(
        Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = settingsPad),
    ) {
        Spacer(Modifier.height(8.dp))
        Text(title, fontSize = 28.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
        Spacer(Modifier.height(14.dp))
        content()
        Spacer(Modifier.height(24.dp))
    }
}

/** 子页骨架：返回头 + 竖向滚动内容（手机单栏 push/pop）。 */
@Composable
internal fun SubPageScaffold(title: String, onBack: () -> Unit, content: @Composable ColumnScope.() -> Unit) {
    Column(
        Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = settingsPad),
    ) {
        Spacer(Modifier.height(8.dp))
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Text("‹", fontSize = 32.sp, color = DesignTokens.textMain,
                modifier = Modifier.clip(CircleShape)
                    .clickable(role = Role.Button, onClick = onBack).padding(horizontal = 6.dp))
            Spacer(Modifier.width(4.dp))
            Text(title, fontSize = 22.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
        }
        Spacer(Modifier.height(10.dp))
        content()
        Spacer(Modifier.height(24.dp))
    }
}