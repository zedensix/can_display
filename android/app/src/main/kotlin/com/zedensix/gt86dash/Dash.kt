package com.zedensix.gt86dash

import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import kotlin.math.min

// The dash on any canvas, the car's or the phone's: the page for the
// view and the readings, scaled from the design's pixels to the area it's
// given (on the car, what Android Auto's own buttons leave visible).
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

    fun draw(c: Canvas, area: Rect, r: Readings, now: Long) {
        Extremes.update(r)
        pages.shiftRpm = settings.shiftRpm
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
        // The design's margins around W x H: 16 on the sides, 10 above
        // and below.
        val scale = min(area.width() / (W + 32), area.height() / (H + 20))
        c.save()
        c.translate(area.left + (area.width() - W * scale) / 2, area.top + (area.height() - H * scale) / 2)
        c.scale(scale, scale)
        when {
            oilCritical -> pages.oilAlarm(c, r, alarmLit)
            limiter -> pages.limiter(c)
            view > 0 -> pages.alone(c, GAUGES[view - 1], r, now)
            r.mode != DriveMode.COMFORT -> pages.track(c, r, now)
            else -> pages.street(c, r, now)
        }
        c.restore()
    }
}
