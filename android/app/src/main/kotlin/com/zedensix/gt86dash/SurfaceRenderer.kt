package com.zedensix.gt86dash

import android.graphics.Rect
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.Surface
import androidx.car.app.AppManager
import androidx.car.app.CarContext
import androidx.car.app.SurfaceCallback
import androidx.car.app.SurfaceContainer
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner

// Draws the dash on the surface Android Auto hands a navigation app, at
// FRAME_MS, inside the area its own buttons leave visible.
class SurfaceRenderer(private val carContext: CarContext, lifecycle: Lifecycle) :
    SurfaceCallback, DefaultLifecycleObserver {
    val dash = Dash(carContext)
    private val handler = Handler(Looper.getMainLooper())
    private var surface: Surface? = null
    private val size = Rect()
    private var visible: Rect? = null
    private val started = SystemClock.uptimeMillis()

    // How many buttons the action strip shows, for area().
    var actions = 3

    private val frame = object : Runnable {
        override fun run() {
            draw()
            handler.postDelayed(this, FRAME_MS)
        }
    }

    init {
        lifecycle.addObserver(this)
    }

    override fun onCreate(owner: LifecycleOwner) {
        carContext.getCarService(AppManager::class.java).setSurfaceCallback(this)
        LiveLink.start(carContext)
    }

    override fun onDestroy(owner: LifecycleOwner) {
        handler.removeCallbacks(frame)
        surface = null
        LiveLink.stop()
    }

    override fun onSurfaceAvailable(container: SurfaceContainer) {
        surface = container.surface
        size.set(0, 0, container.width, container.height)
        handler.removeCallbacks(frame)
        handler.post(frame)
    }

    override fun onVisibleAreaChanged(area: Rect) {
        visible = Rect(area)
    }

    override fun onSurfaceDestroyed(container: SurfaceContainer) {
        handler.removeCallbacks(frame)
        surface = null
    }

    // Where to draw. Android Auto's visible area leaves out the whole band
    // under its action strip, though the strip only takes the right end of
    // it: the landscape pages, as tall as the area lets them, got 2/3 of
    // the screen's height on a Pioneer. They get the band back when that
    // makes them bigger, kept left of the strip: its buttons are about
    // 1.1 bands apart, half a band from the edge (read off that Pioneer).
    private fun area(): Rect {
        val v = visible ?: return size
        val band = v.top - size.top
        if (band <= 0) return v
        val strip = (band * (1.1f * actions + 0.5f)).toInt()
        val tall = Rect(v.left, size.top, v.right - strip, v.bottom)
        return if (dash.scale(tall) > dash.scale(v)) tall else v
    }

    private fun draw() {
        val s = surface ?: return
        if (!s.isValid) return
        val now = SystemClock.uptimeMillis() - started
        val canvas = s.lockHardwareCanvas()
        try {
            dash.frame(canvas, area(), now)
        } finally {
            s.unlockCanvasAndPost(canvas)
        }
    }

    private companion object {
        const val FRAME_MS = 33L
    }
}
