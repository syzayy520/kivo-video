package com.kivo.cinema.ui.design.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 设置页骨架（262 从 SettingsRows 按"变化原因"拆出）：
 * 根页 = 大标题 + 滚动卡列表；子页 = 固定头（圆形白返回钮 + 居中标题）+ 独立滚动内容。
 * 行原语在 SettingsRows.kt —— 骨架管"页怎么搭"，行管"一行长什么样"，两件事分开。
 */

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

/**
 * 子页骨架：固定头不随内容滚（参考：返回钮 + 居中标题钉在顶部）。
 * 头用白玻璃底 + 发丝底线，内容在头下方独立滚动 —— 结构照搬、形态按 Android 重画。
 */
@Composable
internal fun SubPageScaffold(title: String, onBack: () -> Unit, content: @Composable ColumnScope.() -> Unit) {
    Column(Modifier.fillMaxSize()) {
        Column(
            Modifier.fillMaxWidth().statusBarsPadding().background(Color.White.copy(alpha = 0.85f)),
        ) {
            Box(Modifier.fillMaxWidth().height(52.dp)) {
                Text(
                    title, fontSize = 17.sp, fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
                    modifier = Modifier.align(Alignment.Center),
                )
                Box(
                    Modifier.align(Alignment.CenterStart).padding(start = 12.dp).size(36.dp)
                        .clip(CircleShape).background(Color.White.copy(alpha = 0.95f))
                        .clickable(role = Role.Button, onClick = onBack),
                    contentAlignment = Alignment.Center,
                ) {
                    Text("‹", fontSize = 24.sp, color = DesignTokens.textMain)
                }
            }
            Box(Modifier.fillMaxWidth().height(0.5.dp).background(DesignTokens.hairline))
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