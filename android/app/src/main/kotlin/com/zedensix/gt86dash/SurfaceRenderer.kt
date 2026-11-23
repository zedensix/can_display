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
    }

    override fun onDestroy(owner: LifecycleOwner) {
        handler.removeCallbacks(frame)
        surface = null
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

    private fun draw() {
        val s = surface ?: return
        if (!s.isValid) return
        val now = SystemClock.uptimeMillis() - started
        val canvas = s.lockHardwareCanvas()
        try {
            dash.draw(canvas, visible ?: size, FakeSource.at(now), now)
        } finally {
            s.unlockCanvasAndPost(canvas)
        }
    }

    private companion object {
        const val FRAME_MS = 33L
    }
}
