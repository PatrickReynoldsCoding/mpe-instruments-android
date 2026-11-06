package com.patrickreynolds.mpeinstruments.controllers

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.EngineShape
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.ui.PackageSurface
import com.patrickreynolds.mpeinstruments.ui.packageOptions
import org.json.JSONArray
import org.json.JSONObject
import java.util.zip.ZipInputStream

/**
 * A controller package (.odoc): its WebAssembly runs in the engine (Native.nativeCtl*), the same
 * file a computer MESH plays (CONTROLLER_FORMAT.md). Its settings come from its manifest and are
 * shown on the rail like any controller's. One runs at a time: the one in use.
 */
class PackageController(private val bytes: ByteArray, val manifest: JSONObject) : Controller {
    override val id: String = "pkg:" + manifest.optString("id")
    override val name: String get() = Controllers.nameFor(manifest.optString("name", id))
    override val description: String = manifest.optString("description") + " · from a controller package (.odoc)"
    private val orientation = manifest.optString("orientation", "landscape")
    override val landscape = orientation != "portrait" && orientation != "upright"
    override val upright = orientation == "upright"
    private val parts: List<String> = manifest.optJSONArray("parts")?.let { a -> List(a.length()) { a.optString(it) } } ?: listOf("Main")
    override val shape get() = EngineShape(split = parts.size > 1, parts = parts.size.coerceIn(2, 4), transpose = 0, bendRange = 48f)
    override val splitNames get() = if (parts.size > 1) parts else null
    override val arpUse = when (manifest.optString("arp", "app")) { "off" -> ArpUse.Off; "own" -> ArpUse.Own; else -> ArpUse.App }

    /** Its settings, in the manifest's order. */
    val settings: List<JSONObject> = manifest.optJSONArray("settings")?.let { a -> List(a.length()) { a.getJSONObject(it) } } ?: emptyList()
    var values by mutableStateOf(settings.map { default(it) })
        private set

    private fun default(s: JSONObject) = when (s.optString("type")) {
        "switch" -> if (s.optBoolean("default")) 1.0 else 0.0
        else -> s.optDouble("default", 0.0)
    }

    /** True while its WebAssembly is the one loaded in the engine. */
    private var loaded = false

    /** Load it into the engine (when its surface opens): false if it can't run. */
    fun start(): Boolean {
        if (!loaded) {
            loaded = Native.nativeCtlLoad(bytes)
            if (loaded) values.forEachIndexed { k, v -> Native.nativeCtlSetting(k, v) }
        }
        return loaded
    }

    fun change(index: Int, value: Double) {
        if (values.getOrNull(index) == value) return
        values = values.mapIndexed { k, v -> if (k == index) value else v }
        if (loaded) Native.nativeCtlSetting(index, value)
        AppState.controllerChanged(this, engine = false)
    }

    override fun save(): JSONObject = JSONObject().put("values", JSONArray(values))

    override fun load(o: JSONObject?) {
        val a = o?.optJSONArray("values")
        values = settings.mapIndexed { k, s -> if (a != null && k < a.length()) a.optDouble(k, default(s)) else default(s) }
        if (loaded) values.forEachIndexed { k, v -> Native.nativeCtlSetting(k, v) }
    }

    override fun stop() {
        if (loaded) Native.nativeCtlUnload()
        loaded = false
    }

    @Composable
    override fun options(part: Int) = packageOptions(this)

    @Composable
    override fun Surface(part: Int) = PackageSurface(this)

    companion object {
        /** The controller packages that come with the app: assets/controllers/<name>.odoc. */
        fun builtIn(context: Context): List<PackageController> =
            (runCatching { context.assets.list("controllers") }.getOrNull() ?: emptyArray())
                .filter { it.endsWith(".odoc") }.sorted()
                .mapNotNull { f -> runCatching { fromBytes(context.assets.open("controllers/$f").use { it.readBytes() }) }.getOrNull() }

        /** A package's bytes, if it is a controller package (it has controller.json). */
        fun fromBytes(bytes: ByteArray): PackageController? {
            ZipInputStream(bytes.inputStream()).use { z ->
                while (true) {
                    val e = z.nextEntry ?: return null
                    if (e.name == "controller.json") return PackageController(bytes, JSONObject(z.readBytes().decodeToString()))
                }
            }
        }
    }
}
