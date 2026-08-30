package com.rtsoft.growtopia;

import android.app.Activity;
import android.content.Context;
import android.util.Log;

/* Stub implementation - IronSource SDK not included */
public class IronSourceManager {
    private Context baseContext;
    private final String TAG = "Growtopia";
    private final String APP_KEY = "132641b31";
    boolean isIronsourceInitialized = false;
    boolean isRewardedVideoPlaying = false;
    boolean isRewardedVideoLoadingStarted = false;

    private static native void onAdClosed(String str);
    private static native void pauseAnzu();
    private static native void resumeAnzu();
    public static native void sendPingToServer();

    public IronSourceManager(Context context) {
        this.baseContext = context;
    }

    public void OnCreate() {
    }

    public void Init() {
        Log.d(TAG, "IronSourceManager.Init (stub)");
    }

    public boolean ShowRewardedAd(String str) {
        Log.d(TAG, "IronSourceManager.ShowRewardedAd (stub): " + str);
        return false;
    }

    public boolean IsShowingAd() {
        return false;
    }

    public boolean IsAdActive() {
        return false;
    }

    public void SetUserConsent(boolean z) {
    }

    public void SetUserAgeType(int i) {
    }

    public void SetCustomFields(String str, String str2) {
    }

    public void SetDynamicUserID(String str) {
    }

    public void LoadRewardedAd() {
    }

    public void onResume() {
    }

    public void onPause() {
    }
}
