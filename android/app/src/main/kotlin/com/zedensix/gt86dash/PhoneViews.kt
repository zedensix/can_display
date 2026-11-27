package com.zedensix.gt86dash

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import android.os.SystemClock
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import kotlin.math.abs

// The dash drawn on a View, again every frame; a swipe across it turns
// the views, right to left the next one, as the Pico's encoder does.
class DashView(context: Context, private val scene: String?) : View(context) {
    val dash = Dash(context)
    private val area = Rect()
    private val started = SystemClock.uptimeMillis()

    private val swipes = GestureDetector(
        context,
        object : GestureDetector.SimpleOnGestureListener() {
            override fun onDown(e: MotionEvent) = true

            override fun onFling(e1: MotionEvent?, e2: MotionEvent, vx: Float, vy: Float): Boolean {
                if (abs(vx) < abs(vy) || abs(vx) < dp(context, SWIPE_DP_PER_S)) return false
                if (vx < 0) dash.nextView() else dash.previousView()
                return true
            }
        },
    )

    @SuppressLint("ClickableViewAccessibility") // a swipe, not a click
    override fun onTouchEvent(e: MotionEvent) = swipes.onTouchEvent(e) || super.onTouchEvent(e)

    override fun onDraw(canvas: Canvas) {
        val now = SystemClock.uptimeMillis() - started
        area.set(0, 0, width, height)
        if (scene != null) dash.draw(canvas, area, FakeSource.scene(scene), 0) else dash.frame(canvas, area, now)
        postInvalidateOnAnimation()
    }

    private companion object {
        const val SWIPE_DP_PER_S = 300
    }
}

// Pixels for v dp on c's screen.
fun dp(c: Context, v: Int): Int = (v * c.resources.displayMetrics.density).toInt()
