package com.gt.launcher;

import android.app.ActivityManager;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
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
        // Note this must not bail out by returning early: onCreate has to reach
        // super.onCreate or the framework raises SuperNotCalledException. The
        // launcher is the gate that keeps us from getting here unprepared; the
        // fallback below is best-effort for the case where it somehow does.
        String nativeLibraryDir = GameSetup.resolveNativeLibraryDir(this);
        if (nativeLibraryDir == null) {
            ApplicationInfo info = GameSetup.applicationInfo(this);
            nativeLibraryDir = info != null ? info.nativeLibraryDir : null;
            Log.w(TAG, "Native library dir unresolved; falling back to " + nativeLibraryDir);
        }

        if (nativeLibraryDir != null) {
            try {
                NativeUtils.installNativeLibraryPath(getClassLoader(), new File(nativeLibraryDir));
            } catch (Throwable e) {
                // Never fatal on its own: the loads below report the real problem.
                Log.e(TAG, "installNativeLibraryPath failed", e);
            }
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
