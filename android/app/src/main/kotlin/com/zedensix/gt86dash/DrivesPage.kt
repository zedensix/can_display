package com.zedensix.gt86dash

import android.app.Activity
import android.app.AlertDialog
import android.content.res.Configuration
import android.view.Gravity
import android.view.View
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import org.maplibre.android.MapLibre
import org.maplibre.android.camera.CameraUpdateFactory
import org.maplibre.android.geometry.LatLng
import org.maplibre.android.geometry.LatLngBounds
import org.maplibre.android.maps.MapView
import org.maplibre.android.maps.Style
import org.maplibre.android.style.expressions.Expression
import org.maplibre.android.style.layers.LineLayer
import org.maplibre.android.style.layers.Property
import org.maplibre.android.style.layers.PropertyFactory
import org.maplibre.android.style.sources.GeoJsonSource
import org.maplibre.geojson.Feature
import org.maplibre.geojson.FeatureCollection
import org.maplibre.geojson.LineString
import org.maplibre.geojson.Point

// The map: a spirited drive started or stopped, then the drives the
// phone recorded (Drives), a page in place of the dash, the newest first;
// a tap opens one: its way on a map (MapLibre,
// OpenFreeMap's dark tiles), colored by speed as the gauges' bands are,
// its start line, its numbers and its laps, the best in green. Its owner
// says when it shows (resume, pause).
class DrivesPage(private val activity: Activity, private val onClose: () -> Unit) : Page {
    private val frame = FrameLayout(activity)
    private var bottom = 0
    private var open: Drive? = null
    private var map: MapView? = null

    override fun build(bottom: Int): View {
        this.bottom = bottom
        (frame.parent as? android.view.ViewGroup)?.removeView(frame)
        show(open)
        return frame
    }

    override fun resume() {
        map?.onResume()
    }

    override fun pause() {
        map?.onPause()
    }

    // The list (null), or a drive.
    private fun show(drive: Drive?) {
        closeMap()
        open = drive
        frame.removeAllViews()
        frame.addView(if (drive == null) list() else detail(drive), FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT))
    }

    private fun closeMap() {
        map?.apply {
            onPause()
            onStop()
            onDestroy()
        }
        map = null
    }

    private fun header(title: String, back: () -> Unit) = pageHeader(activity, title, back)

    // Start or stop, red while one runs.
    private fun spirited(): View {
        val settings = Settings(activity)
        return pagePill(activity, if (settings.spirited) "Stop the spirited drive" else "Start a spirited drive") {
            settings.spirited = !settings.spirited
            activity.wantGps()
            show(null)
        }.apply { setTextColor(if (settings.spirited) Colors.DANGER else Colors.LABEL) }
    }

    private fun list(): View {
        val a = activity
        val column = LinearLayout(a).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(a, 24), dp(a, 8), dp(a, 24), dp(a, 24) + bottom)
        }
        column.addView(header("MAP") { onClose() })
        column.addView(spirited(), LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(a, 12) })
        column.addView(text("Records the way you drive on a map, until you stop it, the phone locked or not", 15f, Colors.DIM).apply { setPadding(0, dp(a, 4), 0, 0) })
        column.addView(pageText(a, "DRIVES", 15f, R.font.barlow_condensed_semibold, Colors.DIM, 0.15f).apply { setPadding(0, dp(a, 24), 0, 0) })
        val lines = Settings(a).startLines
        val drives = Drives.all(a)
        if (drives.isEmpty()) {
            column.addView(text("No drive yet: start a spirited drive, or time laps", 18f, Colors.DIM).apply { setPadding(0, dp(a, 16), 0, 0) })
        }
        for (d in drives) {
            val s = DriveSummary.of(d, lines)
            val row = LinearLayout(a).apply {
                orientation = LinearLayout.VERTICAL
                setPadding(0, dp(a, 14), 0, dp(a, 14))
                setOnClickListener { show(d) }
                addView(text(d.title(), 24f, Colors.LABEL, R.font.barlow_condensed_bold))
                addView(text(facts(s), 17f, Colors.DIM))
            }
            column.addView(row)
        }
        return ScrollView(a).apply {
            clipToPadding = false
            addView(column)
        }
    }

    // Its kind, distance, time, top speed and laps, in a line.
    private fun facts(s: DriveSummary): String {
        val parts = mutableListOf(s.drive.kind.uppercase(), "%.1f km".format(s.metres / 1000), "${s.ms / 60_000} min", "top %.0f km/h".format(s.topSpeed * 3.6f))
        s.laps.minOrNull()?.let { parts += "${s.laps.size} laps, best ${LapTimer.text(it)}" }
        return parts.joinToString("  ·  ")
    }

    private fun detail(d: Drive): View {
        val a = activity
        val s = DriveSummary.of(d, Settings(a).startLines)
        val landscape = a.resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        val side = LinearLayout(a).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(a, 24), dp(a, 8), dp(a, 24), dp(a, 24) + bottom)
            addView(header(d.title().uppercase()) { show(null) })
            addView(text(facts(s), 18f, Colors.LABEL).apply { setPadding(0, dp(a, 8), 0, dp(a, 8)) })
            s.laps.forEachIndexed { i, lap ->
                addView(text("LAP ${i + 1}   ${LapTimer.text(lap)}", 22f, if (lap == s.laps.min()) Colors.GOOD else Colors.LABEL, R.font.barlow_condensed_semibold))
            }
            addView(
                pagePill(a, "Delete this drive") {
                    AlertDialog.Builder(a).setMessage("Delete this drive from the phone? A backup keeps its copy.")
                        .setPositiveButton("Delete") { _, _ ->
                            d.file.delete()
                            show(null)
                        }
                        .setNegativeButton("Keep", null)
                        .show()
                },
                LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(a, 16) },
            )
        }
        val page = LinearLayout(a).apply { orientation = if (landscape) LinearLayout.HORIZONTAL else LinearLayout.VERTICAL }
        val m = mapOf(s)
        if (landscape) {
            page.addView(m, LinearLayout.LayoutParams(0, MATCH_PARENT, 3f))
            page.addView(ScrollView(a).apply { addView(side) }, LinearLayout.LayoutParams(0, MATCH_PARENT, 2f))
        } else {
            page.addView(ScrollView(a).apply { addView(side) }, LinearLayout.LayoutParams(MATCH_PARENT, 0, 2f))
            page.addView(m, LinearLayout.LayoutParams(MATCH_PARENT, 0, 3f).apply { bottomMargin = bottom })
        }
        return page
    }

    private fun mapOf(s: DriveSummary): View {
        MapLibre.getInstance(activity)
        val view = MapView(activity)
        view.onCreate(null)
        view.onStart()
        view.onResume()
        map = view
        view.getMapAsync { m ->
            m.setStyle(Style.Builder().fromUri(STYLE)) { style ->
                style.addSource(GeoJsonSource("drive", FeatureCollection.fromFeatures(features(s))))
                style.addLayer(
                    LineLayer("drive", "drive").withProperties(
                        PropertyFactory.lineColor(Expression.get("color")),
                        PropertyFactory.lineWidth(4f),
                        PropertyFactory.lineCap(Property.LINE_CAP_ROUND),
                        PropertyFactory.lineJoin(Property.LINE_JOIN_ROUND),
                    ),
                )
                val points = s.fixes.map { LatLng(it.lat, it.lon) }
                if (points.size >= 2) {
                    m.moveCamera(CameraUpdateFactory.newLatLngBounds(LatLngBounds.Builder().includes(points).build(), dp(activity, 32)))
                }
            }
        }
        return view
    }

    // The way in runs of one color, a run per band of speed, and the
    // start line across, white.
    private fun features(s: DriveSummary): List<Feature> {
        val out = mutableListOf<Feature>()
        var run = mutableListOf<Point>()
        var color: String? = null
        for (f in s.fixes) {
            val c = speedColor(f.speed * 3.6f)
            val p = Point.fromLngLat(f.lon, f.lat)
            if (c != color && run.isNotEmpty()) {
                run += p
                out += line(run, color!!)
                run = mutableListOf()
            }
            run += p
            color = c
        }
        if (run.size >= 2) out += line(run, color!!)
        s.line?.let { l ->
            val h = Math.toRadians(l.heading.toDouble())
            val dLat = StartLine.HALF_WIDTH_M / StartLine.M_PER_DEGREE
            val dLon = dLat / Math.cos(Math.toRadians(l.lat))
            // Across the way it's crossed: east-west for a heading north.
            val ends = listOf(-1, 1).map { k -> Point.fromLngLat(l.lon + k * dLon * Math.cos(h), l.lat - k * dLat * Math.sin(h)) }
            out += line(ends, "#FFFFFF")
        }
        return out
    }

    private fun line(points: List<Point>, color: String) =
        Feature.fromGeometry(LineString.fromLngLats(points)).apply { addStringProperty("color", color) }

    private fun speedColor(kmh: Float) = "#%06X".format(
        0xFFFFFF and when {
            kmh < 50 -> Colors.COLD
            kmh < 90 -> Colors.GOOD
            kmh < 130 -> Colors.WARNING
            else -> Colors.DANGER
        },
    )

    fun destroy() = closeMap()

    private fun text(s: String, size: Float, color: Int, font: Int = R.font.barlow_condensed_medium) = pageText(activity, s, size, font, color)

    private companion object {
        const val STYLE = "https://tiles.openfreemap.org/styles/dark"
    }
}

// A page shown in the dash's place (the settings, the drives): built for
// the orientation (again when the phone turns), bottom pixels of room
// under it for the floating bar; told when it shows and when it doesn't.
interface Page {
    fun build(bottom: Int): View

    fun resume()

    fun pause()
}
