package com.anzu.sdk;

import android.content.Context;
import android.util.Log;

/**
 * Neutralized stub of the Anzu in-game advertising SDK.
 *
 * <p>The previous copy of this package was carried over from an older
 * Growtopia and no longer matches the SDK that ships with 5.55. The
 * signatures had drifted -- for example {@code sdkAndroidInit}'s last two
 * parameters went from {@code (Class, Class)} to a single {@code String}.
 * JNI binds a lone {@code native} overload by its short name, so that call
 * would have bound successfully and then handed libanzu.so two jclass values
 * where it expected a jstring.
 *
 * <p>Stubbing rather than porting is safe here, and was verified against the
 * 5.55 arm64 binaries:
 * <ul>
 *   <li>libgrowtopia.so contains no references to {@code com/anzu} at all, so
 *       the game never reaches these classes.</li>
 *   <li>libanzu.so's {@code JNI_OnLoad} only stores the JavaVM pointer and
 *       returns JNI_VERSION_1_4 -- it performs no FindClass or GetMethodID --
 *       so loading the library does not require this class to match.</li>
 *   <li>libanzu.so is still loaded, because libgrowtopia.so carries a
 *       DT_NEEDED on it. That link is unaffected by this file.</li>
 * </ul>
 *
 * <p>Since nothing calls into libanzu.so from here, no advertising code runs
 * and there is no signature to mismatch. This mirrors how IronSource,
 * Firebase, HelpShift, Usercentrics and MAF are already handled in this
 * project.
 */
public class Anzu {
    private static final String TAG = "ANZU";

    private static Context appContext;

    public static void SetContext(Context context) {
        appContext = context != null ? context.getApplicationContext() : null;
        Log.i(TAG, "Anzu SDK is stubbed out; no advertising will be initialized.");
    }

    public static Context GetContext() {
        return appContext;
    }

    public static void SetActivity(Object activity) {}

    public static void OnPause() {}

    public static void OnResume() {}

    public static void Pause() {}

    public static void Resume() {}

    public static void updateGdprConsent(String consent) {}
}
