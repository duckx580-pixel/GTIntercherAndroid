package com.gt.launcher;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Locates Growtopia's native libraries, extracting them from its APK when the
 * installer did not unpack them to disk.
 *
 * <p>This used to live inline in {@link Main#onCreate}, where it ran on the
 * main thread and unzipped the entire APK. That is an ANR waiting to happen:
 * Growtopia's split APK is hundreds of megabytes, and the activity was blocked
 * for the whole extraction. Everything here is main-thread-safe to *call*, but
 * {@link #extract} must be run on a background thread.
 *
 * <p>Only {@code lib/<abi>/} is extracted. The game's assets are never needed
 * from here -- App.getAssets() serves those straight from the installed
 * Growtopia package -- so unpacking anything else was wasted time and disk.
 */
public final class GameSetup {
    private static final String TAG = "GTL.GameSetup";

    public static final String GROWTOPIA_PACKAGE = "com.rtsoft.growtopia";
    private static final String NATIVE_LIB = "libgrowtopia.so";
    private static final String EXTRACTED_DIR = "extracted_apk";

    /** Reports extraction progress so the launcher can show something moving. */
    public interface ProgressListener {
        void onProgress(String message);
    }

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
     * The only ABI Growtopia 5.55 ships. It dropped armeabi-v7a, so there is
     * no 32-bit libgrowtopia.so to load or hook.
     */
    public static final String ABI = "arm64-v8a";

    /**
     * True when this device can actually run the game. A 32-bit-only device
     * has nothing to load, and reports a clearer reason than a failed
     * extraction would.
     */
    public static boolean isDeviceSupported() {
        for (String abi : Build.SUPPORTED_64_BIT_ABIS) {
            if (ABI.equals(abi)) {
                return true;
            }
        }
        return false;
    }

    private static File extractedRoot(Context context) {
        return new File(context.getCacheDir(), EXTRACTED_DIR);
    }

    private static File extractedLibDir(Context context) {
        return new File(extractedRoot(context), "lib/" + ABI);
    }

    /**
     * Directory to hand to NativeUtils, or null when the libraries are not
     * available yet. Cheap enough to call from the main thread.
     */
    public static String resolveNativeLibraryDir(Context context) {
        ApplicationInfo info = applicationInfo(context);
        if (info == null) {
            return null;
        }

        // Preferred: the installer already unpacked the libraries for us.
        if (info.nativeLibraryDir != null
            && new File(info.nativeLibraryDir, NATIVE_LIB).exists()) {
            return info.nativeLibraryDir;
        }

        File extracted = extractedLibDir(context);
        if (new File(extracted, NATIVE_LIB).exists()) {
            return extracted.getAbsolutePath();
        }

        return null;
    }

    /** True when {@link #extract} needs to run before the game can start. */
    public static boolean needsExtraction(Context context) {
        return isGrowtopiaInstalled(context) && resolveNativeLibraryDir(context) == null;
    }

    /**
     * Unpacks {@code lib/<abi>/} out of Growtopia's APKs. Run this on a
     * background thread -- it does real I/O.
     *
     * @return true when the native library is present afterwards.
     */
    public static boolean extract(Context context, ProgressListener listener) {
        ApplicationInfo info = applicationInfo(context);
        if (info == null) {
            Log.e(TAG, "Growtopia is not installed");
            return false;
        }

        // splitSourceDirs is null for a non-split install, which the previous
        // implementation dereferenced straight into a NullPointerException.
        // The base APK carries the libraries in that case, so try both.
        List<String> apks = new ArrayList<>();
        if (info.sourceDir != null) {
            apks.add(info.sourceDir);
        }
        if (info.splitSourceDirs != null) {
            for (String split : info.splitSourceDirs) {
                if (split != null) {
                    apks.add(split);
                }
            }
        }

        if (apks.isEmpty()) {
            Log.e(TAG, "No APK paths reported for " + GROWTOPIA_PACKAGE);
            return false;
        }

        File libDir = extractedLibDir(context);
        if (!libDir.exists() && !libDir.mkdirs()) {
            Log.e(TAG, "Could not create " + libDir);
            return false;
        }

        String prefix = "lib/" + ABI + "/";
        int extracted = 0;

        for (String apk : apks) {
            if (listener != null) {
                listener.onProgress("Reading " + new File(apk).getName());
            }
            extracted += extractLibsFrom(apk, prefix, libDir, listener);

            // Stop as soon as the library we actually need has landed.
            if (new File(libDir, NATIVE_LIB).exists()) {
                break;
            }
        }

        boolean ok = new File(libDir, NATIVE_LIB).exists();
        Log.i(TAG, "Extraction finished: " + extracted + " libraries, "
            + NATIVE_LIB + (ok ? " present" : " MISSING"));
        return ok;
    }

    private static int extractLibsFrom(
        String apkPath, String prefix, File destDir, ProgressListener listener) {

        int count = 0;
        try (ZipInputStream zip = new ZipInputStream(new FileInputStream(apkPath))) {
            byte[] buffer = new byte[64 * 1024];

            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                String name = entry.getName();
                if (entry.isDirectory() || !name.startsWith(prefix)) {
                    continue;
                }

                String fileName = name.substring(prefix.length());
                // Reject anything trying to escape destDir (zip-slip).
                if (fileName.isEmpty() || fileName.contains("/") || fileName.contains("\\")) {
                    Log.w(TAG, "Skipping unexpected entry: " + name);
                    continue;
                }

                File out = new File(destDir, fileName);
                if (listener != null) {
                    listener.onProgress("Extracting " + fileName);
                }

                if (!copy(zip, out, buffer)) {
                    return count;
                }
                count++;
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed reading " + apkPath, e);
        }

        return count;
    }

    private static boolean copy(InputStream in, File dest, byte[] buffer) {
        try (OutputStream out = new FileOutputStream(dest)) {
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Failed writing " + dest, e);
            // Do not leave a truncated .so behind for the next run to trust.
            //noinspection ResultOfMethodCallIgnored
            dest.delete();
            return false;
        }
    }
}
