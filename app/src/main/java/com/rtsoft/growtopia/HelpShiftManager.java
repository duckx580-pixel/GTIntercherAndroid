package com.rtsoft.growtopia;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.util.Log;
import java.util.HashMap;

/* Stub implementation - HelpShift SDK not included */
public class HelpShiftManager {
    private Context baseContext;

    public HelpShiftManager(Context context) {
        this.baseContext = context;
    }

    public String getDeviceInfo() {
        return ((("android version:" + Build.VERSION.RELEASE + "(" + Build.VERSION.INCREMENTAL + ")") + ";\nandroid API Level:" + Build.VERSION.SDK_INT) + ";\ndevice:" + Build.DEVICE) + ";\nmodel:" + Build.MODEL;
    }

    public void Init() {
        Log.d("HelpShift", "HelpShiftManager.Init (stub)");
    }

    public void ShowConversation(HashMap<String, Object> map) {
        Log.d("HelpShift", "HelpShiftManager.ShowConversation (stub)");
    }

    public void ShowFAQs(HashMap<String, Object> map) {
        Log.d("HelpShift", "HelpShiftManager.ShowFAQs (stub)");
    }

    public void SetLanguage(String str) {
    }

    public static void SetConfigValue(HashMap<String, Object> map, String str, String str2, Object obj) {
        HashMap<String, Object> map2 = new HashMap<>();
        map2.put("type", str2);
        map2.put("value", obj);
        map.put(str, map2);
    }

    public boolean HandleDeeplink(Intent intent) {
        Uri data = intent.getData();
        if (data == null) {
            return false;
        }
        if (data.getHost() == null || !data.getHost().contains("helpshift")) {
            return false;
        }
        return false;
    }
}
