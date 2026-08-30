package com.gt.launcher;

// Called from com.rtsoft.growtopia.Main.onCreate() right after
// System.loadLibrary("growtopia") returns, so both native libraries' hooks
// are installed synchronously on the main thread before Growtopia creates
// its GLSurfaceView / starts its render thread. See the comment above
// Java_com_gt_launcher_ModMenuBridge_installHooks in ModMenu/main.cpp for why
// that timing matters.
public final class ModMenuBridge {
    private ModMenuBridge() {}

    public static void onGrowtopiaLibraryLoaded() {
        try {
            installFixHooks();
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
        try {
            installHooks();
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }

    // Implemented in GrowtopiaFix (src/main.cpp).
    private static native void installFixHooks();

    // Implemented in ModMenu (ModMenu/main.cpp).
    private static native void installHooks();
}
