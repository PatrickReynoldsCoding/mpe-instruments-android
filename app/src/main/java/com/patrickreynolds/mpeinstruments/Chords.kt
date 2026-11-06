package com.patrickreynolds.mpeinstruments

/** Chords for the pad's chord modes: each type's notes as semitones from the root. */
object Chords {
    val TYPES: Map<String, List<Int>> = linkedMapOf(
        "maj" to listOf(0, 4, 7), "min" to listOf(0, 3, 7), "dim" to listOf(0, 3, 6), "aug" to listOf(0, 4, 8),
        "sus2" to listOf(0, 2, 7), "sus4" to listOf(0, 5, 7),
        "6" to listOf(0, 4, 7, 9), "m6" to listOf(0, 3, 7, 9), "7" to listOf(0, 4, 7, 10), "maj7" to listOf(0, 4, 7, 11),
        "m7" to listOf(0, 3, 7, 10), "mMaj7" to listOf(0, 3, 7, 11), "dim7" to listOf(0, 3, 6, 9), "m7♭5" to listOf(0, 3, 6, 10),
        "add9" to listOf(0, 4, 7, 14), "9" to listOf(0, 4, 7, 10, 14), "maj9" to listOf(0, 4, 7, 11, 14), "m9" to listOf(0, 3, 7, 10, 14),
        "11" to listOf(0, 4, 7, 10, 14, 17), "m11" to listOf(0, 3, 7, 10, 14, 17), "13" to listOf(0, 4, 7, 10, 14, 21),
        "maj13" to listOf(0, 4, 7, 11, 14, 21), "m13" to listOf(0, 3, 7, 10, 14, 21),
        "7♭9" to listOf(0, 4, 7, 10, 13), "7♯9" to listOf(0, 4, 7, 10, 15), "7♯11" to listOf(0, 4, 7, 10, 18),
        "7♭13" to listOf(0, 4, 7, 10, 20), "7sus4" to listOf(0, 5, 7, 10),
    )
    val GROUPS = listOf(
        "Triads" to listOf("maj", "min", "dim", "aug", "sus2", "sus4"),
        "6ths and 7ths" to listOf("6", "m6", "7", "maj7", "m7", "mMaj7", "dim7", "m7♭5"),
        "Extended" to listOf("add9", "9", "maj9", "m9", "11", "m11", "13", "maj13", "m13"),
        "Altered" to listOf("7♭9", "7♯9", "7♯11", "7♭13", "7sus4"),
    )
    /** Quick changes to the chord in hand, under the wheel. */
    val MODS = listOf("", "7", "9", "6", "sus4", "add9")
    /** How a wheel ring's roots line up with the outer ring's: same root, its relative minor, its 7th degree, its 4th, its 5th. */
    val ALIGN = listOf(0 to "Same root", 9 to "Relative minor (6th)", 11 to "7th degree", 5 to "4th", 7 to "5th", 2 to "2nd", 4 to "3rd")

    val PITCH_NAMES = listOf("C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B")
    /** The circle of fifths, from C clockwise. */
    val FIFTHS = listOf(0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5)
    /** The grid's roots, left to right: fifths order with C in the middle. */
    val GRID_ROOTS = listOf(1, 8, 3, 10, 5, 0, 7, 2, 9, 4, 11, 6)

    fun name(root: Int, type: String) = PITCH_NAMES[((root % 12) + 12) % 12] + suffix(type)
    fun suffix(type: String) = when (type) { "maj" -> ""; "min" -> "m"; "dim" -> "°"; else -> type }

    /** The chord in hand with a quick change applied: "7" on a minor chord is m7, on a major one 7, and so on. */
    fun modify(type: String, mod: String): String {
        if (mod.isEmpty()) return type
        val iv = TYPES[type] ?: return type
        val minor = 3 in iv && 4 !in iv && 6 !in iv
        val dim = 3 in iv && 6 in iv
        return when (mod) {
            "7" -> if (dim) "m7♭5" else if (minor) "m7" else if (type == "sus4") "7sus4" else "7"
            "9" -> if (dim) "m7♭5" else if (minor) "m9" else "9"
            "6" -> if (dim) "dim7" else if (minor) "m6" else "6"
            "sus4" -> if (10 in iv) "7sus4" else "sus4"
            "add9" -> if (minor) "m9" else "add9"
            else -> type
        }
    }

    /**
     * The pad's columns for a chord: its notes going up from the root nearest [center], [count] of
     * them, starting [shift] chord notes higher (or lower). Extensions above the octave are folded
     * down into it, so each octave holds the same notes.
     */
    fun columns(root: Int, type: String, center: Float, count: Int, shift: Int): IntArray {
        val pcs = (TYPES[type] ?: TYPES.getValue("maj")).map { ((it % 12) + 12) % 12 }.distinct().sorted()
        var r = root
        while (r + 12 <= center) r += 12
        while (r > center + 6) r -= 12
        // The chord's notes from a few octaves down, so a shift can go either way.
        val all = (-3..4).flatMap { o -> pcs.map { r + 12 * o + it } }.filter { it in 0..127 }
        val startAt = all.indexOfFirst { it >= r }.coerceAtLeast(0) - (count / 2 - pcs.size / 2).coerceAtLeast(0) + shift
        val s = startAt.coerceIn(0, (all.size - count).coerceAtLeast(0))
        return IntArray(count) { all.getOrElse(s + it) { 127 } }
    }
}
