#include <thread>
#include <dlfcn.h>
#include <jni.h>
#include <android/log.h>

#include "game/hook.h"

#define LOG_TAG "GTL.Fix"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Called from com.rtsoft.growtopia.Main.onCreate(), immediately after
// System.loadLibrary("growtopia") returns and before super.onCreate() starts
// Growtopia's own GL render thread. This is called straight from
// Activity.onCreate() on the main/UI thread, so the actual work below runs on
// a spawned background thread rather than inline here: doing this
// synchronously on the main thread was found (with a since-removed ImGui mod
// menu that had its own, separate reasons for calling this at this exact
// point) to stall onCreate() long enough to freeze the whole app on this
// (slow) emulator -- no crash, just a black screen until the process had to
// be force-stopped.
//
// This used to run on its own unbounded background std::thread that polled
// dlopen(RTLD_NOLOAD) forever from process launch and then called
// game::hook::init() the moment libgrowtopia.so appeared -- a thread with no
// ordering relationship to Growtopia's own startup at all, touching the
// process via DobbySymbolResolver at an arbitrary point in time. That is the
// same shape of race suspected of causing an intermittent SIGSEGV inside
// libhoudini.so (Houdini, the ARM-to-x86 translator MEmu uses) on other test
// runs. Spawning the background thread here instead, the instant
// growtopia.so finishes loading, keeps the work off the main thread while
// shrinking that race window down to the fixed, comparatively small amount
// of Java-side setup SharedActivity.onCreate() does before it creates the
// GLSurfaceView.
//
// In practice this hook is a no-op on Growtopia 5.55 either way: both
// symbols it looks for (LogMsg, SendPacket) are internal engine symbols
// stripped from this build's libgrowtopia.so, so game::hook::init() always
// fails to resolve them and never calls DobbyHook. It is kept only for
// compatibility with older Growtopia builds that still export them.
extern "C" JNIEXPORT void JNICALL Java_com_gt_launcher_GrowtopiaFixBridge_installHooks(JNIEnv* /*env*/, jclass /*clazz*/)
{
    if (dlopen("libgrowtopia.so", RTLD_NOLOAD) == nullptr) {
        LOGE("installFixHooks: libgrowtopia.so is not loaded; skipping");
        return;
    }

    std::thread([]() {
        game::hook::init();
    }).detach();
}
