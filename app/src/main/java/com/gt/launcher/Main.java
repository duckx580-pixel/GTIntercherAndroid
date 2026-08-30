package com.gt.launcher;

import android.app.ActivityManager;
import android.os.Bundle;

import androidx.annotation.Nullable;

public class Main extends com.rtsoft.growtopia.Main {
    static final String[] NATIVE_LIBRARIES = {
        "GrowtopiaFix"
    };
    private static final String TAG = "GTL.Main";

    public static boolean isInApp() {
        ActivityManager.RunningAppProcessInfo runningAppProcessInfo = new ActivityManager.RunningAppProcessInfo();
        ActivityManager.getMyMemoryState(runningAppProcessInfo);
        return runningAppProcessInfo.importance == 100;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        // libgrowtopia.so and its own dependencies (libanzu.so, libsqliteX.so,
        // libc++_shared.so) are bundled directly in this app's own
        // src/main/jniLibs/arm64-v8a/, exactly like any normal native library
        // this app ships -- matching how RealGrowlauncher (a real, working
        // Growtopia launcher) does it, which was compared directly against
        // this project to resolve a native-loading crash that a
        // reflection/extraction-based approach could not get past reliably.
        //
        // Deliberately NOT calling System.loadLibrary("anzu")/("sqliteX") (or
        // System.load on their paths) here or anywhere: doing so previously
        // caused a *different* crash. JNI_OnLoad is invoked by the JVM only in
        // response to an explicit System.load/loadLibrary call for that exact
        // library -- not automatically for a library the native dynamic
        // linker resolves silently as another library's transitive
        // dependency. Explicitly loading libsqliteX.so by name triggered its
        // JNI_OnLoad, which eagerly resolves a Java support class
        // (org.sqlite.database.sqlite.SQLiteCustomFunction) that Growtopia's
        // own app ships but this one does not; missing it took the whole
        // process down. Leaving these libraries unbundled-by-name and only
        // present in the standard search path lets System.loadLibrary
        // ("growtopia") below resolve them silently via the ordinary
        // dynamic linker, the same way RealGrowlauncher's does, without ever
        // triggering their own JNI_OnLoad.
        try {
            for (String nativeLibrary : NATIVE_LIBRARIES) {
                System.loadLibrary(nativeLibrary);
            }
        } catch (ExceptionInInitializerError | UnsatisfiedLinkError e) {
            e.printStackTrace();
        }

        super.onCreate(savedInstanceState);

        // Plain Android-View overlay, added directly into this activity's own
        // view hierarchy -- see ModMenuOverlay's own comment for why this
        // replaced an earlier ImGui-drawn-inside-Growtopia's-own-render-hook
        // menu that could freeze the whole game.
        new ModMenuOverlay(this);
    }

}
