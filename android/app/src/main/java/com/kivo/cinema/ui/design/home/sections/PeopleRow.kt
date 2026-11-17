package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbPerson
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/**
 * 热门人物横滑行（竞品帧 IMG_0334 的人物区）。
 *
 * 设计取舍：
 * - 标题由调用方绘制（本组件只管行），便于与首页其它节共用 `HomeSectionHeader`；
 * - 头像 72dp 圆形 ✅ 实测帧内头像直径 ≈ 帧宽 72/390，`CircleShape` 裁切不留方角；
 * - 姓名 13sp 粗、knownFor 11sp 次色 ◐ 推断：帧内副行在缩略状态下只能辨形，字号取邻近
 *   `TmdbPosterCard` 的 12/11 阶梯；
 * - 间距 16dp ✅（帧内头像列间距约为头像直径的 22%），左 padding 13dp 与 `screenPad` 对齐。
 *
 * 空列表：不渲染任何东西（连空行也不占位），避免首页出现无标题的空白带。
 */
@Composable
fun PeopleRow(people: List<TmdbPerson>, fg: Color, onOpen: (TmdbPerson) -> Unit) {
    if (people.isEmpty()) return
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(16.dp),
        contentPadding = PaddingValues(horizontal = 13.dp),
    ) {
        items(people) { person -> PersonCell(person, fg, onOpen) }
    }
}

/** 单个人物单元：圆形头像 + 两行文字，宽度锁死 72dp 使名字换行不至于撑歪列宽。 */
@Composable
private fun PersonCell(person: TmdbPerson, fg: Color, onOpen: (TmdbPerson) -> Unit) {
    Column(
        Modifier.width(72.dp).clickable { onOpen(person) },
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        TmdbImage(
            url = person.profileUrl.orEmpty(),
            contentDescription = person.name,
            modifier = Modifier.size(72.dp).clip(CircleShape),
        )
        Spacer(Modifier.height(6.dp))
        Text(
            person.name,
            fontSize = 13.sp,
            fontWeight = FontWeight.Bold,
            color = fg,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            textAlign = TextAlign.Center,
        )
        Text(
            person.knownFor,
            fontSize = 11.sp,
            color = DesignTokens.textSub,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
            textAlign = TextAlign.Center,
        )
    }
}

private fun previewPerson(id: Int, name: String, knownFor: String) = TmdbPerson(
    id = id,
    name = name,
    profileUrl = null,
    knownFor = knownFor,
)

@Preview(name = "PeopleRow 空 / 2 条", widthDp = 390, heightDp = 200, showBackground = true)
@Composable
private fun PeopleRowPreview() {
    Column(Modifier.background(DesignTokens.pageBackground)) {
        PeopleRow(emptyList(), DesignTokens.textMain) {}
        Spacer(Modifier.height(12.dp))
        PeopleRow(
            people = listOf(
                previewPerson(1, "汤姆·哈迪", "演员 · 主演"),
                previewPerson(2, "赞达亚", "演员 · 主演"),
            ),
            fg = DesignTokens.textMain,
            onOpen = {},
        )
    }
}