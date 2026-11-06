package com.kivo.cinema.subtitle

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.view.View

/**
 * 字幕叠加 View（212）：全屏透明层，浮在视频 SurfaceView 之上。
 *
 * 合成走系统 SurfaceFlinger（两块 Surface 叠放）——**不碰视频主链路**，
 * P12 的零拷贝红线零触碰（设计稿 D2）。位图由 [setOverlayBitmap] 从外部给
 * （控制器渲染线程拷好），onDraw 只画。
 *
 * M1 契约：位图已经是"视频坐标系"的（控制器按视频宽高渲染 libass），
 * View 直接拉伸到自身尺寸——叠加层与视频同框（布局上 MATCH_PARENT）。
 */
class SubtitleOverlayView(context: Context) : View(context) {

    private var bitmap: Bitmap? = null

    /** 控制器在渲染线程之外（UI 线程）调用；Bitmap 已是拷贝。 */
    fun setOverlayBitmap(new: Bitmap?) {
        val old = bitmap
        bitmap = new
        if (old != new) {
            // 尺寸可能变化：Bitamp 复用交给 GC，M1 不做池化。
            invalidate()
        }
    }

    fun clear() {
        if (bitmap != null) {
            bitmap = null
            invalidate()
        }
    }

    override fun onDraw(canvas: Canvas) {
        val current = bitmap ?: return
        canvas.drawBitmap(current, null, android.graphics.RectF(0f, 0f, width.toFloat(), height.toFloat()), null)
    }
}
