package com.zedensix.gt86dash

import android.content.res.Configuration
import android.os.SystemClock
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.ListItem
import androidx.compose.material3.ListItemDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.DeviceFontFamilyName
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.delay

// The phone's pages in Material 3 (Jetpack Compose): dark, the dash's
// orange their color, in the phone's own type as its Settings app sets it
// (Google Sans Text, headlines Google Sans, no letter spacing, on a Pixel;
// Roboto elsewhere).
// The gauges keep their own drawing (Draw, Barlow Condensed).

private val scheme = darkColorScheme(
    primary = Color(Colors.ACCENT),
    onPrimary = Color.Black,
    primaryContainer = Color(Colors.FRAME),
    onPrimaryContainer = Color(0xFFFFDBCF),
    secondary = Color(Colors.ACCENT),
    onSecondary = Color.Black,
    secondaryContainer = Color(Colors.FRAME),
    onSecondaryContainer = Color(0xFFFFDBCF),
    background = Color.Black,
    onBackground = Color(Colors.LABEL),
    surface = Color.Black,
    onSurface = Color(Colors.LABEL),
    surfaceVariant = Color(0xFF242424),
    onSurfaceVariant = Color(Colors.DIM),
    surfaceContainerLowest = Color.Black,
    surfaceContainerLow = Color(0xFF0E0E0E),
    surfaceContainer = Color(0xFF141414),
    surfaceContainerHigh = Color(0xFF1C1C1C),
    surfaceContainerHighest = Color(0xFF262626),
    outline = Color(0xFF5C5C5C),
    outlineVariant = Color(0xFF2A2A2A),
    error = Color(Colors.DANGER),
    onError = Color.Black,
)

private fun device(name: String) =
    FontFamily(listOf(FontWeight.Normal, FontWeight.Medium, FontWeight.SemiBold, FontWeight.Bold).map { Font(DeviceFontFamilyName(name), it) })

private val typography = Typography().let { t ->
    val heading = device("google-sans")
    val text = device("google-sans-text")
    // No letter spacing: M3's baseline tracking is Roboto's and stretches
    // Google Sans; Pixel's own apps set none.
    fun TextStyle.with(f: FontFamily) = copy(fontFamily = f, letterSpacing = 0.sp)
    t.copy(
        displayLarge = t.displayLarge.with(heading),
        displayMedium = t.displayMedium.with(heading),
        displaySmall = t.displaySmall.with(heading),
        headlineLarge = t.headlineLarge.with(heading),
        headlineMedium = t.headlineMedium.with(heading),
        headlineSmall = t.headlineSmall.with(heading),
        titleLarge = t.titleLarge.with(text),
        titleMedium = t.titleMedium.with(text),
        titleSmall = t.titleSmall.with(text),
        bodyLarge = t.bodyLarge.with(text),
        bodyMedium = t.bodyMedium.with(text),
        bodySmall = t.bodySmall.with(text),
        labelLarge = t.labelLarge.with(text),
        labelMedium = t.labelMedium.with(text),
        labelSmall = t.labelSmall.with(text),
    )
}

@Composable
fun DashTheme(content: @Composable () -> Unit) = MaterialTheme(colorScheme = scheme, typography = typography, content = content)

// The uptime, again every ms: a page reading the link, the recorder or the
// update (plain objects, not Compose state) redraws as they change.
@Composable
fun ticking(ms: Long): Long {
    val now by produceState(SystemClock.uptimeMillis()) {
        while (true) {
            delay(ms)
            value = SystemClock.uptimeMillis()
        }
    }
    return now
}

@Composable
fun landscape() = LocalConfiguration.current.orientation == Configuration.ORIENTATION_LANDSCAPE

// A page's title, a back arrow before it when it has one.
@Composable
fun PageTitle(title: String, onBack: (() -> Unit)? = null) {
    Row(Modifier.padding(top = 16.dp, bottom = 8.dp), verticalAlignment = Alignment.CenterVertically) {
        if (onBack != null) {
            IconButton(onClick = onBack) { Icon(painterResource(R.drawable.ic_back), "Back") }
        }
        Text(title, style = MaterialTheme.typography.headlineMedium, modifier = Modifier.padding(start = if (onBack == null) 16.dp else 4.dp))
    }
}

// A section: its title in the color, its rows in a card, a line between
// them (Divider), a footnote under it.
@Composable
fun SectionTitle(title: String) =
    Text(title, color = MaterialTheme.colorScheme.primary, style = MaterialTheme.typography.titleSmall, modifier = Modifier.padding(start = 16.dp, top = 24.dp, bottom = 8.dp))

@Composable
fun SectionCard(content: @Composable ColumnScope.() -> Unit) =
    Card(colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceContainer), shape = MaterialTheme.shapes.extraLarge) {
        Column(content = content)
    }

@Composable
fun Divider() = HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)

@Composable
fun Footnote(text: String) =
    Text(text, style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.onSurfaceVariant, modifier = Modifier.padding(start = 16.dp, end = 16.dp, top = 8.dp))

// A card's row: a list item, the whole row the tap when it does something.
@Composable
fun Item(
    headline: String,
    supporting: String? = null,
    supportingColor: Color = Color.Unspecified,
    leading: (@Composable () -> Unit)? = null,
    trailing: (@Composable () -> Unit)? = null,
    onClick: (() -> Unit)? = null,
) = ListItem(
    headlineContent = { Text(headline) },
    supportingContent = supporting?.takeIf { it.isNotEmpty() }?.let { { Text(it, color = supportingColor) } },
    leadingContent = leading,
    trailingContent = trailing,
    colors = ListItemDefaults.colors(containerColor = Color.Transparent),
    modifier = if (onClick != null) Modifier.clickable(onClick = onClick) else Modifier,
)

// A state's color, as a dot.
@Composable
fun Dot(color: Int) = Box(Modifier.size(12.dp).clip(CircleShape).background(Color(color)))

@Composable
fun Chevron() = Icon(painterResource(R.drawable.ic_next), null, tint = MaterialTheme.colorScheme.onSurfaceVariant)
