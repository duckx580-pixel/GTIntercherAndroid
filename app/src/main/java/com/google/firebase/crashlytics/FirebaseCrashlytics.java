package com.google.firebase.crashlytics;

public class FirebaseCrashlytics {
    private static final FirebaseCrashlytics INSTANCE = new FirebaseCrashlytics();

    public static FirebaseCrashlytics getInstance() {
        return INSTANCE;
    }

    public void log(String message) {}
    public void recordException(Throwable throwable) {}
    public void setCustomKey(String key, String value) {}
    public void setUserId(String userId) {}
}
