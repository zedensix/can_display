package com.zedensix.gt86dash

import android.graphics.Canvas
import kotlin.math.min

// The pages, laid out in the design's W x H pixels (an 800x480 screen
// less Android Auto's rail and a margin): every gauge (street), the track
// layout, one gauge alone, the oil too hot; their portrait layouts, the
// phone's, in PortraitPages.kt, on PORTRAIT_W x PORTRAIT_H.

const val W = 692f
const val H = 460f
const val PORTRAIT_W = 460f
const val PORTRAIT_H = 900f

// The main area, between the shift light and the strip.
private const val TOP = 40f
private const val BOTTOM = 412f

// A dial's text: its label, its value with the unit up beside it, a line
// under; sizes are the value's, the rest follows.
internal class DialText(val label: Float, val value: Float, val unit: Float, val extra: Float)

internal val BIG = DialText(20f, 112f, 30f, 18f)
internal val TRACK = DialText(17f, 76f, 22f, 15f)
internal val ALONE = DialText(22f, 132f, 34f, 0f)

// Digits and capitals stand about this much of their size above the
// baseline: what text is centered by.
internal const val CAP = 0.7f

class Pages(internal val d: Draw) {
    // The shift light's RPM, 0 for none (Settings).
    var shiftRpm = 0

    // The design's size this frame: W x H, or PORTRAIT_W x PORTRAIT_H.
    var w = W
    var h = H

    internal fun blinkOn(now: Long, period: Long) = (now / period) % 2 == 0L

    internal fun color(g: Gauge, r: Readings, v: Float?): Int =
        when {
            v == null -> Colors.DIM
            g === AFR && r.offThrottle -> Colors.LABEL
            else -> g.range.color(v)
        }

    internal fun inDanger(g: Gauge, r: Readings, v: Float?): Boolean =
        v != null && g.range.danger(v) && !(g === AFR && r.offThrottle)

    private fun dialText(c: Canvas, cx: Float, cy: Float, t: DialText, label: String, value: String, unit: String, color: Int, extra: String) {
        val g1 = 0.16f * t.value
        val g2 = 0.14f * t.value
        val height = CAP * t.label + g1 + CAP * t.value + if (t.extra > 0) g2 + CAP * t.extra else 0f
        var y = cy - height / 2 + CAP * t.label
        d.text(c, label, cx, y, t.label, Weight.SEMIBOLD, Colors.ACCENT, Align.CENTER, t.label / 5)
        y += g1 + CAP * t.value
        val wv = d.width(value, t.value, Weight.BOLD)
        val wu = if (unit.isEmpty()) 0f else d.width(unit, t.unit, Weight.SEMIBOLD) + 2
        val left = cx - (wv + wu) / 2
        d.text(c, value, left, y, t.value, Weight.BOLD, color)
        if (unit.isNotEmpty()) {
            d.text(c, unit, left + wv + 2, y - CAP * t.value + CAP * t.unit, t.unit, Weight.SEMIBOLD, Colors.LABEL)
        }
        if (t.extra > 0 && extra.isNotEmpty()) {
            d.text(c, extra, cx, y + g2 + CAP * t.extra, t.extra, Weight.MEDIUM, Colors.DIM, Align.CENTER, 2f)
        }
    }

    internal fun dial(c: Canvas, x: Float, y: Float, size: Float, width: Float, n: Int, t: DialText, g: Gauge, r: Readings, extra: String, now: Long) {
        val v = g.value(r)
        val blink = inDanger(g, r, v) && blinkOn(now, DANGER_BLINK_MS)
        d.dial(c, x, y, size, width, n, g.range, v, Extremes.max(g), if (t === ALONE) Extremes.min(g) else null)
        val label = if (t === ALONE) g.label else g.short
        dialText(c, x + size / 2, y + size / 2, t, label, g.text(v), g.unit, if (blink) Colors.LABEL else color(g, r, v), extra)
    }

    // A gauge as a row: its label, its value on the right, a bar under.
    internal fun row(c: Canvas, x: Float, y: Float, w: Float, g: Gauge, r: Readings, now: Long) {
        val v = g.value(r)
        if (inDanger(g, r, v) && blinkOn(now, DANGER_BLINK_MS)) d.rect(c, x - 8, y - 6, w + 16, 62f, Colors.ALARM, 6f)
        d.text(c, g.short, x, y + 30, 17f, Weight.SEMIBOLD, Colors.LABEL, spacing = 2.5f)
        d.text(c, g.unit, x + w - 22, y + 30, 18f, Weight.SEMIBOLD, Colors.DIM)
        d.text(c, g.text(v), x + w - 25, y + 30, 40f, Weight.BOLD, color(g, r, v), Align.RIGHT)
        d.bar(c, x, y + 40, w, 30, g.range, v)
    }

    // Along the bottom: the gear (unless the page shows it big), the
    // outside temperature, the odometer. No labels: the units say it.
    internal fun strip(c: Canvas, r: Readings, gear: Boolean) {
        d.line(c, 0f, h - 39.5f, w, h - 39.5f, Colors.FRAME, 1f)
        var x = 0f
        if (gear) {
            d.outline(c, 0f, h - 34, 38f, 34f, Colors.ACCENT, 2f, 8f)
            d.text(c, gearText(r.gear), 19f, h - 7, 28f, Weight.BOLD, Colors.ACCENT, Align.CENTER)
            x = 58f
        }
        val outside = r.outside?.let { "%.0f°C".format(it) } ?: "--°C"
        d.text(c, outside, x, h - 8, 24f, Weight.SEMIBOLD, Colors.LABEL)
        val km = when {
            r.km == null -> "-- km"
            r.trip -> "%.1f km".format(r.km)
            else -> "%,d km".format(r.km.toLong()).replace(',', ' ')
        }
        d.text(c, km, w, h - 8, 24f, Weight.SEMIBOLD, Colors.LABEL, Align.RIGHT)
    }

    internal fun gearText(gear: Int?): String = when (gear) {
        null -> "-"
        0 -> "N"
        else -> gear.toString()
    }

    // Clear of the car's buttons at the top right in landscape.
    internal fun shiftLight(c: Canvas, r: Readings, now: Long, width: Float = 620f) {
        if (shiftRpm > 0) d.shiftLight(c, 0f, 9f, width, r.rpm, shiftRpm, now)
    }

    internal fun scaleEnds(c: Canvas, x: Float, y: Float, size: Float, inset: Float, g: Gauge) {
        val baseline = y + size - inset
        d.text(c, g.text(g.range.min).substringBefore('.'), x + inset * 2.3f, baseline, 16f, Weight.MEDIUM, Colors.DIM)
        d.text(c, g.text(g.range.max).substringBefore('.'), x + size - inset * 2.1f, baseline, 16f, Weight.MEDIUM, Colors.DIM, Align.RIGHT)
    }

    fun street(c: Canvas, r: Readings, now: Long) {
        shiftLight(c, r, now)
        val max = Extremes.max(OIL)?.let { "MAX %.0f".format(it) } ?: ""
        dial(c, 0f, 56f, 340f, 20f, 40, BIG, OIL, r, max, now)
        scaleEnds(c, 0f, 56f, 340f, 26f, OIL)
        val rows = listOf(COOLANT, AFR, INTAKE, BATTERY, ETHANOL)
        val y0 = TOP + (BOTTOM - TOP - (rows.size * 54 + (rows.size - 1) * 12)) / 2
        rows.forEachIndexed { i, g -> row(c, 364f, y0 + i * 66, 328f, g, r, now) }
        strip(c, r, gear = true)
    }

    fun track(c: Canvas, r: Readings, now: Long) {
        shiftLight(c, r, now)
        val oilMax = Extremes.max(OIL)?.let { "MAX %.0f".format(it) } ?: ""
        val lean = Extremes.max(AFR)?.let { "LEAN %.1f".format(it) } ?: ""
        dial(c, 0f, 55f, 240f, 16f, 32, TRACK, OIL, r, oilMax, now)
        dial(c, W - 240, 55f, 240f, 16f, 32, TRACK, AFR, r, lean, now)
        // The mode's badge over the gear, big, between the dials.
        val cx = W / 2
        badge(c, cx, 87f, r.mode)
        d.text(c, gearText(r.gear), cx, 262f, 180f, Weight.BOLD, Colors.LABEL, Align.CENTER)
        row(c, 0f, 338f, 330f, COOLANT, r, now)
        row(c, W - 330, 338f, 330f, INTAKE, r, now)
        strip(c, r, gear = false)
    }

    // The drive mode's name, outlined in its color, centered on cx from y.
    internal fun badge(c: Canvas, cx: Float, y: Float, mode: DriveMode) {
        val color = when (mode) {
            DriveMode.RACE -> Colors.DANGER
            DriveMode.SPORT -> Colors.ACCENT
            else -> Colors.WARNING
        }
        val bw = d.width(mode.badge, 17f, Weight.BOLD, 1.5f) + 24
        d.outline(c, cx - bw / 2, y, bw, 30f, color, 2f, 14f)
        d.text(c, mode.badge, cx + 0.75f, y + 21, 17f, Weight.BOLD, color, Align.CENTER, 1.5f)
    }

    // One gauge alone: a big dial, the session's lowest and highest
    // beside it, then what its colors mean.
    fun alone(c: Canvas, g: Gauge, r: Readings, now: Long) {
        shiftLight(c, r, now)
        dial(c, 0f, TOP, 372f, 24f, 44, ALONE, g, r, "", now)
        scaleEnds(c, 0f, TOP, 372f, 30f, g)
        legend(c, g, 404f, TOP + (BOTTOM - TOP - 230) / 2, 120f)
    }

    // The session's lowest and highest side by side, column apart, then
    // what the gauge's colors mean, from x to the right edge.
    internal fun legend(c: Canvas, g: Gauge, x: Float, top: Float, column: Float) {
        val unit = if (g.unit == "°C") "°" else g.unit
        listOf("MIN" to Extremes.min(g), "MAX" to Extremes.max(g)).forEachIndexed { i, (name, v) ->
            val left = x + i * column
            d.text(c, name, left, top + 12, 16f, Weight.SEMIBOLD, Colors.DIM, spacing = 2.5f)
            val valueColor = if (v == null) Colors.DIM else if (i == 0) Colors.COLD else g.range.color(v)
            d.text(c, g.text(v) + unit, left, top + 62, 52f, Weight.BOLD, valueColor)
        }
        d.line(c, x, top + 90.5f, w, top + 90.5f, Colors.FRAME, 1f)
        val bands = g.range.bands()
        bands.forEachIndexed { i, (from, color) ->
            val baseline = top + 122 + i * 30
            val to = bands.getOrNull(i + 1)?.first
            val span = when {
                bands.size == 1 -> "all"
                i == 0 -> "< ${g.text(to)}"
                to == null -> "≥ ${g.text(from)}"
                else -> "${g.text(from)}–${g.text(to)}"
            }
            d.rect(c, x, baseline - 12, 28f, 10f, color, 2f)
            d.text(c, span, x + 40, baseline, 20f, Weight.SEMIBOLD, Colors.LABEL)
            d.text(c, meaning(color), x + 136, baseline, 18f, Weight.MEDIUM, Colors.DIM)
        }
    }

    private fun meaning(color: Int): String = when (color) {
        Colors.COLD -> "cold, don't rev it"
        Colors.GOOD -> "fine"
        Colors.WARNING -> "watch it"
        else -> "danger"
    }

    // The oil from OIL_CRITICAL: the whole screen, blinking (Dash fills
    // the background); in portrait, the last words on two lines.
    fun oilAlarm(c: Canvas, r: Readings, lit: Boolean) {
        val fg = if (lit) Colors.ALARM_TEXT else Colors.DANGER
        val cx = w / 2
        val y = (h - H) / 2
        d.text(c, "OIL TEMP", cx, y + 108, 52f, Weight.BOLD, fg, Align.CENTER, 8f)
        val value = OIL.text(r.oil)
        val wv = d.width(value, 240f, Weight.BOLD)
        val wu = d.width("°", 80f, Weight.BOLD)
        val left = cx - (wv + wu) / 2
        d.text(c, value, left, y + 312, 240f, Weight.BOLD, fg)
        d.text(c, "°", left + wv, y + 312 - CAP * 240 + CAP * 80, 80f, Weight.BOLD, fg)
        if (w > h) {
            d.text(c, "TOO HOT, LIFT OFF", cx, y + 388, 52f, Weight.BOLD, fg, Align.CENTER, 4f)
            return
        }
        d.text(c, "TOO HOT,", cx, y + 400, 52f, Weight.BOLD, fg, Align.CENTER, 4f)
        d.text(c, "LIFT OFF", cx, y + 466, 52f, Weight.BOLD, fg, Align.CENTER, 4f)
    }

    // While there's nothing to show: what the app waits for, and what to
    // do about it, centered in w x h (the phone shows it in portrait); text
    // too wide for it shrinks.
    fun status(c: Canvas, w: Float, h: Float, title: String, line: String, now: Long) {
        fun fit(s: String, size: Float, weight: Weight, spacing: Float) =
            min(size, size * (w - 40) / d.width(s, size, weight, spacing))
        d.spinner(c, w / 2, h / 2 - 80, 50f, now)
        d.text(c, title, w / 2, h / 2 + 60, fit(title, 44f, Weight.BOLD, 4f), Weight.BOLD, Colors.LABEL, Align.CENTER, 4f)
        d.text(c, line, w / 2, h / 2 + 105, fit(line, 24f, Weight.MEDIUM, 0f), Weight.MEDIUM, Colors.DIM, Align.CENTER)
    }

    // Near the limiter: SHIFT across the page (Dash floods it red), as
    // big as fits.
    fun limiter(c: Canvas) {
        val size = min(200f, 200f * (w - 20) / d.width("SHIFT", 200f, Weight.BOLD, 12f))
        d.text(c, "SHIFT", w / 2, h / 2 + CAP * size / 2, size, Weight.BOLD, Colors.ALARM_TEXT, Align.CENTER, size * 0.06f)
    }
}
