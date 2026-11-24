package com.zedensix.gt86dash

import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import kotlin.math.min

// The dash on any canvas, the car's or the phone's: the page for the
// view and the readings, scaled from the design's pixels to the area it's
// given (on the car, what Android Auto's own buttons leave visible); an
// area taller than wide (the phone upright) gets the portrait pages.
class Dash(context: Context) {
    private val pages = Pages(Draw(context))
    private val settings = Settings(context)

    // 0 is the main view (every gauge, or the track layout past comfort),
    // then each gauge alone, as the encoder turns them on the Pico.
    var view = 0
        private set

    fun nextView() {
        view = (view + 1) % (1 + GAUGES.size)
    }

    fun previousView() {
        view = (view + GAUGES.size) % (1 + GAUGES.size)
    }

    fun home() {
        view = 0
    }

    // A frame from what's there: the made-up readings in demo, else the
    // dash's, else what the link waits for. Returns whether it drew the
    // gauges.
    fun frame(c: Canvas, area: Rect, now: Long): Boolean {
        val r = if (settings.demo) FakeSource.at(now) else LiveLink.readings()
        if (r != null) {
            draw(c, area, r, now)
            return true
        }
        LiveLink.retry()
        val (title, line) = when (LiveLink.status) {
            LiveLink.Status.NO_PERMISSION -> "ALLOW BLUETOOTH" to "on the phone, for GT86 Dash to find the dash"
            LiveLink.Status.BLUETOOTH_OFF -> "BLUETOOTH IS OFF" to "turn it on, on the phone"
            LiveLink.Status.SEARCHING -> "LOOKING FOR THE DASH" to "it's on with the contact"
            LiveLink.Status.CONNECTING, LiveLink.Status.LIVE -> "CONNECTING" to "GT86 DASH"
        }
        c.drawColor(Colors.BACKGROUND)
        size(area)
        place(c, area) { pages.status(c, pages.w, pages.h, title, line, now) }
        return false
    }

    private fun portrait(area: Rect) = area.height() > area.width()

    // The design's size for area: portrait or landscape.
    private fun size(area: Rect) {
        pages.w = if (portrait(area)) PORTRAIT_W else W
        pages.h = if (portrait(area)) PORTRAIT_H else H
    }

    // How far the design's pixels scale up to fit area, its margins included.
    fun scale(area: Rect): Float {
        val w = if (portrait(area)) PORTRAIT_W else W
        val h = if (portrait(area)) PORTRAIT_H else H
        return min(area.width() / (w + 32), area.height() / (h + 20))
    }

    // Draws in the design's pixels, scaled into area and centered. The
    // design's margins around it: 16 on the sides, 10 above and below.
    private fun place(c: Canvas, area: Rect, draw: () -> Unit) {
        val w = pages.w
        val h = pages.h
        val scale = scale(area)
        c.save()
        c.translate(area.left + (area.width() - w * scale) / 2, area.top + (area.height() - h * scale) / 2)
        c.scale(scale, scale)
        draw()
        c.restore()
    }

    fun draw(c: Canvas, area: Rect, r: Readings, now: Long) {
        Extremes.update(r)
        pages.shiftRpm = settings.shiftRpm
        size(area)
        val portrait = portrait(area)
        pages.lap = Recorder.lapText(System.currentTimeMillis(), short = portrait)
            ?: if (Recorder.recording) "● REC" to Colors.DANGER else null
        val oilCritical = (r.oil ?: 0f) >= OIL_CRITICAL
        val limiter = (r.rpm ?: 0f) >= LIMITER_FLASH_RPM && (now / LIMITER_FLASH_MS) % 2 == 0L
        val alarmLit = (now / 400) % 2 == 0L
        c.drawColor(
            when {
                oilCritical && alarmLit -> Colors.ALARM
                !oilCritical && limiter -> Colors.ALARM
                else -> Colors.BACKGROUND
            },
        )
        place(c, area) {
            when {
                oilCritical -> pages.oilAlarm(c, r, alarmLit)
                limiter -> pages.limiter(c)
                view > 0 && portrait -> pages.alonePortrait(c, GAUGES[view - 1], r, now)
                view > 0 -> pages.alone(c, GAUGES[view - 1], r, now)
                r.mode != DriveMode.COMFORT && portrait -> pages.trackPortrait(c, r, now)
                r.mode != DriveMode.COMFORT -> pages.track(c, r, now)
                portrait -> pages.streetPortrait(c, r, now)
                else -> pages.street(c, r, now)
            }
        }
    }
}
