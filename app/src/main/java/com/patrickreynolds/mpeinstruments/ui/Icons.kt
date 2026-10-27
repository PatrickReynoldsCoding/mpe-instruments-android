package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.graphics.vector.path
import androidx.compose.ui.unit.dp

/** Tab icons drawn here, so the app needs only Material's core icon set. */
object MpeIcons {
    private fun icon(name: String, block: ImageVector.Builder.() -> Unit) =
        ImageVector.Builder(name, 24.dp, 24.dp, 24f, 24f).apply(block).build()

    /** Rising steps: an arpeggio. */
    val Arp = icon("Arp") {
        path(fill = SolidColor(Color.Black)) {
            for ((i, h) in listOf(5f, 9f, 13f, 17f).withIndex()) {
                val x = 3f + i * 5f
                moveTo(x, 21f); lineTo(x, 21f - h); lineTo(x + 3.2f, 21f - h); lineTo(x + 3.2f, 21f); close()
            }
        }
    }

    /** Three faders. */
    val Sliders = icon("Sliders") {
        path(fill = SolidColor(Color.Black)) {
            for ((i, knob) in listOf(8f, 15f, 6f).withIndex()) {
                val x = 5f + i * 7f
                moveTo(x - 0.9f, 3f); lineTo(x + 0.9f, 3f); lineTo(x + 0.9f, 21f); lineTo(x - 0.9f, 21f); close()
                moveTo(x - 3f, knob - 1.8f); lineTo(x + 3f, knob - 1.8f); lineTo(x + 3f, knob + 1.8f); lineTo(x - 3f, knob + 1.8f); close()
            }
        }
    }
}
