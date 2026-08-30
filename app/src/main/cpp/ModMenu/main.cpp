#include <string>
#include <thread>
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
// game::hook::init() needs one to call RegisterNatives from the background
// thread spawned in Java_com_gt_launcher_ModMenuBridge_installHooks below,
// which is a plain std::thread with no JNIEnv of its own until it attaches.
//
// It also captures the app's own classloader, which that background thread
// needs for a much less obvious reason: JNIEnv::FindClass, called from a
// thread that was attached via AttachCurrentThread rather than one that
// originated from Java, resolves against the boot classloader instead of
// the app's own PathClassLoader -- a well-known Android JNI pitfall -- so it
// fails to find application classes like com.rtsoft.growtopia.AppRenderer.
// JNI_OnLoad itself runs synchronously on the thread that called
// System.loadLibrary("ModMenu"), which *is* a proper Java-originated thread,
// so FindClass here resolves correctly; the standard fix is to fetch that
// class's ClassLoader object here and use its loadClass() method later
// instead of calling FindClass directly from the attached thread.
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
// see SharedActivity.onCreate()) -- i.e. as early as it is possible to call
// this while still being guaranteed growtopia.so is loaded.
//
// This used to do the hook installation work inline, synchronously, on
// whatever thread called it. That thread turned out to be the app's main/UI
// thread (this is called straight from Activity.onCreate()), and on this
// emulator -- which is generally slow; native asset loading alone routinely
// takes several seconds even on a successful run -- that stalled onCreate()
// long enough to freeze the whole app: no crash, just a black screen until
// the activity manager's pause timeout fired and the process eventually had
// to be force-stopped. Doing any meaningfully slow work synchronously inside
// onCreate() risks exactly that, regardless of how fast it is expected to be.
//
// Before that, this ran on a background std::thread that polled
// dlopen(RTLD_NOLOAD) from process launch until libgrowtopia.so appeared,
// with no ordering relationship to Growtopia's own startup at all -- it
// could fire at any point during Growtopia's entire lifetime, including
// while its GL thread was already alive and actively executing translated
// code on this x86 emulator's ARM translator (libhoudini.so). That is
// suspected to be the cause of a separate, intermittent SIGSEGV observed
// entirely inside libhoudini.so on other test runs -- Growtopia's thread and
// ours both touching the process concurrently, not anything specific to how
// the hook itself was installed (RegisterNatives already ruled out inline
// code patching as the cause; see helper/hook.h).
//
// Splitting the difference: spawn the background thread here, the instant
// growtopia.so finishes loading, instead of leaving it polling independently
// from process launch. The actual work still happens off the main thread (no
// ANR risk), but the window in which it can race Growtopia's own render
// thread shrinks from "unbounded, any time during the game's entire runtime"
// down to the fixed, comparatively small amount of Java-side setup
// SharedActivity.onCreate() does before it creates the GLSurfaceView --
// which on this same slow emulator reliably takes far longer than the
// handful of dlsym/RegisterNatives calls this thread needs to make.
extern "C" JNIEXPORT void JNICALL Java_com_gt_launcher_ModMenuBridge_installHooks(JNIEnv* /*env*/, jclass /*clazz*/)
{
    if (dlopen("libgrowtopia.so", RTLD_NOLOAD) == nullptr) {
        LOGE("installHooks: libgrowtopia.so is not loaded; mod menu disabled");
        return;
    }

    std::thread([]() {
        JNIEnv* env = nullptr;
        if (g_jvm == nullptr || g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK || env == nullptr) {
            LOGE("installHooks: AttachCurrentThread failed; mod menu disabled");
            return;
        }

        KittyMemory::ProcMap map = KittyMemory::getLibraryBaseMap("libgrowtopia.so");
        if (!map.isValid()) {
            LOGE("installHooks: failed to map libgrowtopia.so; mod menu disabled");
            g_jvm->DetachCurrentThread();
            return;
        }

        LOGD("libgrowtopia.so start address: 0x%llX", map.startAddress);

        // Create a global pointer to hold the class that will be
        // called inside the hook function.
        g_mod_menu = new ModMenu{};

        // Starting to hook Growtopia function.
        game::hook::init(env);

        g_jvm->DetachCurrentThread();
    }).detach();
}
