package com.kivo.cinema.ui.detail

import android.graphics.Paint
import android.graphics.Typeface
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 255 #31 追剧日历月视图：周日起始；连续播日画棕色刷痕连播带，
 * 带下居中集数范围（1-4）；今日绿点；‹ 月标题 › 跨月切换。无播日格只显日期。
 * 全部 Canvas 精绘（带/数字/范围/点），保证多格连播带与范围标签精确居中。
 */
@Composable
internal fun DetailCalendarGrid(
    month: DetailCalendarData.Month,
    fg: Color,
    fgSub: Color,
    onPrev: () -> Unit,
    onNext: () -> Unit,
) {
    Column(Modifier.fillMaxWidth()) {
        Row(Modifier.fillMaxWidth().padding(vertical = 6.dp),
            verticalAlignment = Alignment.CenterVertically) {
            MonthArrow(fg, left = true, onClick = onPrev)
            Text(month.title, fontSize = 17.sp, fontWeight = FontWeight.SemiBold, color = fg,
                textAlign = TextAlign.Center, modifier = Modifier.weight(1f))
            MonthArrow(fg, left = false, onClick = onNext)
        }
        Row(Modifier.fillMaxWidth().padding(top = 4.dp, bottom = 6.dp)) {
            listOf("日", "一", "二", "三", "四", "五", "六").forEach {
                Text(it, fontSize = 12.sp, color = fgSub, textAlign = TextAlign.Center,
                    modifier = Modifier.weight(1f))
            }
        }
        month.weeks.forEach { week -> WeekRow(week, fg, fgSub) }
    }
}

@Composable
private fun MonthArrow(fg: Color, left: Boolean, onClick: () -> Unit) {
    Box(Modifier.size(40.dp).clip(CircleShape).clickable(onClick = onClick),
        contentAlignment = Alignment.Center) {
        Canvas(Modifier.size(11.dp, 18.dp)) {
            val w = 2.2.dp.toPx()
            if (left) {
                drawLine(fg, Offset(size.width, 0f), Offset(0f, size.height / 2), w, StrokeCap.Round)
                drawLine(fg, Offset(0f, size.height / 2), Offset(size.width, size.height), w, StrokeCap.Round)
            } else {
                drawLine(fg, Offset(0f, 0f), Offset(size.width, size.height / 2), w, StrokeCap.Round)
                drawLine(fg, Offset(size.width, size.height / 2), Offset(0f, size.height), w, StrokeCap.Round)
            }
        }
    }
}

@Composable
private fun WeekRow(week: List<DetailCalendarData.Cell>, fg: Color, fgSub: Color) {
    val paint = remember { Paint(Paint.ANTI_ALIAS_FLAG) }
    val band = remember { Color(0xFF6B5147) }
    val today = remember { Color(0xFF34C759) }
    Canvas(Modifier.fillMaxWidth().height(56.dp)) {
        val cellW = size.width / 7f
        val runs = DetailCalendarData.runs(week)
        val pad = 3.dp.toPx()
        val bandTop = 6.dp.toPx()
        val bandH = 26.dp.toPx()
        val corner = CornerRadius(12.dp.toPx())
        runs.forEach { run ->
            val x0 = run.first * cellW + pad
            val x1 = (run.last + 1) * cellW - pad
            drawRoundRect(band, Offset(x0, bandTop), Size(x1 - x0, bandH), corner)
        }
        val numY = 24.dp.toPx()
        week.forEachIndexed { i, cell ->
            val cx = i * cellW + cellW / 2f
            if (cell.day != null) {
                val onBand = cell.eps.isNotEmpty()
                paint.color = if (onBand) android.graphics.Color.WHITE else fg.toArgb()
                paint.textSize = 15.sp.toPx()
                paint.textAlign = Paint.Align.CENTER
                paint.typeface = if (cell.isToday) Typeface.DEFAULT_BOLD else Typeface.DEFAULT
                drawIntoCanvas { it.nativeCanvas.drawText(cell.day.toString(), cx, numY, paint) }
                if (cell.isToday) drawCircle(today, 3.dp.toPx(), Offset(cx, 38.dp.toPx()))
            }
        }
        paint.textSize = 11.sp.toPx()
        paint.textAlign = Paint.Align.CENTER
        paint.typeface = Typeface.DEFAULT
        paint.color = fgSub.toArgb()
        runs.forEach { run ->
            val label = DetailCalendarData.rangeLabel(week, run)
            if (label.isNotEmpty()) {
                val cx = (run.first + run.last + 1) / 2f * cellW
                drawIntoCanvas { it.nativeCanvas.drawText(label, cx, 50.dp.toPx(), paint) }
            }
        }
    }
}