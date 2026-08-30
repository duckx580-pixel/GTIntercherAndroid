#include <string>
#include <dlfcn.h>
#include <jni.h>
#include <KittyMemory.h>

#include "game/hook.h"
#include "utils/log.h"
#include "ui/ui.h"
#include "mod_menu.h"

ModMenu* g_mod_menu{ nullptr };
JavaVM* g_jvm{ nullptr };
jobject g_class_loader{ nullptr };
jmethodID g_load_class_method{ nullptr };

// System.loadLibrary("ModMenu") triggers this automatically; it is the
// standard, documented way a native library obtains a JavaVM* for later use.
//
// It also captures the app's own classloader. This used to matter for a
// background poller thread that no longer exists (see the comment above
// Java_com_gt_launcher_ModMenuBridge_installHooks for why), but is kept as
// a defensive fallback for find_app_class in case anything ever needs to
// resolve an application class from a thread that isn't a proper
// Java-originated one: JNIEnv::FindClass called from a thread attached via
// AttachCurrentThread resolves against the boot classloader instead of the
// app's own PathClassLoader -- a well-known Android JNI pitfall -- so it
// fails to find application classes like com.rtsoft.growtopia.AppRenderer.
// JNI_OnLoad itself runs synchronously on the thread that called
// System.loadLibrary("ModMenu"), which *is* a proper Java-originated thread,
// so FindClass here resolves correctly; the standard fix is to fetch that
// class's ClassLoader object here and use its loadClass() method later
// instead of calling FindClass directly from an attached thread.
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    g_jvm = vm;

    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || env == nullptr) {
        LOGE("JNI_OnLoad: GetEnv failed");
        return JNI_VERSION_1_6;
    }

    jclass any_app_class = env->FindClass("com/rtsoft/growtopia/AppRenderer");
    if (any_app_class == nullptr) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        LOGE("JNI_OnLoad: could not FindClass AppRenderer to capture the classloader");
        return JNI_VERSION_1_6;
    }

    jclass class_class = env->GetObjectClass(any_app_class);
    jmethodID get_class_loader = env->GetMethodID(class_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject class_loader = env->CallObjectMethod(any_app_class, get_class_loader);

    jclass class_loader_class = env->GetObjectClass(class_loader);
    g_load_class_method = env->GetMethodID(class_loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    g_class_loader = env->NewGlobalRef(class_loader);

    env->DeleteLocalRef(any_app_class);
    env->DeleteLocalRef(class_class);
    env->DeleteLocalRef(class_loader);
    env->DeleteLocalRef(class_loader_class);

    LOGI("JNI_OnLoad: captured app classloader");
    return JNI_VERSION_1_6;
}

// Resolves an application class by its JNI-style slash-separated name (e.g.
// "com/rtsoft/growtopia/AppRenderer", matching what FindClass takes)
// through the classloader captured in JNI_OnLoad, rather than
// JNIEnv::FindClass directly -- see the comment above JNI_OnLoad for why
// that matters when called from an attached native thread. Returns nullptr
// (with any pending exception cleared) on failure.
jclass find_app_class(JNIEnv* env, const char* slash_name)
{
    if (g_class_loader == nullptr || g_load_class_method == nullptr) {
        LOGE("find_app_class(%s): classloader was not captured", slash_name);
        return nullptr;
    }

    // ClassLoader.loadClass() takes a Java binary name (dots), not a JNI
    // internal name (slashes).
    std::string dotted_name(slash_name);
    for (char& c : dotted_name) {
        if (c == '/') c = '.';
    }

    jstring name = env->NewStringUTF(dotted_name.c_str());
    jclass clazz = static_cast<jclass>(env->CallObjectMethod(g_class_loader, g_load_class_method, name));
    env->DeleteLocalRef(name);

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }

    return clazz;
}

// Called from com.rtsoft.growtopia.Main.onCreate(), immediately after
// System.loadLibrary("growtopia") returns and before super.onCreate() (which
// is what creates the AppGLSurfaceView and starts the GL render thread --
// see SharedActivity.onCreate()).
//
// An earlier version of this hook installer ran on a background std::thread
// that polled dlopen(RTLD_NOLOAD) until libgrowtopia.so appeared, then
// attached itself to the JVM and installed the hooks from there. That thread
// had no ordering relationship with Growtopia's own startup: it could end up
// calling RegisterNatives (and, before that, resolving symbols) while
// Growtopia's GL thread was already alive and actively executing translated
// code, on an x86 emulator running everything through libhoudini.so. That is
// suspected to be the cause of an intermittent SIGSEGV observed entirely
// inside libhoudini.so on real test runs -- a race between our thread
// touching the process while Houdini is concurrently translating/running
// Growtopia's own code on another thread, not anything specific to how the
// hook itself was installed (RegisterNatives already ruled out inline code
// patching as the cause; see helper/hook.h).
//
// Calling this synchronously, on the same thread and at the one point where
// growtopia.so is guaranteed loaded but nothing of Growtopia's has started
// running yet, removes that race entirely: there is no other thread of
// Growtopia's alive at all yet for this to run concurrently with. It also
// means the JNIEnv handed to us here is already correct for this thread --
// no AttachCurrentThread/DetachCurrentThread needed.
extern "C" JNIEXPORT void JNICALL Java_com_gt_launcher_ModMenuBridge_installHooks(JNIEnv* env, jclass /*clazz*/)
{
    if (dlopen("libgrowtopia.so", RTLD_NOLOAD) == nullptr) {
        LOGE("installHooks: libgrowtopia.so is not loaded; mod menu disabled");
        return;
    }

    KittyMemory::ProcMap map = KittyMemory::getLibraryBaseMap("libgrowtopia.so");
    if (!map.isValid()) {
        LOGE("installHooks: failed to map libgrowtopia.so; mod menu disabled");
        return;
    }

    LOGD("libgrowtopia.so start address: 0x%llX", map.startAddress);

    // Create a global pointer to hold the class that will be
    // called inside the hook function.
    g_mod_menu = new ModMenu{};

    // Starting to hook Growtopia function.
    game::hook::init(env);
}
