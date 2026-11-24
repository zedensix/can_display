package com.zedensix.gt86dash

import android.graphics.Canvas

// The pages in portrait, the phone held upright, on PORTRAIT_W x
// PORTRAIT_H: the landscape ones' pieces stacked instead of side by side,
// the shift light across the whole width, the strip along the bottom.

// Every gauge: the oil's dial across the top, the others as rows under it.
fun Pages.streetPortrait(c: Canvas, r: Readings, now: Long) {
    shiftLight(c, r, now, w)
    val max = Extremes.max(OIL)?.let { "MAX %.0f".format(it) } ?: ""
    dial(c, 20f, 44f, 420f, 22f, 44, BIG, OIL, r, max, now)
    scaleEnds(c, 20f, 44f, 420f, 30f, OIL)
    listOf(COOLANT, AFR, INTAKE, BATTERY, ETHANOL).forEachIndexed { i, g -> row(c, 0f, 492f + i * 66, w, g, r, now) }
    strip(c, r, gear = true)
}

// The track layout: the mode's badge, the oil's and the AFR's dials side
// by side, the gear big under them, then the coolant and the intake.
fun Pages.trackPortrait(c: Canvas, r: Readings, now: Long) {
    shiftLight(c, r, now, w)
    badge(c, w / 2, 40f, r.mode)
    val oilMax = Extremes.max(OIL)?.let { "MAX %.0f".format(it) } ?: ""
    val lean = Extremes.max(AFR)?.let { "LEAN %.1f".format(it) } ?: ""
    dial(c, 0f, 86f, 225f, 16f, 30, TRACK, OIL, r, oilMax, now)
    dial(c, w - 225, 86f, 225f, 16f, 30, TRACK, AFR, r, lean, now)
    d.text(c, gearText(r.gear), w / 2, 520f, 200f, Weight.BOLD, Colors.LABEL, Align.CENTER)
    row(c, 0f, 600f, w, COOLANT, r, now)
    row(c, 0f, 672f, w, INTAKE, r, now)
    strip(c, r, gear = false)
}

// One gauge alone: its dial across the top, the session's lowest and
// highest under it, then what its colors mean.
fun Pages.alonePortrait(c: Canvas, g: Gauge, r: Readings, now: Long) {
    shiftLight(c, r, now, w)
    dial(c, 10f, 44f, 440f, 24f, 44, ALONE, g, r, "", now)
    scaleEnds(c, 10f, 44f, 440f, 30f, g)
    legend(c, g, 24f, 510f, 210f)
}
