#include <cstdio>
#include <cstring>
#include <android/log.h>
#include <dobby.h>
#include <jni.h>
#include <GLES2/gl2.h>
#include <KittyMemory.h>

#include "../helper/hook.h"
#include "../ui/ui.h"
#include "../mod_menu.h"
#include "game_api.h"

extern ModMenu* g_mod_menu;
extern JavaVM* g_jvm;

namespace {
// Growtopia 5.55 ships a fully stripped libgrowtopia.so: the internal C++
// symbols the old hooks relied on (BaseApp::Draw, AppOnTouch, GetScreenSizeXf,
// ...) are no longer in .dynsym. The JNI entry points, however, must stay
// exported for the runtime to bind them, and they are named by the ABI rather
// than by the compiler, so they survive version bumps.
//
// Resolve against a candidate list so a single build works on both the modern
// (JNI) and legacy (internal symbol) layouts.
void* resolve(const char* const* candidates, std::size_t count)
{
    for (std::size_t i = 0; i < count; ++i) {
        // Naming the image costs nothing on the dlsym(RTLD_DEFAULT) path Dobby
        // tries first, and lets its ELF-scanning fallback actually run.
        void* addr = DobbySymbolResolver("libgrowtopia.so", candidates[i]);
        if (addr != nullptr) {
            LOGI("Resolved '%s' -> %p", candidates[i], addr);
            return addr;
        }

        LOGW("Symbol '%s' not found, trying next candidate", candidates[i]);
    }

    return nullptr;
}

template <std::size_t N>
void* resolve(const char* const (&candidates)[N])
{
    return resolve(candidates, N);
}

// Read the live viewport instead of calling the (now unexported)
// GetScreenSizeXf/GetScreenSizeYf helpers. This is queried from the GL thread
// with the context current, so it also tracks rotation and resizes for free.
bool query_display_size(ImVec2& out)
{
    GLint viewport[4]{};
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (viewport[2] <= 0 || viewport[3] <= 0) {
        return false;
    }

    out = ImVec2{ static_cast<float>(viewport[2]), static_cast<float>(viewport[3]) };
    return true;
}
} // namespace

// AppRenderer.nativeRender() is invoked once per frame from onDrawFrame, on the
// GL thread with the context current. Draw the game first, then stack the
// overlay on top of it.
INSTALL_JNI_HOOK(AppRenderer__nativeRender, void, JNIEnv* env, jclass clazz)
{
    orig_AppRenderer__nativeRender(env, clazz);

    ImVec2 display_size{};
    if (!query_display_size(display_size)) {
        return;
    }

    if (g_mod_menu->m_ui == nullptr) {
        g_mod_menu->m_ui = new ui::Ui{ display_size };
        if (!g_mod_menu->m_ui->init()) {
            LOGE("Failed to initialize ImGui");

            delete g_mod_menu->m_ui;
            g_mod_menu->m_ui = nullptr;
            return;
        }

        LOGI("ImGui initialized at %.0fx%.0f", display_size.x, display_size.y);
    }

    // The menu calls back into the game's JNI entry points while it draws, so
    // publish this frame's env for the duration of the draw. It is only valid
    // on this thread, and only inside this call.
    game::api::set_frame_env(env, clazz);

    g_mod_menu->m_ui->set_display_size(display_size);
    g_mod_menu->m_ui->render();

    game::api::clear_frame_env();
}

// AppGLSurfaceView.nativeOnTouch(int action, float x, float y, int finger).
// Both the single-touch path (AppGLSurfaceView.onTouchEvent) and the
// multi-touch path (SharedMultiTouchInput.processMouse) funnel through here, so
// this one hook sees every touch. `action` is an Android MotionEvent action,
// with ACTION_POINTER_DOWN/UP already normalized to ACTION_DOWN/UP.
INSTALL_JNI_HOOK(AppGLSurfaceView__nativeOnTouch, void,
    JNIEnv* env, jclass clazz, jint action, jfloat x, jfloat y, jint finger)
{
    if (g_mod_menu->m_ui != nullptr) {
        g_mod_menu->m_ui->on_touch(action, finger, x, y);

        // Swallow the touch when the menu owns it so it does not also reach
        // the game underneath.
        if (g_mod_menu->m_ui->wants_input()) {
            return;
        }
    }

    orig_AppGLSurfaceView__nativeOnTouch(env, clazz, action, x, y, finger);
}

namespace game {
namespace hook {
void init()
{
    // set Dobby logging level.
    log_set_level(0);

    static const char* const render_symbols[]{
        // Growtopia 5.55+ (stripped binary, JNI entry point).
        "Java_com_rtsoft_growtopia_AppRenderer_nativeRender",
        // Legacy builds that still exported BaseApp::Draw(void).
        "_ZN7BaseApp4DrawEv",
    };

    static const char* const touch_symbols[]{
        // Growtopia 5.55+ (stripped binary, JNI entry point).
        "Java_com_rtsoft_growtopia_AppGLSurfaceView_nativeOnTouch",
        // Legacy AppOnTouch(_JNIEnv *,_jobject *,int,float,float,int).
        "_Z10AppOnTouchP7_JNIEnvP8_jobjectiffi",
    };

    void* render_addr = resolve(render_symbols);
    if (render_addr == nullptr) {
        LOGE("Could not resolve a render function; mod menu disabled");
        return;
    }

    void* touch_addr = resolve(touch_symbols);
    if (touch_addr == nullptr) {
        LOGE("Could not resolve a touch function; mod menu will not accept input");
    }

    // RegisterNatives needs a JNIEnv for this thread. This runs on the
    // background poller thread from main.cpp, a plain std::thread with no
    // JNIEnv of its own, so attach it to the JVM captured in JNI_OnLoad.
    if (g_jvm == nullptr) {
        LOGE("No JavaVM captured (JNI_OnLoad did not run?); mod menu disabled");
        return;
    }

    JNIEnv* env = nullptr;
    if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK || env == nullptr) {
        LOGE("AttachCurrentThread failed; mod menu disabled");
        return;
    }

    bool render_hooked = install_hook_AppRenderer__nativeRender(
        env, "com/rtsoft/growtopia/AppRenderer", "nativeRender", "()V", render_addr);

    if (!render_hooked) {
        g_jvm->DetachCurrentThread();
        LOGE("Failed to install render hook; mod menu disabled");
        return;
    }

    if (touch_addr != nullptr) {
        bool touch_hooked = install_hook_AppGLSurfaceView__nativeOnTouch(
            env, "com/rtsoft/growtopia/AppGLSurfaceView", "nativeOnTouch", "(IFFI)V", touch_addr);

        if (touch_hooked) {
            // Hand the menu the captured original so injected taps reach the
            // game directly instead of re-entering our own hook.
            game::api::set_touch_passthrough(orig_AppGLSurfaceView__nativeOnTouch);
        }
    }

    // This thread only needed the JNIEnv to install the hooks above; the
    // hooks themselves receive their own JNIEnv on whatever thread ART
    // calls them from later.
    g_jvm->DetachCurrentThread();

    // Resolve the JNI entry points the menu calls into for its features.
    game::api::init();
}
} // hook
} // game
