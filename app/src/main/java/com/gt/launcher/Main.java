package com.gt.launcher;

import android.app.ActivityManager;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.util.Log;

import androidx.annotation.Nullable;

import com.gt.launcher.utils.NativeUtils;

import java.io.File;

public class Main extends com.rtsoft.growtopia.Main {
    static final String[] NATIVE_LIBRARIES = {
        "anzu", // We need anzu because we are using NativeUtils.installNativeLibraryPath
        "GrowtopiaFix",
        "ModMenu"
    };
    private static final String TAG = "GTL.Main";

    public static boolean isInApp() {
        ActivityManager.RunningAppProcessInfo runningAppProcessInfo = new ActivityManager.RunningAppProcessInfo();
        ActivityManager.getMyMemoryState(runningAppProcessInfo);
        return runningAppProcessInfo.importance == 100;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        // The libraries are located (and if necessary extracted) by
        // LauncherActivity on a background thread before it starts us. Doing it
        // here would block the main thread on a multi-hundred-megabyte unzip,
        // which is what used to freeze the app on launch.
        //
        // This must not bail out by returning early: onCreate has to reach
        // super.onCreate or the framework raises SuperNotCalledException, and
        // super.onCreate (com.rtsoft.growtopia.Main) unconditionally calls
        // System.loadLibrary("growtopia") with no guard of its own. That means
        // there is no recovering here once we've been started: if the native
        // libraries are not genuinely ready, super.onCreate crashes regardless
        // of what this method does. A previous version of this method "fell
        // back" to ApplicationInfo.nativeLibraryDir directly when resolution
        // failed, on the theory that something was better than nothing --
        // that was wrong. resolveNativeLibraryDir already checks that exact
        // directory first; falling back to it again after it was rejected
        // guarantees the same missing-library crash through a different path.
        // LauncherActivity is the only real gate: it must not start this
        // activity unless resolveNativeLibraryDir() is already non-null.
        String nativeLibraryDir = GameSetup.resolveNativeLibraryDir(this);
        Log.i(TAG, "resolveNativeLibraryDir() = " + nativeLibraryDir);

        if (nativeLibraryDir != null) {
            try {
                NativeUtils.installNativeLibraryPath(getClassLoader(), new File(nativeLibraryDir));
            } catch (Throwable e) {
                // Never fatal on its own: the loads below report the real problem.
                Log.e(TAG, "installNativeLibraryPath failed", e);
            }

            // Must happen before super.onCreate(), which unconditionally does
            // System.loadLibrary("growtopia") -- see GameSetup.preloadDependencies
            // for why installNativeLibraryPath's directory injection alone was
            // not enough for that call to resolve libgrowtopia.so's own
            // dependencies, confirmed on a real device.
            GameSetup.preloadDependencies(nativeLibraryDir);
        }

        try {
            for (String nativeLibrary : NATIVE_LIBRARIES) {
                System.loadLibrary(nativeLibrary);
            }
        } catch (ExceptionInInitializerError | UnsatisfiedLinkError e) {
            e.printStackTrace();
        }

        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            makeToastUI("Overlay permission is required in order to show mod menu. " + "Restart the game after you allow permission");
            startActivity(new Intent("android.settings.action.MANAGE_OVERLAY_PERMISSION",
                Uri.parse("package:" + getPackageName())
            ));
            new Handler().postDelayed(this::finish, 5000);
        } else {
            new Handler().postDelayed(() -> {
                startService(new Intent(Main.this, FloatingService.class));
            }, 700);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        stopService(new Intent(this, FloatingService.class));
    }

}
