package com.zedensix.gt86dash

import android.widget.Toast
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

// Lap timing, a page in place of the dash: the lap running, big, in the
// gauges' Barlow Condensed, the best and the last under it, what it waits
// for when it can't time (lap timing off, no GPS fix, no start line near,
// no crossing yet); the switch, the start line set where the phone is, the
// laps so far, the best in green. Two columns in landscape.
@Composable
fun LapsPage(wantGps: () -> Unit) {
    val context = LocalContext.current
    val settings = remember { Settings(context) }
    var lapTiming by remember { mutableStateOf(settings.lapTiming) }
    ticking(TICK_MS)
    val t = Recorder.timer
    val start = t?.lapStart
    val now = System.currentTimeMillis()
    val last = t?.laps?.lastOrNull()
    val (time, color, line) = when {
        !lapTiming -> Triple("--:--.-", Colors.DIM, "Lap timing is off")
        !Recorder.locationGranted(context) -> Triple("--:--.-", Colors.DIM, "The GPS isn't allowed")
        !Recorder.hasFix -> Triple("--:--.-", Colors.DIM, "Waiting for the GPS")
        t == null -> Triple("--:--.-", Colors.DIM, "No start line near: set one crossing it")
        start == null -> Triple("0:00.0", Colors.DIM, "Waiting for the first crossing")
        last != null && now - start < Recorder.SHOW_LAP_MS ->
            Triple(LapTimer.text(last), if (last == t.best) Colors.GOOD else Colors.WARNING, "Lap ${t.laps.size}${if (last == t.best) "  ·  best" else ""}")
        else -> Triple(
            LapTimer.text(now - start),
            Colors.LABEL,
            listOfNotNull("Lap ${t.laps.size + 1}", t.best?.let { "best ${LapTimer.text(it)}" }, last?.let { "last ${LapTimer.text(it)}" }).joinToString("  ·  "),
        )
    }

    val live = @Composable {
        Column(Modifier.fillMaxWidth().padding(vertical = 24.dp), horizontalAlignment = Alignment.CenterHorizontally) {
            Text(time, color = Color(color), fontSize = 88.sp, fontFamily = TIMER, textAlign = TextAlign.Center)
            Text(line, style = MaterialTheme.typography.titleMedium, color = MaterialTheme.colorScheme.onSurfaceVariant, textAlign = TextAlign.Center)
        }
    }
    val controls = @Composable {
        Column {
            SectionCard {
                Item(
                    "Lap timing",
                    "Lap times from the phone's GPS",
                    trailing = {
                        Switch(checked = lapTiming, onCheckedChange = {
                            lapTiming = it
                            settings.lapTiming = it
                            wantGps()
                        })
                    },
                )
                Divider()
                Item(
                    "Set the start line here",
                    "Tap it crossing the line: it lies across the way you drive, one per track, found again when you're back",
                    onClick = { Toast.makeText(context, Recorder.setStartLine(context), Toast.LENGTH_SHORT).show() },
                )
                Divider()
                Item("Forget the start lines", onClick = {
                    Recorder.forgetStartLines(context)
                    Toast.makeText(context, "Start lines forgotten", Toast.LENGTH_SHORT).show()
                })
            }
            SectionTitle("Laps")
            SectionCard {
                val laps = t?.laps.orEmpty()
                if (laps.isEmpty()) Item("None yet")
                val best = laps.minOrNull()
                laps.forEachIndexed { i, ms ->
                    if (i > 0) Divider()
                    Item("Lap ${i + 1}", trailing = {
                        Text(LapTimer.text(ms), style = MaterialTheme.typography.titleMedium, color = if (ms == best) Color(Colors.GOOD) else Color.Unspecified)
                    })
                }
            }
        }
    }

    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = 16.dp)) {
        PageTitle("Lap timing")
        if (landscape()) {
            Row(horizontalArrangement = Arrangement.spacedBy(24.dp)) {
                Column(Modifier.weight(1f)) { live() }
                Column(Modifier.weight(1f)) { controls() }
            }
        } else {
            live()
            controls()
        }
        Spacer(Modifier.height(24.dp))
    }
}

private val TIMER = FontFamily(Font(R.font.barlow_condensed_bold))
private const val TICK_MS = 100L
