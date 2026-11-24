package com.zedensix.gt86dash

import android.content.Context
import android.graphics.Canvas
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RenderEffect
import android.graphics.RenderNode
import android.graphics.Shader
import android.view.View
import android.widget.FrameLayout

// A frame whose bar floats over the rest of it as frosted glass: what
// passes under the bar shows through it blurred, under a dark tint, a
// light edge and a sheen across its top, its corners round as a pill.
// The rest is recorded once and drawn twice, as itself and blurred
// inside the bar's shape (the GPU's blur, RenderEffect).
class GlassFrame(context: Context) : FrameLayout(context) {
    var bar: View? = null

    private val content = RenderNode("content")
    private val glass = RenderNode("glass").apply {
        val r = BLUR_DP * context.resources.displayMetrics.density
        setRenderEffect(RenderEffect.createBlurEffect(r, r, Shader.TileMode.CLAMP))
    }
    private val shape = Path()
    private val tint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = TINT }
    private val sheen = Paint(Paint.ANTI_ALIAS_FLAG)
    private val edge = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = context.resources.displayMetrics.density
        color = EDGE
    }

    override fun dispatchDraw(canvas: Canvas) {
        val b = bar
        if (b == null || !canvas.isHardwareAccelerated) return super.dispatchDraw(canvas)
        content.setPosition(0, 0, width, height)
        val recording = content.beginRecording()
        for (i in 0 until childCount) {
            val child = getChildAt(i)
            if (child !== b && child.visibility == VISIBLE) drawChild(recording, child, drawingTime)
        }
        content.endRecording()
        canvas.drawRenderNode(content)

        val left = b.left.toFloat()
        val top = b.top.toFloat()
        val radius = b.height / 2f
        shape.rewind()
        shape.addRoundRect(left, top, b.right.toFloat(), b.bottom.toFloat(), radius, radius, Path.Direction.CW)
        glass.setPosition(b.left, b.top, b.right, b.bottom)
        val under = glass.beginRecording()
        under.translate(-left, -top)
        under.drawRenderNode(content)
        glass.endRecording()
        canvas.save()
        canvas.clipPath(shape)
        canvas.drawRenderNode(glass)
        canvas.drawPath(shape, tint)
        sheen.shader = LinearGradient(0f, top, 0f, top + b.height / 2f, SHEEN, 0, Shader.TileMode.CLAMP)
        canvas.drawPath(shape, sheen)
        canvas.restore()
        canvas.drawPath(shape, edge)
        drawChild(canvas, b, drawingTime)
    }

    private companion object {
        const val BLUR_DP = 18f
        const val TINT = 0x8C161616.toInt()
        const val SHEEN = 0x1FFFFFFF
        const val EDGE = 0x38FFFFFF
    }
}
