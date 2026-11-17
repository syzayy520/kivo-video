package com.kivo.cinema.ui.components

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/** 节标题层级：Primary 用于页面主区块，Secondary 用于页面内子区块。 */
enum class SectionLevel { Primary, Secondary }

/** 区块标题：左标题 + 可选右侧「›」，标题与展开入口共用同一套排版。 */
@Composable
fun SectionHeader(
    title: String,
    level: SectionLevel = SectionLevel.Primary,
    onMore: (() -> Unit)? = null,
    color: Color = DesignTokens.textMain,
) {
    val titleSize = when (level) {
        SectionLevel.Primary -> PrimaryTitleSize
        SectionLevel.Secondary -> SecondaryTitleSize
    }
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = title,
            fontSize = titleSize,
            fontWeight = FontWeight.Bold,
            color = color,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            modifier = Modifier.weight(1f),
        )
        if (onMore != null) {
            Box(
                modifier = Modifier
                    .size(MoreHitWidth, MoreHitHeight)
                    .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
                    .clickable(role = Role.Button, onClick = onMore)
                    .semantics(mergeDescendants = true) {
                        contentDescription = "查看全部$title"
                    },
                contentAlignment = Alignment.Center,
            ) {
                Text(
                    text = "›",
                    fontSize = MoreGlyphSize,
                    fontWeight = FontWeight.Bold,
                    color = color,
                    modifier = Modifier.clearAndSetSemantics { },
                )
            }
        }
    }
}

// 节标题字号按 REX 实拍统一到 17sp（唯一一档 17 Bold），层级靠颜色与间距区分，不靠字号。
// DesignTokens.fontSectionHeader 为 22f，本组件不引用它。
private val PrimaryTitleSize = 17.sp
private val SecondaryTitleSize = 17.sp
// 右侧「›」的 28×22dp 命中盒与字形大小来自任务规格，令牌无对应项。
private val MoreHitWidth = 28.dp
private val MoreHitHeight = 22.dp
private val MoreGlyphSize = 20.sp