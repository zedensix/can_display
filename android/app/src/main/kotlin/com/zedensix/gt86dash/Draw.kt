package com.zedensix.gt86dash

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Typeface
import kotlin.math.cos
import kotlin.math.floor
import kotlin.math.max
import kotlin.math.sin

// The pieces every page is drawn from: text, a round dial, a bar, the
// shift light. Sizes are the design's pixels (an 800x480 screen); Dash
// scales the canvas to the real one.

enum class Weight { MEDIUM, SEMIBOLD, BOLD }

enum class Align { LEFT, CENTER, RIGHT }

class Draw(context: Context) {
    private val faces = mapOf(
        Weight.MEDIUM to context.resources.getFont(R.font.barlow_condensed_medium),
        Weight.SEMIBOLD to context.resources.getFont(R.font.barlow_condensed_semibold),
        Weight.BOLD to context.resources.getFont(R.font.barlow_condensed_bold),
    )
    private val text = Paint(Paint.ANTI_ALIAS_FLAG).apply { fontFeatureSettings = "tnum" }
    private val fill = Paint(Paint.ANTI_ALIAS_FLAG)
    private val stroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeCap = Paint.Cap.BUTT
    }
    private val oval = RectF()

    private fun textPaint(size: Float, weight: Weight, color: Int, spacing: Float): Paint =
        text.apply {
            typeface = faces[weight] ?: Typeface.DEFAULT
            textSize = size
            this.color = color
            letterSpacing = spacing / size
        }

    // Text on its baseline y; spacing is the letter spacing in pixels.
    // Returns its width.
    fun text(
        c: Canvas,
        s: String,
        x: Float,
        y: Float,
        size: Float,
        weight: Weight,
        color: Int,
        align: Align = Align.LEFT,
        spacing: Float = 0f,
    ): Float {
        val p = textPaint(size, weight, color, spacing)
        val w = p.measureText(s)
        val left = when (align) {
            Align.LEFT -> x
            Align.CENTER -> x - w / 2
            Align.RIGHT -> x - w
        }
        c.drawText(s, left, y, p)
        return w
    }

    fun width(s: String, size: Float, weight: Weight, spacing: Float = 0f): Float =
        textPaint(size, weight, 0, spacing).measureText(s)

    fun rect(c: Canvas, x: Float, y: Float, w: Float, h: Float, color: Int, radius: Float = 0f, alpha: Float = 1f) {
        fill.color = color
        fill.alpha = (alpha * 255).toInt()
        c.drawRoundRect(x, y, x + w, y + h, radius, radius, fill)
    }

    fun outline(c: Canvas, x: Float, y: Float, w: Float, h: Float, color: Int, width: Float, radius: Float) {
        stroke.color = color
        stroke.strokeWidth = width
        c.drawRoundRect(x + width / 2, y + width / 2, x + w - width / 2, y + h - width / 2, radius, radius, stroke)
    }

    fun line(c: Canvas, x0: Float, y0: Float, x1: Float, y1: Float, color: Int, width: Float) {
        stroke.color = color
        stroke.strokeWidth = width
        c.drawLine(x0, y0, x1, y1, stroke)
    }

    // An arc of the circle centered on cx, cy, from angle a0 to a1 in
    // degrees clockwise from 12 o'clock.
    private fun arc(c: Canvas, cx: Float, cy: Float, r: Float, a0: Float, a1: Float, color: Int, width: Float, alpha: Float) {
        stroke.color = color
        stroke.alpha = (alpha * 255).toInt()
        stroke.strokeWidth = width
        oval.set(cx - r, cy - r, cx + r, cy + r)
        c.drawArc(oval, a0 - 90, a1 - a0, false, stroke)
    }

    private fun tick(c: Canvas, cx: Float, cy: Float, r: Float, width: Float, angle: Float, color: Int) {
        val t = Math.toRadians(angle.toDouble())
        val s = sin(t).toFloat()
        val k = -cos(t).toFloat()
        val r0 = r - width / 2 - 5
        val r1 = r + width / 2 + 5
        line(c, cx + r0 * s, cy + r0 * k, cx + r1 * s, cy + r1 * k, color, 3f)
    }

    // A 270 degree dial of n segments inside the size x size square at x,
    // y: each segment its band's color, the unlit ones faint so the red
    // zone shows at rest, a glow under the lit arc, a white tick at the
    // peak and a blue one at the lowest.
    fun dial(c: Canvas, x: Float, y: Float, size: Float, width: Float, n: Int, range: Range, v: Float?, peak: Float?, low: Float? = null) {
        val cx = x + size / 2
        val cy = y + size / 2
        val r = size / 2 - width / 2 - 10
        val step = 270f / n
        if (v != null) {
            arc(c, cx, cy, r, -135f, -135f + max(1f, 270 * range.fraction(v)), range.color(v), width + 16, 0.16f)
        }
        for (k in 0 until n) {
            val lo = range.min + (range.max - range.min) * k / n
            val mid = range.min + (range.max - range.min) * (k + 0.5f) / n
            val lit = v != null && v > lo
            arc(c, cx, cy, r, -135 + k * step + 0.9f, -135 + (k + 1) * step - 0.9f, range.color(mid), width, if (lit) 1f else 0.16f)
        }
        if (peak != null) tick(c, cx, cy, r, width, -135 + 270 * range.fraction(peak), Colors.LABEL)
        if (low != null) tick(c, cx, cy, r, width, -135 + 270 * range.fraction(low), Colors.COLD)
    }

    // A bar of n segments, w wide and 10 high, colored as a dial.
    fun bar(c: Canvas, x: Float, y: Float, w: Float, n: Int, range: Range, v: Float?) {
        val gap = 3f
        val sw = (w - (n - 1) * gap) / n
        for (k in 0 until n) {
            val lo = range.min + (range.max - range.min) * k / n
            val mid = range.min + (range.max - range.min) * (k + 0.5f) / n
            val lit = v != null && v > lo
            rect(c, x + k * (sw + gap), y, sw, 10f, range.color(mid), 1.5f, if (lit) 1f else 0.16f)
        }
    }

    // The race wheel's LED bar, w wide: pairs light from both ends over
    // the last SHIFT_SPAN_RPM, green, amber, red; all flash blue from the
    // shift RPM.
    fun shiftLight(c: Canvas, x: Float, y: Float, w: Float, rpm: Float?, shiftRpm: Int, now: Long) {
        val pairs = SHIFT_LEDS / 2
        val gap = 4f
        val lw = (w - (SHIFT_LEDS - 1) * gap) / SHIFT_LEDS
        val from = shiftRpm - SHIFT_SPAN_RPM
        val shift = rpm != null && rpm >= shiftRpm
        val lit = if (rpm == null || rpm < from) 0 else 1 + floor((rpm - from) / (SHIFT_SPAN_RPM / pairs)).toInt()
        for (k in 0 until pairs) {
            var color = if (k < 4) Colors.GOOD else if (k < 7) Colors.WARNING else Colors.DANGER
            var alpha = if (k < lit) 1f else 0.14f
            if (shift) {
                color = Colors.COLD
                alpha = if ((now / SHIFT_BLINK_MS) % 2 == 0L) 1f else 0.1f
            }
            rect(c, x + k * (lw + gap), y, lw, 14f, color, 2f, alpha)
            rect(c, x + w - lw - k * (lw + gap), y, lw, 14f, color, 2f, alpha)
        }
    }
}
