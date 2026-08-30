package com.gt.launcher;

// Called from com.rtsoft.growtopia.Main.onCreate() right after
// System.loadLibrary("growtopia") returns, so GrowtopiaFix's hooks are
// installed before Growtopia creates its GLSurfaceView / starts its render
// thread. See the comment above Java_com_gt_launcher_GrowtopiaFixBridge_installHooks
// in src/main.cpp for why that timing matters.
public final class GrowtopiaFixBridge {
    private GrowtopiaFixBridge() {}

    public static void onGrowtopiaLibraryLoaded() {
        try {
            installHooks();
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }

    // Implemented in GrowtopiaFix (src/main.cpp).
    private static native void installHooks();
}
