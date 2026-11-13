package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface

/**
 * 设置行原语（268 视觉升级）：inset 分组卡 + 淡底 chip 图标 + 多行副标题 + 开关/步进/分隔/Beta。
 * 行 = 共享标题区 [RowLabels] + 可选 leading [RowIcon] + 右侧控件（五态）。原子在 SettingsRowParts.kt。
 * 新增 icon/iconColor/footnote 一律带默认值且插在 onClick/onChange 之前 —— 尾随 lambda 调用零改动。
 * 骨架（根页/子页固定头）在 SettingsScaffold.kt，按"变化原因"与本文件分离。
 */
internal val settingsPad = DesignTokens.screenPad.dp

/** 节头小字灰 + inset 分组卡。 */
@Composable
internal fun SectionCard(header: String, beta: Boolean = false, content: @Composable ColumnScope.() -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.padding(start = 4.dp, bottom = 6.dp)) {
        Text(header, fontSize = SettingsVisuals.fontSection.sp, fontWeight = FontWeight.SemiBold,
            color = DesignTokens.textSub)
        if (beta) BetaBadge()
    }
    GlassSurface(Modifier.fillMaxWidth()) { Column { content() } }
}

/** 无头分组卡：与 SectionCard 同玻璃底，仅无节头小字。 */
@Composable
internal fun PlainCard(content: @Composable ColumnScope.() -> Unit) {
    GlassSurface(Modifier.fillMaxWidth()) { Column { content() } }
}

/** 组下多行说明脚注。 */
@Composable
internal fun Footnote(text: String) {
    Text(text, fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub,
        lineHeight = 15.sp, modifier = Modifier.padding(start = 4.dp, end = 4.dp, top = 8.dp))
}

/** chevron 行：可选 chip + 标题 + 副标题 + 右 ›。enabled=false 置灰；footnote 单行原因。 */
@Composable
internal fun ChevronRow(
    title: String, subtitle: String? = null, beta: Boolean = false, enabled: Boolean = true,
    value: String? = null, valueColor: Color = DesignTokens.textSub,
    icon: ImageVector? = null, iconColor: Color = SettingsVisuals.blue, footnote: String? = null,
    onClick: () -> Unit,
) {
    RowFrame(footnote) {
        Row(
            Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
                .clickable(role = Role.Button, enabled = enabled, onClick = onClick)
                .padding(horizontal = 16.dp, vertical = 12.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (icon != null) { RowIcon(icon, iconColor, enabled); Spacer(Modifier.width(12.dp)) }
            RowLabels(title, subtitle, beta, Modifier.alpha(if (enabled) 1f else 0.4f))
            Row(verticalAlignment = Alignment.CenterVertically) {
                if (value != null) {
                    Text(value, fontSize = 15.sp, color = valueColor.copy(alpha = if (enabled) 1f else 0.5f))
                    Spacer(Modifier.width(4.dp))
                }
                Text("›", fontSize = 24.sp, color = DesignTokens.textSub.copy(alpha = if (enabled) 1f else 0.5f))
            }
        }
    }
}

/** 图标计数行：leading 淡底 chip + 标题 + 右「N/N ›」，钻入来源页。enabled=false 置灰。 */
@Composable
internal fun IconChevronRow(
    icon: String, iconColor: Color, title: String, value: String? = null, enabled: Boolean = true,
    onClick: () -> Unit,
) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
            .clickable(role = Role.Button, enabled = enabled, onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(
            Modifier.size(SettingsVisuals.chipSize.dp).clip(RoundedCornerShape(SettingsVisuals.chipRadius.dp))
                .background(SettingsVisuals.tint(iconColor).let { if (enabled) it else it.copy(alpha = 0.06f) }),
            contentAlignment = Alignment.Center,
        ) { Text(icon, fontSize = 15.sp, color = if (enabled) iconColor else iconColor.copy(alpha = 0.45f)) }
        Spacer(Modifier.width(12.dp))
        Text(title, fontSize = DesignTokens.fontRowTitle.sp,
            color = DesignTokens.textMain.copy(alpha = if (enabled) 1f else 0.4f),
            modifier = Modifier.weight(1f))
        Row(verticalAlignment = Alignment.CenterVertically) {
            if (value != null) {
                Text(value, fontSize = 15.sp, color = DesignTokens.textSub.copy(alpha = if (enabled) 1f else 0.5f))
                Spacer(Modifier.width(4.dp))
            }
            Text("›", fontSize = 24.sp, color = DesignTokens.textSub.copy(alpha = if (enabled) 1f else 0.5f))
        }
    }
}

/** 只读取值行：可选 chip + 标题（+副标题）+ 右侧真实值。无开关、不可点。 */
@Composable
internal fun ValueRow(
    title: String, value: String, subtitle: String? = null, dim: Boolean = false,
    icon: ImageVector? = null, iconColor: Color = SettingsVisuals.blue,
) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp).padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        if (icon != null) { RowIcon(icon, iconColor, !dim); Spacer(Modifier.width(12.dp)) }
        RowLabels(title, subtitle, false, Modifier.alpha(if (dim) 0.5f else 1f))
        Text(value, fontSize = 15.sp, color = DesignTokens.textSub.copy(alpha = if (dim) 0.5f else 1f))
    }
}

/** 蓝字动作行（刷新等）；enabled=false 置灰 + 非空原因副标题，不放假动作。 */
@Composable
internal fun LinkRow(title: String, subtitle: String? = null, enabled: Boolean = true, onClick: () -> Unit) {
    Column(
        Modifier.fillMaxWidth().clickable(role = Role.Button, enabled = enabled, onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
    ) {
        Text(title, fontSize = 15.sp,
            color = if (enabled) Color(0xFF007AFF) else DesignTokens.textSub.copy(alpha = 0.5f))
        if (subtitle != null)
            Text(subtitle, fontSize = 11.sp, color = DesignTokens.textSub, lineHeight = 16.sp)
    }
}

/** 只读说明行：标题 + 多行策略说明（无右值，给「敢解释」留宽度）。 */
@Composable
internal fun InfoRow(title: String, detail: String) {
    Column(Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 12.dp)) {
        Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
        Spacer(Modifier.height(3.dp))
        Text(detail, fontSize = 12.sp, color = DesignTokens.textSub, lineHeight = 17.sp)
    }
}

/** 开关行：整行可点（toggleable），右 iOS 绿/灰 Switch。可选 chip + 单行原因脚注。 */
@Composable
internal fun SwitchRow(
    title: String, checked: Boolean, subtitle: String? = null, beta: Boolean = false, enabled: Boolean = true,
    icon: ImageVector? = null, iconColor: Color = SettingsVisuals.blue, footnote: String? = null,
    onChange: (Boolean) -> Unit,
) {
    RowFrame(footnote) {
        Row(
            Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
                .toggleable(value = checked, enabled = enabled, role = Role.Switch, onValueChange = onChange)
                .padding(horizontal = 16.dp, vertical = 12.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (icon != null) { RowIcon(icon, iconColor, enabled); Spacer(Modifier.width(12.dp)) }
            RowLabels(title, subtitle, beta, Modifier.alpha(if (enabled) 1f else 0.4f))
            Switch(
                checked = checked, onCheckedChange = null, enabled = enabled,
                colors = SwitchDefaults.colors(
                    checkedTrackColor = DesignTokens.accentGreen,
                    checkedThumbColor = Color.White,
                    uncheckedTrackColor = Color(0xFFE9E9EA),
                    uncheckedThumbColor = Color.White,
                    uncheckedBorderColor = Color.Transparent,
                    disabledCheckedTrackColor = DesignTokens.accentGreen.copy(alpha = 0.4f),
                    disabledUncheckedTrackColor = Color(0xFFE9E9EA),
                ),
            )
        }
    }
}

/** 单选行：可选 chip + 标题区 + 右蓝✓（当前项）。 */
@Composable
internal fun RadioRow(
    title: String, checked: Boolean, subtitle: String? = null, beta: Boolean = false, enabled: Boolean = true,
    icon: ImageVector? = null, iconColor: Color = SettingsVisuals.blue, footnote: String? = null,
    onClick: () -> Unit,
) {
    RowFrame(footnote) {
        Row(
            Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
                .clickable(role = Role.RadioButton, enabled = enabled, onClick = onClick)
                .padding(horizontal = 16.dp, vertical = 12.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (icon != null) { RowIcon(icon, iconColor, enabled); Spacer(Modifier.width(12.dp)) }
            RowLabels(title, subtitle, beta, Modifier.alpha(if (enabled) 1f else 0.4f))
            if (checked) Text("✓", fontSize = 20.sp, fontWeight = FontWeight.Bold, color = Color(0xFF007AFF))
        }
    }
}

/** 步进行：可选 chip + 右值 + ⇕，点击弹出 SettingsPicker 菜单。 */
@Composable
internal fun StepperRow(
    title: String, value: String, subtitle: String? = null, beta: Boolean = false, enabled: Boolean = true,
    icon: ImageVector? = null, iconColor: Color = SettingsVisuals.blue, footnote: String? = null,
    onClick: () -> Unit,
) {
    RowFrame(footnote) {
        Row(
            Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
                .clickable(role = Role.Button, enabled = enabled, onClick = onClick)
                .padding(horizontal = 16.dp, vertical = 12.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (icon != null) { RowIcon(icon, iconColor, enabled); Spacer(Modifier.width(12.dp)) }
            RowLabels(title, subtitle, beta, Modifier.alpha(if (enabled) 1f else 0.4f))
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(value, fontSize = 15.sp,
                    color = if (enabled) DesignTokens.textSub else DesignTokens.textSub.copy(alpha = 0.5f))
                Spacer(Modifier.width(4.dp))
                Text("⇕", fontSize = 16.sp, color = DesignTokens.textSub)
            }
        }
    }
}