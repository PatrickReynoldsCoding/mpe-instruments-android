package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.FilterChipDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

// A dark stage: the touch view and the two part colours are what stand out.
val Ink = Color(0xFF0E1014)
val Panel = Color(0xFF181B22)
val PanelHigh = Color(0xFF222632)
val Line = Color(0xFF2E3341)
val TextDim = Color(0xFF9AA2B4)
/** Part 0 (main, left of a split) and part 1 (layer, right of a split). */
val PartColors = listOf(Color(0xFFFF8A65), Color(0xFF4DD0E1))
val Warn = Color(0xFFFFC857)

private val scheme = darkColorScheme(
    primary = PartColors[0], onPrimary = Ink,
    secondary = PartColors[1], onSecondary = Ink,
    background = Ink, onBackground = Color(0xFFECEFF5),
    surface = Ink, onSurface = Color(0xFFECEFF5),
    surfaceVariant = Panel, onSurfaceVariant = TextDim,
    surfaceContainer = Panel, surfaceContainerHigh = PanelHigh, surfaceContainerHighest = PanelHigh,
    surfaceContainerLow = Panel, surfaceContainerLowest = Ink,
    outline = Line, outlineVariant = Line,
)

@Composable
fun MpeTheme(content: @Composable () -> Unit) = MaterialTheme(colorScheme = scheme, content = content)

@Composable
fun SectionCard(title: String, accent: Color? = null, trailing: (@Composable () -> Unit)? = null, content: @Composable () -> Unit) {
    Card(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 6.dp),
        shape = RoundedCornerShape(18.dp),
        colors = CardDefaults.cardColors(containerColor = Panel),
    ) {
        Column(Modifier.padding(16.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(title.uppercase(), color = accent ?: TextDim, fontSize = 12.sp, fontWeight = FontWeight.SemiBold,
                     letterSpacing = 1.2.sp, modifier = Modifier.weight(1f))
                trailing?.invoke()
            }
            Spacer(Modifier.padding(top = 8.dp))
            content()
        }
    }
}

@Composable
fun LabeledSlider(label: String, value: Float, range: ClosedFloatingPointRange<Float>, valueText: String,
                  accent: Color = PartColors[0], steps: Int = 0, onChange: (Float) -> Unit) {
    Column(Modifier.fillMaxWidth().padding(vertical = 2.dp)) {
        Row {
            Text(label, modifier = Modifier.weight(1f), fontSize = 14.sp)
            Text(valueText, color = TextDim, fontSize = 14.sp)
        }
        Slider(value = value, onValueChange = onChange, valueRange = range, steps = steps,
               colors = SliderDefaults.colors(thumbColor = accent, activeTrackColor = accent, inactiveTrackColor = Line))
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun Segmented(options: List<String>, selected: Int, accent: Color = PartColors[0], onSelect: (Int) -> Unit) {
    SingleChoiceSegmentedButtonRow(Modifier.fillMaxWidth()) {
        options.forEachIndexed { i, label ->
            SegmentedButton(
                selected = i == selected, onClick = { onSelect(i) },
                shape = SegmentedButtonDefaults.itemShape(i, options.size),
                colors = SegmentedButtonDefaults.colors(activeContainerColor = accent.copy(alpha = 0.22f),
                                                        activeContentColor = accent, activeBorderColor = accent,
                                                        inactiveBorderColor = Line),
                icon = {},
            ) { Text(label, fontSize = 13.sp, maxLines = 1) }
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun Chips(options: List<String>, selected: Int, accent: Color = PartColors[0], onSelect: (Int) -> Unit) {
    FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        options.forEachIndexed { i, label ->
            FilterChip(
                selected = i == selected, onClick = { onSelect(i) }, label = { Text(label) },
                colors = FilterChipDefaults.filterChipColors(selectedContainerColor = accent.copy(alpha = 0.22f),
                                                             selectedLabelColor = accent),
            )
        }
    }
}

@Composable
fun SwitchRow(label: String, checked: Boolean, detail: String? = null, accent: Color = PartColors[0], onChange: (Boolean) -> Unit) {
    Row(Modifier.fillMaxWidth().padding(vertical = 4.dp), verticalAlignment = Alignment.CenterVertically) {
        Column(Modifier.weight(1f)) {
            Text(label, fontSize = 14.sp)
            if (detail != null) Text(detail, color = TextDim, fontSize = 12.sp)
        }
        Spacer(Modifier.width(12.dp))
        Switch(checked = checked, onCheckedChange = onChange,
               colors = androidx.compose.material3.SwitchDefaults.colors(checkedTrackColor = accent, checkedThumbColor = Ink))
    }
}

fun pct(v: Float) = "${(v * 100).toInt()}%"
