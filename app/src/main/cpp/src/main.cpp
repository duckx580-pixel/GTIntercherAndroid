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
// full reasoning.
//
// This used to run on its own unbounded background std::thread that polled
// dlopen(RTLD_NOLOAD) forever and then called game::hook::init() the moment
// libgrowtopia.so appeared -- a second thread, entirely uncoordinated with
// ModMenu's own (former) polling thread and with Growtopia's own startup,
// touching the process via DobbySymbolResolver at an arbitrary point in time.
// That is the same shape of race suspected of causing an intermittent
// SIGSEGV inside libhoudini.so (Houdini, the ARM-to-x86 translator MEmu uses)
// on real test runs. Calling this synchronously and only once, at the one
// point where growtopia.so is guaranteed loaded but nothing of Growtopia's
// has started running yet, removes that race for this library too.
extern "C" JNIEXPORT void JNICALL Java_com_gt_launcher_ModMenuBridge_installFixHooks(JNIEnv* /*env*/, jclass /*clazz*/)
{
    if (dlopen("libgrowtopia.so", RTLD_NOLOAD) == nullptr) {
        LOGE("installFixHooks: libgrowtopia.so is not loaded; skipping");
        return;
    }

    game::hook::init();
}
