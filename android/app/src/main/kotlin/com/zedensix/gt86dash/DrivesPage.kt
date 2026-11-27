package com.zedensix.gt86dash

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.viewinterop.AndroidView
import androidx.compose.ui.unit.dp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
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

// The map: a spirited drive started or stopped, then the drives the phone
// recorded (Drives), a page in place of the dash, the newest first; a tap
// opens one, Back closing it: its way on a map (MapLibre, OpenFreeMap's
// dark tiles), colored by speed as the gauges' bands are, its start line,
// its numbers and its laps, the best in green.
@Composable
fun DrivesPage(wantGps: () -> Unit) {
    val context = LocalContext.current
    val settings = remember { Settings(context) }
    var open by remember { mutableStateOf<Drive?>(null) }
    var version by remember { mutableIntStateOf(0) }
    open?.let { d ->
        BackHandler { open = null }
        DriveDetail(d) {
            open = null
            version++
        }
        return
    }
    var spirited by remember { mutableStateOf(settings.spirited) }
    val drives = remember(version) {
        val lines = settings.startLines
        Drives.all(context).map { it to DriveSummary.of(it, lines) }
    }
    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = 16.dp)) {
        PageTitle("Map")
        Button(
            onClick = {
                spirited = !spirited
                settings.spirited = spirited
                wantGps()
            },
            colors = if (spirited) ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error) else ButtonDefaults.buttonColors(),
            modifier = Modifier.fillMaxWidth().padding(top = 8.dp),
        ) { Text(if (spirited) "Stop the spirited drive" else "Start a spirited drive") }
        Footnote("Records the way you drive on a map, until you stop it, the phone locked or not")
        SectionTitle("Drives")
        if (drives.isEmpty()) {
            Footnote("No drive yet: start a spirited drive, or time laps")
        } else {
            SectionCard {
                drives.forEachIndexed { i, (d, s) ->
                    if (i > 0) Divider()
                    Item(d.title(), facts(s), trailing = { Chevron() }, onClick = { open = d })
                }
            }
        }
        Spacer(Modifier.height(24.dp))
    }
}

// Its kind, distance, time, top speed and laps, in a line.
private fun facts(s: DriveSummary): String {
    val parts = mutableListOf(s.drive.kind.replaceFirstChar { it.uppercase() }, "%.1f km".format(s.metres / 1000), "${s.ms / 60_000} min", "top %.0f km/h".format(s.topSpeed * 3.6f))
    s.laps.minOrNull()?.let { parts += "${s.laps.size} laps, best ${LapTimer.text(it)}" }
    return parts.joinToString("  ·  ")
}

@Composable
private fun DriveDetail(d: Drive, onClose: () -> Unit) {
    val context = LocalContext.current
    val s = remember(d) { DriveSummary.of(d, Settings(context).startLines) }
    var confirm by remember { mutableStateOf(false) }
    val info = @Composable { modifier: Modifier ->
        Column(modifier.verticalScroll(rememberScrollState()).padding(horizontal = 16.dp)) {
            PageTitle(d.title(), onBack = onClose)
            Text(facts(s), style = MaterialTheme.typography.bodyLarge, modifier = Modifier.padding(start = 16.dp, bottom = 8.dp))
            if (s.laps.isNotEmpty()) {
                SectionCard {
                    s.laps.forEachIndexed { i, lap ->
                        if (i > 0) Divider()
                        Item("Lap ${i + 1}", trailing = {
                            Text(LapTimer.text(lap), style = MaterialTheme.typography.titleMedium, color = if (lap == s.laps.min()) Color(Colors.GOOD) else Color.Unspecified)
                        })
                    }
                }
            }
            OutlinedButton(onClick = { confirm = true }, modifier = Modifier.padding(vertical = 16.dp)) { Text("Delete this drive") }
        }
    }
    if (landscape()) {
        Row(Modifier.fillMaxSize()) {
            DriveMap(s, Modifier.weight(3f).fillMaxHeight())
            info(Modifier.weight(2f).fillMaxHeight())
        }
    } else {
        Column(Modifier.fillMaxSize()) {
            info(Modifier.weight(2f).fillMaxWidth())
            Box(Modifier.weight(3f).fillMaxWidth()) { DriveMap(s, Modifier.fillMaxSize()) }
        }
    }
    if (confirm) {
        AlertDialog(
            onDismissRequest = { confirm = false },
            text = { Text("Delete this drive from the phone? A backup keeps its copy.") },
            confirmButton = {
                TextButton(onClick = {
                    d.file.delete()
                    confirm = false
                    onClose()
                }) { Text("Delete") }
            },
            dismissButton = { TextButton(onClick = { confirm = false }) { Text("Keep") } },
        )
    }
}

// MapLibre's MapView, its lifecycle the page's.
@Composable
private fun DriveMap(s: DriveSummary, modifier: Modifier) {
    val context = LocalContext.current
    val lifecycle = LocalLifecycleOwner.current.lifecycle
    val map = remember(s) {
        MapLibre.getInstance(context)
        MapView(context).apply {
            onCreate(null)
            getMapAsync { m ->
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
                        val padding = (32 * context.resources.displayMetrics.density).toInt()
                        m.moveCamera(CameraUpdateFactory.newLatLngBounds(LatLngBounds.Builder().includes(points).build(), padding))
                    }
                }
            }
        }
    }
    DisposableEffect(lifecycle, map) {
        val observer = LifecycleEventObserver { _, event ->
            when (event) {
                Lifecycle.Event.ON_START -> map.onStart()
                Lifecycle.Event.ON_RESUME -> map.onResume()
                Lifecycle.Event.ON_PAUSE -> map.onPause()
                Lifecycle.Event.ON_STOP -> map.onStop()
                else -> {}
            }
        }
        lifecycle.addObserver(observer)
        onDispose {
            lifecycle.removeObserver(observer)
            map.onPause()
            map.onStop()
            map.onDestroy()
        }
    }
    AndroidView({ map }, modifier)
}

// The way in runs of one color, a run per band of speed, and the start
// line across, white.
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

private const val STYLE = "https://tiles.openfreemap.org/styles/dark"
