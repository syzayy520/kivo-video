package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
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
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface

/**
 * 设置行原语（262 播放设置卡升级）：inset 分组卡 + 多行副标题 + 开关/步进/分隔/Beta。
 * 所有行共用 [RowLabels] 画标题区（标题+可选多行副标题+可选 Beta），控件只换右侧 ——
 * 四种行是"同一个标题区的四份右侧控件"，不是四套重复布局。
 * 按压反馈用 Material ripple；开关弹簧口径与 HomePreviewSetting 一致（绿/灰 iOS Switch）。
 * 骨架（根页/子页固定头）在 SettingsScaffold.kt，按"变化原因"与本文件分离。
 */
internal val settingsPad = DesignTokens.screenPad.dp

/** 节头小字灰 + inset 分组卡（set_a 帧）。 */
@Composable
internal fun SectionCard(header: String, beta: Boolean = false, content: @Composable ColumnScope.() -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.padding(start = 4.dp, bottom = 6.dp)) {
        Text(header, fontSize = 13.sp, color = DesignTokens.textSub)
        if (beta) BetaBadge()
    }
    GlassSurface(Modifier.fillMaxWidth()) { Column { content() } }
}

/** 组下多行说明脚注。 */
@Composable
internal fun Footnote(text: String) {
    Text(text, fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub,
        lineHeight = 15.sp, modifier = Modifier.padding(start = 4.dp, end = 4.dp, top = 8.dp))
}

/** 卡内行分隔线：左对齐文字（16pt 内缩），右到卡边。 */
@Composable
internal fun RowDivider() {
    Box(Modifier.fillMaxWidth().padding(start = 16.dp).height(0.5.dp).background(DesignTokens.hairline))
}

/** 橙色 Beta 徽标（参考：实验性能力右上角）。 */
@Composable
internal fun BetaBadge() {
    val orange = Color(0xFFFF9500)
    Text("Beta", fontSize = 10.sp, fontWeight = FontWeight.Bold, color = orange,
        modifier = Modifier.padding(start = 6.dp).clip(RoundedCornerShape(4.dp))
            .background(orange.copy(alpha = 0.15f)).padding(horizontal = 5.dp, vertical = 1.dp))
}

/** 行标题区：标题 17pt + 可选 Beta + 可选多行灰副标题。所有行共用。 */
@Composable
private fun RowScope.RowLabels(title: String, subtitle: String?, beta: Boolean, modifier: Modifier = Modifier) {
    Column(modifier.weight(1f).padding(end = 12.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
            if (beta) BetaBadge()
        }
        if (subtitle != null)
            Text(subtitle, fontSize = 11.sp, color = DesignTokens.textSub, lineHeight = 16.sp)
    }
}

/** chevron 行：标题 + 可选副标题 + 右 ›，按压进子页/外部导航。enabled=false 置灰（无后端钻入）。 */
@Composable
internal fun ChevronRow(
    title: String,
    subtitle: String? = null,
    beta: Boolean = false,
    enabled: Boolean = true,
    value: String? = null,
    valueColor: Color = DesignTokens.textSub,
    onClick: () -> Unit,
) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
            .clickable(role = Role.Button, enabled = enabled, onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
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

/** 只读取值行：标题（+可选副标题）+ 右侧真实值。无开关、不可点。 */
@Composable
internal fun ValueRow(title: String, value: String, subtitle: String? = null, dim: Boolean = false) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp).padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
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

/** 开关行：整行可点（toggleable），右 iOS 绿/灰 Switch。副标题可多行。 */
@Composable
internal fun SwitchRow(
    title: String,
    checked: Boolean,
    subtitle: String? = null,
    beta: Boolean = false,
    enabled: Boolean = true,
    onChange: (Boolean) -> Unit,
) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
            .toggleable(value = checked, enabled = enabled, role = Role.Switch, onValueChange = onChange)
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
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

/** 单选行：标题区 + 右蓝✓（当前项），内核选择等 radio 组形态。 */
@Composable
internal fun RadioRow(
    title: String,
    checked: Boolean,
    subtitle: String? = null,
    beta: Boolean = false,
    enabled: Boolean = true,
    onClick: () -> Unit,
) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
            .clickable(role = Role.RadioButton, enabled = enabled, onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        RowLabels(title, subtitle, beta, Modifier.alpha(if (enabled) 1f else 0.4f))
        if (checked) Text("✓", fontSize = 20.sp, fontWeight = FontWeight.Bold, color = Color(0xFF007AFF))
    }
}

/** 步进行：右值 + ⇕，点击弹出 SettingsPicker 菜单。副标题可多行。 */
@Composable
internal fun StepperRow(
    title: String,
    value: String,
    subtitle: String? = null,
    beta: Boolean = false,
    enabled: Boolean = true,
    onClick: () -> Unit,
) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = DesignTokens.rowHeight.dp)
            .clickable(role = Role.Button, enabled = enabled, onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        RowLabels(title, subtitle, beta, Modifier.alpha(if (enabled) 1f else 0.4f))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(value, fontSize = 15.sp,
                color = if (enabled) DesignTokens.textSub else DesignTokens.textSub.copy(alpha = 0.5f))
            Spacer(Modifier.width(4.dp))
            Text("⇕", fontSize = 16.sp, color = DesignTokens.textSub)
        }
    }
}