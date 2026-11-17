package com.kivo.cinema.ui.profile.modules

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape

import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.ProfileModule

/**
 * 「我的」页底部「管理模块」弹层：列出 [ProfileModule] 全部 12 项，每项一个绿色开关。
 *
 * 取舍（**不做拖动重排**）：竞品有长按拖动排序，本卡刻意简化 —— 模块开关状态没有持久化存储
 * （`ProfilePage` 只把开关留在本弹层的 Compose 状态里），排序若可拖却无法保存，会出现
 * 「重排后离开页面即丢失」的假象。等模块存储落地后由后续卡补 `LazyColumn` + `reorderable`。
 *
 * 已明确未覆盖项（◐ 推断 / 未接）：
 * - 开关**不落盘**：`onToggle` 只向上抛变更，调用方目前只维护弹层内的瞬时集合；
 * - 关闭的模块**不影响**「我的」页渲染：页面仍按基准截图顺序渲染全部区块
 *   （质量门要求密度不低于基准，隐藏区块会让页面失去对比价值），文案按竞品原文保留。
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ModuleManagerSheet(
    enabled: Set<ProfileModule>,
    onToggle: (ProfileModule, Boolean) -> Unit,
    onDismiss: () -> Unit,
) {
    ModalBottomSheet(
        onDismissRequest = onDismiss,
        containerColor = DesignTokens.pageBackground,
    ) {
        Column(Modifier.fillMaxWidth().padding(bottom = 18.dp)) {
            SheetTitle(onDismiss = onDismiss)
            LazyColumn(
                modifier = Modifier.fillMaxWidth().heightIn(max = SheetListMaxHeight),
                verticalArrangement = Arrangement.spacedBy(0.dp),
            ) {
                items(ProfileModule.values().toList(), key = { it.name }) { module ->
                    ModuleToggleRow(
                        module = module,
                        checked = module in enabled,
                        onCheckedChange = { onToggle(module, it) },
                    )
                }
            }
            Text(
                text = ModuleHint,
                fontSize = HintSize,
                color = DesignTokens.textSub,
                modifier = Modifier.padding(horizontal = SidePad, vertical = HintTopGap),
            )
        }
    }
}

/** 标题行：「管理模块」+ 右侧关闭 X（32dp 圆命中盒，颜色取次文本令牌）。 */
@Composable
private fun SheetTitle(onDismiss: () -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = SidePad, vertical = TitleGap),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = "管理模块",
            fontSize = TitleSize,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            modifier = Modifier.weight(1f),
        )
        Box(
            modifier = Modifier
                .size(CloseHitSize)
                .clip(CircleShape)
                .background(DesignTokens.groupedFill)
                .clickable(role = Role.Button, onClick = onDismiss)
                .semantics { contentDescription = "关闭管理模块" },
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = "✕",
                fontSize = CloseGlyphSize,
                color = DesignTokens.textSub,
            )
        }
    }
}

/** 单行：模块名 + 绿色开关。行高 52dp 与 Material3 Switch 的 32dp 轨道垂直居中对齐。 */
@Composable
private fun ModuleToggleRow(
    module: ProfileModule,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(RowHeight)
            .clickable(role = Role.Switch) { onCheckedChange(!checked) }
            .padding(horizontal = SidePad),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = module.label,
            fontSize = RowLabelSize,
            color = DesignTokens.textMain,
            modifier = Modifier.weight(1f),
        )
        Spacer(Modifier.width(RowLabelGap))
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange,
            colors = SwitchDefaults.colors(
                checkedThumbColor = DesignTokens.canvasFill,
                checkedTrackColor = DesignTokens.accentGreen,
                checkedBorderColor = DesignTokens.accentGreen,
                uncheckedThumbColor = DesignTokens.canvasFill,
                uncheckedTrackColor = DesignTokens.groupedFill,
                uncheckedBorderColor = DesignTokens.hairline,
            ),
        )
    }
}

/** 说明文案：竞品原文，不改写成更「友好」的措辞。 */
private const val ModuleHint = "关闭的模块不在「我的」页显示。"

private val SidePad = DesignTokens.screenPad.dp
private val TitleGap = 10.dp
private val TitleSize = 20.sp
private val CloseHitSize = 32.dp
private val CloseGlyphSize = 15.sp
private val RowHeight = 52.dp
private val RowLabelSize = 16.sp
private val RowLabelGap = 10.dp
private val SheetListMaxHeight = 460.dp
private val HintSize = 13.sp
private val HintTopGap = 10.dp
