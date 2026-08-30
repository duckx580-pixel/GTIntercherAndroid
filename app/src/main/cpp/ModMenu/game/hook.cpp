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

extern ModMenu* g_mod_menu;

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
INSTALL_HOOK(AppRenderer__nativeRender, void, JNIEnv* env, jclass clazz)
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

    g_mod_menu->m_ui->set_display_size(display_size);
    g_mod_menu->m_ui->render();
}

// AppGLSurfaceView.nativeOnTouch(int action, float x, float y, int finger).
// Both the single-touch path (AppGLSurfaceView.onTouchEvent) and the
// multi-touch path (SharedMultiTouchInput.processMouse) funnel through here, so
// this one hook sees every touch. `action` is an Android MotionEvent action,
// with ACTION_POINTER_DOWN/UP already normalized to ACTION_DOWN/UP.
INSTALL_HOOK(AppGLSurfaceView__nativeOnTouch, void,
    JNIEnv* env, jclass clazz, jint action, jfloat x, jfloat y, jint finger)
{
    if (g_mod_menu->m_ui != nullptr) {
        g_mod_menu->m_ui->on_touch(action, finger, x, y);
    }

    // Swallow the touch when ImGui owns it so it does not also reach the game.
    if (g_mod_menu->m_ui != nullptr && ImGui::GetCurrentContext() != nullptr
        && ImGui::GetIO().WantCaptureMouse) {
        return;
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

    install_hook_AppRenderer__nativeRender(render_addr);

    if (touch_addr != nullptr) {
        install_hook_AppGLSurfaceView__nativeOnTouch(touch_addr);
    }
}
} // hook
} // game
