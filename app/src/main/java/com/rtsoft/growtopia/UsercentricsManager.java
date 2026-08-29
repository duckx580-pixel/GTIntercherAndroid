package com.rtsoft.growtopia;

import android.app.Activity;
import android.util.Log;
import java.util.List;

/* Stub implementation - Usercentrics SDK not included */
public class UsercentricsManager {
    private Activity baseContext;

    native void InitFinish(boolean z);
    native void OnConsentFetchedFail(int i, String str);
    native void OnConsentFetchedSuccess(List list);

    public UsercentricsManager(Activity activity) {
        this.baseContext = activity;
    }

    public void InitWithSettings(String str) {
        Log.d("Usercentrics", "InitWithSettings (stub): " + str);
    }

    public void InitWithRuleSet(String str) {
        Log.d("Usercentrics", "InitWithRuleSet (stub): " + str);
    }

    public void CheckConsentState() {
        Log.d("Usercentrics", "CheckConsentState (stub)");
    }

    public void ShowConsentSettings() {
        Log.d("Usercentrics", "ShowConsentSettings (stub)");
    }
}
