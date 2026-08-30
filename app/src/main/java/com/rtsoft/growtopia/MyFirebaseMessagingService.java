package com.rtsoft.growtopia;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

/* Stub implementation - Firebase Messaging SDK not included */
public class MyFirebaseMessagingService extends Service {
    private static final String TAG = "MyFirebaseMsgService";

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    public void onMessageReceived(Object remoteMessage) {
        Log.d(TAG, "onMessageReceived (stub)");
    }

    public void onNewToken(String str) {
        Log.d(TAG, "Refreshed token (stub): " + str);
    }
}
