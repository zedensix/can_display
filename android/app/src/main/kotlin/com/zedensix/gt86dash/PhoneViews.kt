package com.zedensix.gt86dash

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import android.os.SystemClock
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.RoundedCorner
import android.view.View
import android.view.WindowInsets
import android.widget.ImageButton
import android.widget.ImageView
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

// What the phone's screens share: everything inside the safe area, clear
// of the camera's cutout, the system bars and the screen's rounded
// corners, whichever side the phone is turned to; a round icon button.
fun safeArea(root: View) {
    root.setOnApplyWindowInsetsListener { v, insets ->
        val safe = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
        // A corner of radius r cuts into a square at it up to 0.3 r from
        // both edges: each side clear of its two corners' cut.
        fun cut(a: Int, b: Int) = (CORNER_CUT * maxOf(insets.getRoundedCorner(a)?.radius ?: 0, insets.getRoundedCorner(b)?.radius ?: 0)).toInt()
        v.setPadding(
            maxOf(safe.left, cut(RoundedCorner.POSITION_TOP_LEFT, RoundedCorner.POSITION_BOTTOM_LEFT)),
            maxOf(safe.top, cut(RoundedCorner.POSITION_TOP_LEFT, RoundedCorner.POSITION_TOP_RIGHT)),
            maxOf(safe.right, cut(RoundedCorner.POSITION_TOP_RIGHT, RoundedCorner.POSITION_BOTTOM_RIGHT)),
            maxOf(safe.bottom, cut(RoundedCorner.POSITION_BOTTOM_LEFT, RoundedCorner.POSITION_BOTTOM_RIGHT)),
        )
        WindowInsets.CONSUMED
    }
}

private const val CORNER_CUT = 0.3f

fun dp(c: Context, v: Int): Int = (v * c.resources.displayMetrics.density).toInt()

fun iconButton(c: Context, icon: Int, size: Int, onClick: () -> Unit) =
    ImageButton(c).apply {
        setImageResource(icon)
        setColorFilter(0xFFBDBDBD.toInt())
        setBackgroundResource(R.drawable.round_button)
        val pad = dp(c, size) / 5
        setPadding(pad, pad, pad, pad)
        scaleType = ImageView.ScaleType.FIT_CENTER
        setOnClickListener { onClick() }
    }
