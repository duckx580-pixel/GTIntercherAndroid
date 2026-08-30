package com.gt.launcher;

import android.app.Application;

import java.util.Arrays;

public class App extends Application {
    // Package string (with method name) to rename package to Growtopia package name.
    // Only needed for the AppsFlyer SDK, which keys attribution off the
    // package name it observes -- unrelated to asset loading (see the
    // removed getAssets() override below).
    final static String[] CHANGE_PACKAGE_NAMES = {
        "com.appsflyer.internal"
    };

    @Override
    public String getPackageName() {
        StackTraceElement[] stackTraceElements = Thread.currentThread().getStackTrace();
        String stackTraceElement = Arrays.toString(stackTraceElements);

        for (String changePackageName : CHANGE_PACKAGE_NAMES) {
            if (stackTraceElement.contains(changePackageName)) {
                return "com.rtsoft.growtopia";
            }
        }

        return super.getPackageName();
    }

    // This used to override getAssets() to redirect two specific Java call
    // sites (SharedActivity.music_play/sound_load) to the installed
    // Growtopia app's own AssetManager via createPackageContext(), by
    // pattern-matching the calling thread's stack trace. That only ever
    // covered those two audio-loading methods -- it did nothing for the
    // native engine's own asset reads (items.dat, UI textures, etc.), which
    // go through Activity.getAssets() directly from C++ and never touch
    // Java code at all, so no stack-trace-based Java hook could catch them.
    // Since this app never bundled any Growtopia asset files of its own,
    // every one of those native reads found nothing, and Growtopia rendered
    // a black screen forever: no crash, just nothing to draw.
    //
    // Fixed by bundling Growtopia's own asset files directly in this app's
    // own src/main/assets/ (items.dat, interface/, GameData/, audio/,
    // dexopt/, fonts/) -- the same category of fix as bundling Growtopia's
    // native .so files instead of extracting them at runtime, and matching
    // how RealGrowlauncher (a real, working Growtopia launcher) does it.
    // With Growtopia's own assets actually present in this app's APK, the
    // ordinary unoverridden Application.getAssets() finds them directly, so
    // no redirect of any kind is needed anymore.
}
