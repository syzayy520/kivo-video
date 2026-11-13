package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
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
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 设置页骨架（268 视觉升级）：根页大标题 32/800 + 滚动卡列表 + 版本脚注；
 * 子页固定头（淡紫 tint 返回钮 + 居中标题）+ 独立滚动内容。
 * 行原语在 SettingsRows.kt —— 骨架管"页怎么搭"，行管"一行长什么样"，两件事分开。
 */

/** 根页骨架：大标题 32pt/800 + 竖向滚动 inset 卡列表。 */
@Composable
internal fun SettingsScaffold(title: String, content: @Composable ColumnScope.() -> Unit) {
    Column(
        Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = settingsPad),
    ) {
        Spacer(Modifier.height(10.dp))
        Text(title, fontSize = SettingsVisuals.fontRootTitle.sp, fontWeight = FontWeight.ExtraBold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(16.dp))
        content()
        Spacer(Modifier.height(24.dp))
    }
}

/** 版本脚注：左品牌灰字，右真实版本（等宽数字 tnum）。读不到版本显示"未知"，不编默认。 */
@Composable
internal fun VersionFootnote(version: String) {
    Row(
        Modifier.fillMaxWidth().padding(horizontal = 4.dp, vertical = 10.dp),
        horizontalArrangement = Arrangement.SpaceBetween, verticalAlignment = Alignment.CenterVertically,
    ) {
        Text("Kivo Video", fontSize = 12.sp, color = SettingsVisuals.sub)
        Text(version, fontSize = 12.sp, color = SettingsVisuals.sub,
            style = TextStyle(fontFeatureSettings = "tnum"))
    }
}

/**
 * 子页骨架（v3）：固定头不随内容滚（蓝 tint 返回圆钮 + 左对齐标题 22/700）。
 * 头与内容同处统一冷灰画布（外层 AnimatedGradientBg flat），无玻璃带；内容在头下方独立滚动。
 */
@Composable
internal fun SubPageScaffold(title: String, onBack: () -> Unit, content: @Composable ColumnScope.() -> Unit) {
    Column(Modifier.fillMaxSize()) {
        Column(Modifier.fillMaxWidth().statusBarsPadding()) {
            Row(
                Modifier.fillMaxWidth().height(52.dp).padding(horizontal = 12.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Box(
                    Modifier.size(36.dp).clip(CircleShape)
                        .background(SettingsVisuals.blue.copy(alpha = 0.12f))
                        .clickable(role = Role.Button, onClick = onBack),
                    contentAlignment = Alignment.Center,
                ) {
                    Text("‹", fontSize = 24.sp, color = SettingsVisuals.blue)
                }
                Spacer(Modifier.width(12.dp))
                Text(title, fontSize = 22.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
            }
            Box(Modifier.fillMaxWidth().height(0.5.dp).background(SettingsVisuals.hairline))
        }
        Column(
            Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = settingsPad),
        ) {
            Spacer(Modifier.height(14.dp))
            content()
            Spacer(Modifier.height(24.dp))
        }
    }
}