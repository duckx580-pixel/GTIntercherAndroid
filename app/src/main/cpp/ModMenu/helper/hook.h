#pragma once
#include <dlfcn.h>
#include <jni.h>
#include <dobby.h>

#include "../utils/log.h"

// Defined in main.cpp; see the comment above JNI_OnLoad there for why
// INSTALL_JNI_HOOK below must go through this instead of a plain
// JNIEnv::FindClass call.
jclass find_app_class(JNIEnv* env, const char* slash_name);

#define INSTALL_HOOK(fn_name_t, fn_ret_t, fn_args_t...)                                                                    \
    fn_ret_t (*orig_##fn_name_t)(fn_args_t);                                                                               \
    fn_ret_t fake_##fn_name_t(fn_args_t);                                                                                  \
    static void install_hook_##fn_name_t(void* sym_addr)                                                                   \
    {                                                                                                                      \
        hook_function(sym_addr, (dobby_dummy_func_t)fake_##fn_name_t, (dobby_dummy_func_t*)&orig_##fn_name_t);             \
    }                                                                                                                      \
    static void install_hook_##fn_name_t(const char* lib, const char* name)                                                \
    {                                                                                                                      \
        void *sym_addr = DobbySymbolResolver(lib, name);                                                                   \
        install_hook_##fn_name_t(sym_addr);                                                                                \
    }                                                                                                                      \
    static void install_hook_##fn_name_t(const char* name)                                                                 \
    {                                                                                                                      \
        install_hook_##fn_name_t(nullptr, name);                                                                           \
    }                                                                                                                      \
    fn_ret_t fake_##fn_name_t(fn_args_t)

inline int hook_function(void* original, dobby_dummy_func_t replace, dobby_dummy_func_t* backup)
{
    Dl_info info;
    if (dladdr(original, &info)) {
        LOGI("Hooking %s (%p) from %s (%p)",
            info.dli_sname ? info.dli_sname : "(unknown symbol)", info.dli_saddr,
            info.dli_fname ? info.dli_fname : "(unknown file)", info.dli_fbase);
    }

    return DobbyHook(original, replace, backup);
}

inline int unhook_function(void* original)
{
    Dl_info info;
    if (dladdr(original, &info)) {
        LOGI("Unhooking %s (%p) from %s (%p)",
            info.dli_sname ? info.dli_sname : "(unknown symbol)", info.dli_saddr,
            info.dli_fname ? info.dli_fname : "(unknown file)", info.dli_fbase);
    }

    return DobbyDestroy(original);
}

// Intercepts a JNI-exported native method by replacing ART's own method
// table entry for it (JNIEnv::RegisterNatives), instead of rewriting the
// target function's machine code in place the way DobbyHook (above) does.
//
// This exists specifically because inline patching crashed under an x86
// Android emulator's ARM translation layer (libhoudini.so) with a SIGSEGV
// deep inside the translator itself, on a real device test of this project.
// Binary translators pre-translate and cache code; overwriting a function's
// bytes after it has already been translated is a well-known way to break
// that cache. RegisterNatives never touches the target's code at all -- it
// is a plain, JNI-specification-supported pointer write into the VM's own
// method table, so there is nothing for a translator to have stale-cached.
// It applies cleanly here because both of this project's hook targets
// (AppRenderer.nativeRender, AppGLSurfaceView.nativeOnTouch) are JNI native
// methods to begin with, not arbitrary internal C++ functions.
#define INSTALL_JNI_HOOK(fn_name_t, fn_ret_t, fn_args_t...)                                                                 \
    fn_ret_t (*orig_##fn_name_t)(fn_args_t){ nullptr };                                                                     \
    fn_ret_t fake_##fn_name_t(fn_args_t);                                                                                   \
    static bool install_hook_##fn_name_t(                                                                                   \
        JNIEnv* env, const char* class_name, const char* method_name, const char* signature, void* original_addr)          \
    {                                                                                                                       \
        orig_##fn_name_t = reinterpret_cast<fn_ret_t (*)(fn_args_t)>(original_addr);                                        \
                                                                                                                              \
        jclass clazz = find_app_class(env, class_name);                                                                     \
        if (clazz == nullptr) {                                                                                             \
            LOGE("RegisterNatives: class '%s' not found", class_name);                                                      \
            return false;                                                                                                   \
        }                                                                                                                   \
                                                                                                                              \
        JNINativeMethod method{ method_name, signature, reinterpret_cast<void*>(fake_##fn_name_t) };                        \
        bool ok = env->RegisterNatives(clazz, &method, 1) == JNI_OK;                                                        \
        if (!ok && env->ExceptionCheck()) env->ExceptionClear();                                                            \
        env->DeleteLocalRef(clazz);                                                                                         \
                                                                                                                              \
        if (ok) {                                                                                                           \
            LOGI("RegisterNatives installed for %s.%s%s (original %p)", class_name, method_name, signature, original_addr); \
        } else {                                                                                                            \
            LOGE("RegisterNatives failed for %s.%s%s", class_name, method_name, signature);                                 \
        }                                                                                                                   \
        return ok;                                                                                                          \
    }                                                                                                                        \
    fn_ret_t fake_##fn_name_t(fn_args_t)
