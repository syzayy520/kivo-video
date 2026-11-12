package com.kivo.cinema.ui.compose.player

import androidx.compose.runtime.Composable
import androidx.compose.runtime.State
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.ClipOp
import androidx.compose.ui.layout.boundsInRoot
import androidx.compose.ui.layout.onGloballyPositioned

/**
 * 面板遮挡 ⇒ 底栏文字只画面板外的部分（v3 §1.4）。
 *
 * 【为什么需要它】玻璃壳是黑 ≈75% 且**没有背景模糊**（Android 的 RenderEffect
 * 只能模糊自己这一层，见 [playerGlassSurface]）。目标效果是把盖住的字遮死、
 * 露在面板外的保持清晰；我们的半透玻璃做不到 —— 被盖的白字以 25% 透出来，
 * 看着就是"右下角文案显示不全"。
 *
 * 【为什么是裁剪不是整块 alpha】实测（K40 s10）倍速左缘紧贴面板右缘、时长横跨
 * 面板缘。整块 alpha 会把"只被盖住一两个像素"的倍速也整块隐掉，与目标行为相反。
 * 真实行为是**只遮矩形内的那部分**，所以这里用 clipRect(Difference)
 * 把面板矩形从绘制区里挖掉：完全在面板内 ⇒ 不画；跨边 ⇒ 只画外面那截；
 * 完全在外 ⇒ 原样清晰。不靠加黑（那会把玻璃变成实心贴纸，v2 已推翻）。
 *
 * 单职责：本文件只回答"这个元素哪些部分该画"，不碰面板/底栏布局。
 * 面板矩形由 [PlayerOverlay] 的 FloatingPanel 在壳上实测发布，关闭时置 null。
 */
@Composable
internal fun Modifier.hideWhenCoveredByPanel(panelRect: State<Rect?>): Modifier {
    val panel = panelRect.value
    var selfBounds by remember { mutableStateOf<Rect?>(null) }
    return this
        // 放在 padding 之内 ⇒ 量到的是文字 ink 盒，不是命中盒。
        .onGloballyPositioned { selfBounds = it.boundsInRoot() }
        .drawWithContent {
            val self = selfBounds
            if (panel == null || self == null) {
                drawContent()
                return@drawWithContent
            }
            // 面板矩形换算到本元素局部坐标，再挖掉（Canvas 级裁剪，save/restore 包裹）。
            val hole = Rect(
                left = panel.left - self.left,
                top = panel.top - self.top,
                right = panel.right - self.left,
                bottom = panel.bottom - self.top,
            )
            val canvas = drawContext.canvas
            canvas.save()
            canvas.clipRect(hole, ClipOp.Difference)
            drawContent()
            canvas.restore()
        }
}