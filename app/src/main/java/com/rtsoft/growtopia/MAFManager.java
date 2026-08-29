package com.rtsoft.growtopia;

import android.content.Context;
import android.util.Log;

/* Stub implementation - MAF/MyChips SDK not included */
public class MAFManager {
    private Context baseContext;

    public MAFManager(Context context) {
        this.baseContext = context;
    }

    public void SetUserConsent(boolean z) {
    }

    public void Init() {
        Log.d("MAFManager", "Init (stub)");
    }

    public void SetUserId(String str) {
    }

    public void SetCustomParam(int i, String str) {
    }

    public void ShowOfferwall(String str) {
        Log.d("MAFManager", "ShowOfferwall (stub): " + str);
    }
}
