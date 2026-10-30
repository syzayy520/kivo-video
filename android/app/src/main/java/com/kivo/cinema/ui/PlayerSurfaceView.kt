package com.kivo.cinema.ui

import android.content.Context
import android.util.AttributeSet
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * 只负责 Surface 生命周期回调的转发。
 *
 * 不持有播放状态，不碰解码/渲染 —— 见 08-p12-family-tree.md 文件职责表。
 */
class PlayerSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr), SurfaceHolder.Callback {

    interface Listener {
        fun onSurfaceAvailable(holder: SurfaceHolder)
        fun onSurfaceLost()
    }

    var listener: Listener? = null

    init {
        holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        listener?.onSurfaceAvailable(holder)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // 尺寸变化由 native 侧按需处理，此处不转发以避免重复初始化
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        listener?.onSurfaceLost()
    }
}
