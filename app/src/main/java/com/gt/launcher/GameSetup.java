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

    /**
     * libgrowtopia.so's own DT_NEEDED entries that are NOT part of Android's
     * system linker namespace (confirmed with readelf -d against the real
     * 5.55 binary) -- everything else it needs (libGLESv2, libEGL, libc,
     * libandroid, libdl, liblog, libz, libm) is always resolvable regardless
     * of native library directory, so only these three have to actually be
     * found alongside it.
     *
     * Checking for libgrowtopia.so alone is not enough: on a split-APK
     * install, ApplicationInfo.nativeLibraryDir can report a directory that
     * has libgrowtopia.so merged into it but not one of these, if it shipped
     * in a different split. dlopen then fails on the *dependency*'s name,
     * not the library actually requested -- exactly what surfaced as
     * "UnsatisfiedLinkError: dlopen failed: library libanzu.so not found"
     * at Main.java's System.loadLibrary("growtopia") call, even though
     * libgrowtopia.so itself was present.
     */
    private static final String[] REQUIRED_LIBS = {
        NATIVE_LIB, "libanzu.so", "libsqliteX.so", "libc++_shared.so"
    };

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

        // Deliberately NOT using ApplicationInfo.nativeLibraryDir here, even
        // when it looks complete. On a real device this passed
        // hasAllRequiredLibs() for every required library, including
        // libanzu.so -- File.exists() was true -- and dlopen still failed
        // to find libanzu.so. That directory belongs to another app's
        // private storage; whatever the exact reason (a corrupt or
        // zero-byte file from however the installer merged split APKs, or
        // SELinux declining to let this process load another app's native
        // library even though it can stat() it), loading from it is not
        // reliable. Our own extracted copy lives in storage this app fully
        // owns, sidestepping that whole class of problem, so it is the only
        // path trusted for loading now.
        File extracted = extractedLibDir(context);
        if (hasAllRequiredLibs(extracted, true)) {
            return extracted.getAbsolutePath();
        }

        return null;
    }

    private static boolean hasAllRequiredLibs(File dir) {
        return hasAllRequiredLibs(dir, false);
    }

    private static boolean hasAllRequiredLibs(File dir, boolean verbose) {
        boolean ok = true;
        for (String lib : REQUIRED_LIBS) {
            File f = new File(dir, lib);
            long length = f.exists() ? f.length() : -1;
            // A file that exists but is 0 bytes cannot be a valid ELF
            // shared object; treat it the same as missing so a corrupted
            // extraction gets retried rather than trusted.
            boolean present = length > 0;
            if (verbose) {
                Log.i(TAG, "  " + lib + ": " + (present ? length + " bytes" : "MISSING (length=" + length + ")"));
            }
            if (!present) {
                ok = false;
            }
        }
        return ok;
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

        Log.i(TAG, "Scanning " + apks.size() + " APK(s) for lib/" + ABI + "/*:");
        for (String apk : apks) {
            Log.i(TAG, "  " + apk);
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

            // Stop once every library libgrowtopia.so needs has landed. Each
            // one can live in a different split APK, so this has to keep
            // scanning rather than stopping the moment libgrowtopia.so
            // itself is found -- that was exactly the bug that let a
            // directory missing libanzu.so pass as "ready".
            if (hasAllRequiredLibs(libDir)) {
                break;
            }
        }

        boolean ok = hasAllRequiredLibs(libDir, true);
        Log.i(TAG, "Extraction finished: " + extracted + " files copied, required set "
            + (ok ? "complete" : "INCOMPLETE"));
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

                long size = entry.getSize();
                if (!copy(zip, out, buffer)) {
                    return count;
                }
                Log.i(TAG, "  found " + fileName + " (" + size + " bytes reported) in "
                    + new File(apkPath).getName() + " -> wrote " + out.length() + " bytes");
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
