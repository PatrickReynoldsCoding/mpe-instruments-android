package com.patrickreynolds.mpeinstruments

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.content.pm.PackageInfo
import android.content.pm.PackageInstaller
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.util.Log
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import org.json.JSONObject
import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors

/**
 * Updates from inside the app. The releases are APKs on GitHub, so: ask GitHub for the latest
 * release, download its APK, check it is this app, newer, and signed with the same key, and hand
 * it to Android's installer. Android asks the user to confirm (and, the first time, to allow this
 * app to install apps). The only network access the app has is this, to GitHub.
 */
object Updater {
    const val REPO = "PatrickReynoldsCoding/odo"
    private const val TAG = "mpe"
    private const val DAY_MS = 20L * 60 * 60 * 1000          // "once a day", with some slack

    data class Release(val version: String, val tag: String, val notes: String, val apkUrl: String, val apkName: String, val size: Long)

    sealed class State {
        object Idle : State()
        object Checking : State()
        data class UpToDate(val latest: String) : State()
        data class Available(val release: Release) : State()
        data class Downloading(val release: Release, val progress: Float) : State()
        /** Downloaded and checked; waiting for "install unknown apps" to be allowed for this app. */
        data class NeedsPermission(val release: Release, val apk: File) : State()
        data class Installing(val release: Release) : State()
        data class Failed(val message: String, val release: Release? = null) : State()
    }

    var state by mutableStateOf<State>(State.Idle)
        private set
    var autoCheck by mutableStateOf(true)
        private set
    var lastCheckMs by mutableStateOf(0L)
        private set

    private lateinit var prefs: SharedPreferences
    private val worker = Executors.newSingleThreadExecutor { r -> Thread(r, "updater") }
    private val main = Handler(Looper.getMainLooper())
    private fun post(s: State) = main.post { state = s }

    fun init(context: Context) {
        if (::prefs.isInitialized) return
        prefs = context.applicationContext.getSharedPreferences("updates", Context.MODE_PRIVATE)
        autoCheck = prefs.getBoolean("auto", true)
        lastCheckMs = prefs.getLong("last", 0L)
    }

    fun changeAutoCheck(on: Boolean) { autoCheck = on; prefs.edit().putBoolean("auto", on).apply() }

    /** On opening the app: check, if that is on and the last check was a day or more ago. */
    fun checkIfDue(context: Context) {
        init(context)
        if (autoCheck && System.currentTimeMillis() - lastCheckMs > DAY_MS && state == State.Idle) check(context)
    }

    fun check(context: Context) {
        init(context)
        if (state is State.Checking || state is State.Downloading || state is State.Installing) return
        state = State.Checking
        worker.execute {
            try {
                val r = latest()
                val now = System.currentTimeMillis()
                main.post { lastCheckMs = now; prefs.edit().putLong("last", now).apply() }
                post(if (r != null && newer(r.version, BuildConfig.VERSION_NAME)) State.Available(r) else State.UpToDate(r?.version ?: BuildConfig.VERSION_NAME))
            } catch (e: Exception) {
                Log.w(TAG, "update check failed", e)
                post(State.Failed("Could not reach GitHub: ${e.message ?: e.javaClass.simpleName}"))
            }
        }
    }

    /** The latest published release (GitHub leaves drafts and pre-releases out of "latest"). */
    private fun latest(): Release? {
        val c = URL("https://api.github.com/repos/$REPO/releases/latest").openConnection() as HttpURLConnection
        c.connectTimeout = 10_000; c.readTimeout = 15_000
        c.setRequestProperty("Accept", "application/vnd.github+json")
        c.setRequestProperty("User-Agent", "odo/${BuildConfig.VERSION_NAME}")
        try {
            if (c.responseCode == 404) return null                     // no release yet
            if (c.responseCode != 200) throw IllegalStateException("GitHub answered ${c.responseCode}")
            val o = JSONObject(c.inputStream.bufferedReader().readText())
            val tag = o.getString("tag_name")
            val assets = o.optJSONArray("assets")
            var apk: JSONObject? = null
            for (i in 0 until (assets?.length() ?: 0)) assets!!.getJSONObject(i).let { if (it.getString("name").endsWith(".apk")) apk = it }
            val a = apk ?: return null                                  // a release without an APK cannot be installed
            return Release(tag.removePrefix("v"), tag, o.optString("body", ""), a.getString("browser_download_url"),
                           a.getString("name"), a.optLong("size", -1))
        } finally { c.disconnect() }
    }

    /** "0.10.0" > "0.9.2": compared number by number; anything after a '-' is ignored. */
    fun newer(candidate: String, current: String): Boolean {
        fun parts(v: String) = v.substringBefore('-').split('.').map { it.trim().toIntOrNull() ?: 0 }
        val a = parts(candidate); val b = parts(current)
        for (i in 0 until maxOf(a.size, b.size)) {
            val x = a.getOrElse(i) { 0 }; val y = b.getOrElse(i) { 0 }
            if (x != y) return x > y
        }
        return false
    }

    fun downloadAndInstall(context: Context, r: Release) {
        val app = context.applicationContext
        if (state is State.Downloading || state is State.Installing) return
        state = State.Downloading(r, 0f)
        worker.execute {
            try {
                val dir = File(app.cacheDir, "updates").apply { mkdirs() }
                dir.listFiles()?.forEach { it.delete() }                 // an older download
                val apk = File(dir, r.apkName)
                download(r, apk)
                verify(app, apk)
                if (!app.packageManager.canRequestPackageInstalls()) { post(State.NeedsPermission(r, apk)); return@execute }
                install(app, r, apk)
            } catch (e: Exception) {
                Log.w(TAG, "update failed", e)
                post(State.Failed(e.message ?: e.javaClass.simpleName, r))
            }
        }
    }

    /** After "install unknown apps" was allowed in Android's settings. */
    fun continueInstall(context: Context) {
        val s = state as? State.NeedsPermission ?: return
        if (!context.packageManager.canRequestPackageInstalls()) return
        state = State.Installing(s.release)
        worker.execute {
            try { install(context.applicationContext, s.release, s.apk) }
            catch (e: Exception) { post(State.Failed(e.message ?: e.javaClass.simpleName, s.release)) }
        }
    }

    /** Android's page where this app is allowed to install apps (its updates). */
    fun permissionSettings(context: Context) = Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:${context.packageName}"))

    private fun download(r: Release, to: File) {
        val c = URL(r.apkUrl).openConnection() as HttpURLConnection   // follows GitHub's redirect to its file host
        c.connectTimeout = 15_000; c.readTimeout = 30_000
        c.setRequestProperty("User-Agent", "odo/${BuildConfig.VERSION_NAME}")
        try {
            if (c.responseCode != 200) throw IllegalStateException("Download failed: GitHub answered ${c.responseCode}")
            val total = c.contentLengthLong.takeIf { it > 0 } ?: r.size
            var got = 0L
            var shown = -1
            c.inputStream.use { input -> to.outputStream().use { out ->
                val buf = ByteArray(64 * 1024)
                while (true) {
                    val n = input.read(buf)
                    if (n < 0) break
                    out.write(buf, 0, n)
                    got += n
                    val pct = if (total > 0) (100 * got / total).toInt() else 0
                    if (pct != shown) { shown = pct; post(State.Downloading(r, pct / 100f)) }
                }
            } }
            if (total > 0 && got != total) throw IllegalStateException("Download cut short ($got of $total bytes)")
        } finally { c.disconnect() }
    }

    /** It must be this app, newer than this one, and signed with the same key. */
    private fun verify(context: Context, apk: File) {
        val pm = context.packageManager
        val flags = PackageManager.GET_SIGNING_CERTIFICATES
        val new = pm.getPackageArchiveInfo(apk.path, flags) ?: throw IllegalStateException("The download is not an app (damaged?)")
        if (new.packageName != context.packageName) throw IllegalStateException("The download is a different app (${new.packageName})")
        val mine = pm.getPackageInfo(context.packageName, flags)
        if (new.longVersionCode <= mine.longVersionCode)
            throw IllegalStateException("The download (${new.versionName}) is not newer than this app (${mine.versionName})")
        val a = signers(new); val b = signers(mine)
        if (a != null && b != null && a != b)
            throw IllegalStateException(if (BuildConfig.DEBUG) "This is a test build, signed with a different key from the releases, " +
                "so it cannot update itself. Install the release from GitHub once." else "The download is signed with a different key: not installed.")
    }

    private fun signers(p: PackageInfo): Set<String>? =
        p.signingInfo?.apkContentsSigners?.map { it.toCharsString() }?.toSet()

    private fun install(context: Context, r: Release, apk: File) {
        post(State.Installing(r))
        val installer = context.packageManager.packageInstaller
        val params = PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL).apply {
            setAppPackageName(context.packageName)
            setSize(apk.length())
            // An app updating itself may do so without a confirmation on Android 12+; Android asks anyway if it wants to.
            setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_NOT_REQUIRED)
        }
        val id = installer.createSession(params)
        installer.openSession(id).use { s ->
            s.openWrite("base.apk", 0, apk.length()).use { out ->
                apk.inputStream().use { it.copyTo(out) }
                s.fsync(out)
            }
            val intent = Intent(context, UpdateReceiver::class.java).setAction(UpdateReceiver.ACTION)
            val pi = PendingIntent.getBroadcast(context, id, intent, PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE)
            prefs.edit().putBoolean("updating", true).commit()          // for UpdatedReceiver in the new version
            s.commit(pi.intentSender)
        }
        Log.i(TAG, "update: installing ${r.version}")
    }

    /** True once, in the new version, if the update came from here (not from adb or a file manager). */
    internal fun takeUpdatedFlag(context: Context): Boolean {
        init(context)
        val was = prefs.getBoolean("updating", false)
        if (was) prefs.edit().putBoolean("updating", false).apply()
        return was
    }

    internal fun installFailed(message: String) {
        val r = (state as? State.Installing)?.release
        prefs.edit().putBoolean("updating", false).apply()
        state = State.Failed(message, r)
    }
}

/** What Android's installer says about the update: a confirmation to show, or how it went. */
class UpdateReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        when (val status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE)) {
            PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                val confirm = (if (android.os.Build.VERSION.SDK_INT >= 33) intent.getParcelableExtra(Intent.EXTRA_INTENT, Intent::class.java)
                               else @Suppress("DEPRECATION") intent.getParcelableExtra(Intent.EXTRA_INTENT)) ?: return
                context.startActivity(confirm.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
            }
            PackageInstaller.STATUS_SUCCESS -> Log.i("mpe", "update: installed")  // this process is about to be replaced
            else -> {
                val msg = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE)
                Log.w("mpe", "update: install failed, status $status: $msg")
                Updater.installFailed(if (status == PackageInstaller.STATUS_FAILURE_ABORTED) "Update cancelled." else "Android did not install it: ${msg ?: status}")
            }
        }
    }

    companion object { const val ACTION = "com.patrickreynolds.mpeinstruments.UPDATE_STATUS" }
}

/** Runs in the new version straight after an update: a note to say so, to open the app again. */
class UpdatedReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != Intent.ACTION_MY_PACKAGE_REPLACED || !Updater.takeUpdatedFlag(context)) return
        val nm = context.getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(NotificationChannel("updates", "Updates", NotificationManager.IMPORTANCE_DEFAULT))
        val open = PendingIntent.getActivity(context, 0, Intent(context, MainActivity::class.java), PendingIntent.FLAG_IMMUTABLE)
        nm.notify(2, Notification.Builder(context, "updates")
            .setSmallIcon(R.drawable.ic_notification)
            .setContentTitle("odo updated to ${BuildConfig.VERSION_NAME}")
            .setContentText("Tap to open")
            .setContentIntent(open)
            .setAutoCancel(true)
            .build())
    }
}
