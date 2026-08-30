#include <thread>
#include <dlfcn.h>
#include <jni.h>
#include <android/log.h>

#include "game/hook.h"

#define LOG_TAG "GTL.Fix"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Called from com.rtsoft.growtopia.Main.onCreate(), immediately after
// System.loadLibrary("growtopia") returns and before super.onCreate() starts
// Growtopia's own GL render thread. See the matching comment above
// Java_com_gt_launcher_ModMenuBridge_installHooks in ModMenu/main.cpp for the
// full reasoning, including why the actual work below runs on a spawned
// background thread rather than inline here: this is called straight from
// Activity.onCreate() on the main/UI thread, and doing this synchronously
// there was found to stall onCreate() long enough to freeze the whole app on
// this (slow) emulator -- no crash, just a black screen until the process
// had to be force-stopped.
//
// This used to run on its own unbounded background std::thread that polled
// dlopen(RTLD_NOLOAD) forever from process launch and then called
// game::hook::init() the moment libgrowtopia.so appeared -- a second thread,
// entirely uncoordinated with ModMenu's own (former) polling thread and with
// Growtopia's own startup, touching the process via DobbySymbolResolver at
// an arbitrary point in time. That is the same shape of race suspected of
// causing an intermittent SIGSEGV inside libhoudini.so (Houdini, the
// ARM-to-x86 translator MEmu uses) on other test runs. Spawning the
// background thread here instead, the instant growtopia.so finishes
// loading, keeps the work off the main thread while shrinking that race
// window down to the fixed, comparatively small amount of Java-side setup
// SharedActivity.onCreate() does before it creates the GLSurfaceView.
extern "C" JNIEXPORT void JNICALL Java_com_gt_launcher_ModMenuBridge_installFixHooks(JNIEnv* /*env*/, jclass /*clazz*/)
{
    if (dlopen("libgrowtopia.so", RTLD_NOLOAD) == nullptr) {
        LOGE("installFixHooks: libgrowtopia.so is not loaded; skipping");
        return;
    }

    std::thread([]() {
        game::hook::init();
    }).detach();
}
