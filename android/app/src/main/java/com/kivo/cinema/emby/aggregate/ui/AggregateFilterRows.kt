package com.kivo.cinema.emby.aggregate.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.aggregate.AggregateFilter
import com.kivo.cinema.emby.aggregate.AggregateSearchModel
import com.kivo.cinema.emby.aggregate.AggregateSource

/** 一个筛选 chip 的数据：key + 显示名 + 是否选中。六排共用，不写六套控件。 */
private data class Chip(val key: String, val label: String, val selected: Boolean)

/**
 * 256 六排筛选：媒体库(多选)/类别/排序/类型/地区/年份。一个 [ChipRow] 渲染六份数据。
 * 宽屏 = 左 label 列 + FlowRow 换行（平板参考形态）；窄屏 = 单行左滑（手机参考形态：
 * 选中浅底 pill、未选中纯文本，类型/地区/年份起头浅色 label pill，媒体库排尾部 label pill，无「不限」chip）。
 * 单一职责：把模型选择态画成 chip 排并回写；翻译逻辑在 AggregateFilter。
 */
@Composable
fun AggregateFilterRows(model: AggregateSearchModel, sources: List<AggregateSource>, wide: Boolean) {
    Column(verticalArrangement = Arrangement.spacedBy(if (wide) 10.dp else 4.dp)) {
        ChipRow("媒体库", sources.map { Chip(it.id, it.name, it.id in model.selectedIds) }, wide, false, true) {
            model.toggleSource(it)
        }
        ChipRow("全部", AggregateFilter.categories.mapIndexed { i, c -> Chip("$i", c.label, i == model.categoryIndex) }, wide, false) {
            model.setCategory(it.toInt())
        }
        ChipRow("排序", AggregateFilter.sortings.mapIndexed { i, s -> Chip("$i", s.label, i == model.sortingIndex) }, wide, false) {
            model.setSorting(it.toInt())
        }
        ChipRow("类型", (if (wide) listOf(Chip("none", "不限", model.genre == null)) else emptyList()) +
            AggregateFilter.genres.map { Chip(it, it, model.genre == it) }, wide, true) {
            model.pickGenre(if (it == "none" || model.genre == it) null else it)
        }
        ChipRow("地区", (if (wide) listOf(Chip("none", "不限", model.region == null)) else emptyList()) +
            AggregateFilter.regions.map { Chip(it, it, model.region == it) }, wide, true) {
            model.pickRegion(if (it == "none" || model.region == it) null else it)
        }
        ChipRow("年份", (if (wide) listOf(Chip("none", "不限", model.yearIndex == null)) else emptyList()) +
            AggregateFilter.yearOptions.mapIndexed { i, y -> Chip("$i", y.label, i == model.yearIndex) }, wide, true) {
            model.setYear(if (it == "none") null else it.toInt().takeIf { i -> i != model.yearIndex })
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun ChipRow(label: String, chips: List<Chip>, wide: Boolean, labelPill: Boolean,
                    labelAtEnd: Boolean = false, onClick: (String) -> Unit) {
    if (wide) {
        Row(verticalAlignment = Alignment.Top) {
            Text(label, fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.width(52.dp).padding(top = 6.dp))
            FlowRow(modifier = Modifier.weight(1f),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)) {
                chips.forEach { chip ->
                    val bg = if (chip.selected) MaterialTheme.colorScheme.primary else Color.White
                    val fg = if (chip.selected) Color.White else MaterialTheme.colorScheme.onSurfaceVariant
                    Text(chip.label, fontSize = 12.sp, color = fg,
                        fontWeight = if (chip.selected) FontWeight.Medium else FontWeight.Normal,
                        modifier = Modifier.clip(RoundedCornerShape(16.dp)).background(bg)
                            .clickable(interactionSource = remember { MutableInteractionSource() },
                                indication = null) { onClick(chip.key) }
                            .padding(horizontal = 12.dp, vertical = 6.dp))
                }
            }
        }
    } else {
        Row(Modifier.horizontalScroll(rememberScrollState()), verticalAlignment = Alignment.CenterVertically) {
            if (labelPill) {
                LabelPill(label)
                Spacer(Modifier.width(8.dp))
            }
            chips.forEach { chip ->
                val tint = MaterialTheme.colorScheme.primary
                Text(chip.label, fontSize = 13.sp,
                    color = if (chip.selected) tint else MaterialTheme.colorScheme.onSurfaceVariant,
                    fontWeight = if (chip.selected) FontWeight.Medium else FontWeight.Normal,
                    modifier = Modifier.clip(RoundedCornerShape(16.dp))
                        .background(if (chip.selected) tint.copy(alpha = 0.12f) else Color.Transparent)
                        .clickable(interactionSource = remember { MutableInteractionSource() },
                            indication = null) { onClick(chip.key) }
                        .padding(horizontal = 12.dp, vertical = 6.dp))
            }
            if (labelAtEnd) {
                Spacer(Modifier.width(8.dp))
                LabelPill(label)
            }
        }
    }
}

/** 浅色 label pill（不可点）：类型/地区/年份起头、媒体库排尾部共用。 */
@Composable
private fun LabelPill(label: String) {
    Text(label, fontSize = 12.sp, color = MaterialTheme.colorScheme.primary, fontWeight = FontWeight.Medium,
        modifier = Modifier.clip(RoundedCornerShape(16.dp))
            .background(MaterialTheme.colorScheme.primary.copy(alpha = 0.12f))
            .padding(horizontal = 12.dp, vertical = 6.dp))
}