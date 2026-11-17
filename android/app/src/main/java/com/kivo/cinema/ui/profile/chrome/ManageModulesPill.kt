package com.kivo.cinema.ui.profile.chrome

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
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

/**
 * 「我的」页底部的「管理模块」白胶囊：居中、白底、高 40dp、圆角 20dp（基准 `profile-4.png`）。
 *
 * 独立成文件只为守住「页面编排 ≤300 行」：本组件没有任何页面状态，回调即点击。
 */
@Composable
fun ManageModulesPill(onClick: () -> Unit, modifier: Modifier = Modifier) {
    Row(
        modifier = Modifier.fillMaxWidth().then(modifier),
        horizontalArrangement = Arrangement.Center,
    ) {
        Box(
            modifier = Modifier
                .height(PillHeight)
                .widthIn(min = PillMinWidth)
                .clip(RoundedCornerShape(PillRadius))
                .background(DesignTokens.canvasFill)
                .clickable(role = Role.Button, onClick = onClick)
                .padding(horizontal = PillSidePad)
                .semantics { contentDescription = "管理模块" },
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = "管理模块",
                fontSize = PillLabelSize,
                fontWeight = FontWeight.Medium,
                color = DesignTokens.textMain,
            )
        }
    }
}

private val PillHeight = 40.dp
private val PillRadius = 20.dp
private val PillMinWidth = 148.dp
private val PillSidePad = 20.dp
private val PillLabelSize = 15.sp