// A pretend Seaboard for the emulator: a virtual MIDI device that plays recordings and test
// gestures, so the app can be tested end to end without the real controller. Debug use only.
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.patrickreynolds.fakeseaboard"
    compileSdk = 35
    defaultConfig {
        applicationId = "com.patrickreynolds.fakeseaboard"
        minSdk = 31
        targetSdk = 35
        versionCode = 1
        versionName = "0.1"
    }
    sourceSets["main"].assets.srcDirs("../captures")      // the recorded Seaboard sessions
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
}
