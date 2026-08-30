package com.gt.launcher;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build;

/**
 * Checks whether Growtopia is installed, for its game assets.
 *
 * <p>This used to also locate and extract Growtopia's native libraries at
 * runtime (they used to not be part of this app at all). That approach hit a
 * native-loading crash that could not be made reliable -- compared directly
 * against RealGrowlauncher, a real, working Growtopia launcher, whose fix was
 * architectural: it bundles Growtopia's native libraries (libgrowtopia.so,
 * libanzu.so, libsqliteX.so, libc++_shared.so) directly in its own APK's
 * standard native library directory, rather than reaching into another app's
 * installed files at runtime. This project now does the same -- see
 * app/src/main/jniLibs/arm64-v8a/ -- so none of that extraction machinery is
 * needed here any more.
 *
 * <p>Growtopia's own game assets (items.dat, UI textures, audio, ...) are
 * likewise bundled directly in this app's own src/main/assets/ now, rather
 * than reached via a runtime redirect into the installed Growtopia package's
 * AssetManager -- that redirect only ever covered two specific Java-side
 * audio-loading call sites and did nothing for the native engine's own asset
 * reads, which go through Activity.getAssets() directly from C++. See the
 * comment on the (now removed) getAssets() override in App.java.
 */
public final class GameSetup {
    public static final String GROWTOPIA_PACKAGE = "com.rtsoft.growtopia";

    /**
     * The only ABI Growtopia 5.55 ships. It dropped armeabi-v7a, so there is
     * no 32-bit libgrowtopia.so to load or hook.
     */
    public static final String ABI = "arm64-v8a";

    private GameSetup() {}

    public static boolean isGrowtopiaInstalled(Context context) {
        return applicationInfo(context) != null;
    }

    public static ApplicationInfo applicationInfo(Context context) {
        try {
            return context.getPackageManager().getApplicationInfo(GROWTOPIA_PACKAGE, 0);
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
    }

    /**
     * True when this device can actually run the game. A 32-bit-only device
     * has nothing to load, and reports a clearer reason than a failed
     * asset lookup would.
     */
    public static boolean isDeviceSupported() {
        for (String abi : Build.SUPPORTED_64_BIT_ABIS) {
            if (ABI.equals(abi)) {
                return true;
            }
        }
        return false;
    }
}
