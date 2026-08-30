#include <dobby.h>

#include "game_api.h"
#include "../utils/log.h"

namespace game {
namespace api {
namespace {

using fn_input_text = void (*)(JNIEnv*, jclass, jstring);
using fn_on_key = void (*)(JNIEnv*, jclass, jint, jint, jint);
using fn_get_float = jfloat (*)(JNIEnv*, jclass);
using fn_send_gui = void (*)(JNIEnv*, jclass, jint, jint, jint, jint);
using fn_void = void (*)(JNIEnv*, jclass);

// The magic keycode the game's own keyboard handler sends to commit typed
// text; see Main.OnKeyboardHeightChanged.
constexpr int kCommitTextKeycode = 500000;
constexpr int kKeyDown = 1;

struct State {
    bool initialized{ false };
    Capabilities caps{};

    fn_input_text input_text{ nullptr };
    fn_on_key on_key{ nullptr };
    fn_get_float screen_width{ nullptr };
    fn_get_float screen_height{ nullptr };
    fn_send_gui send_gui{ nullptr };
    fn_void cancel_button{ nullptr };
    fn_on_touch touch_passthrough{ nullptr };

    JNIEnv* env{ nullptr };
    jclass fallback_class{ nullptr };
    jclass shared_activity{ nullptr }; // global ref, resolved lazily
    bool shared_activity_tried{ false };
};

State& state()
{
    static State s{};
    return s;
}

template <typename Fn>
Fn resolve(const char* name)
{
    auto fn = reinterpret_cast<Fn>(DobbySymbolResolver("libgrowtopia.so", name));
    if (fn == nullptr) {
        LOGW("JNI export '%s' not found; dependent features disabled", name);
    } else {
        LOGI("JNI export '%s' -> %p", name, reinterpret_cast<void*>(fn));
    }
    return fn;
}

// These entry points are `public static native`, so the jclass they receive is
// SharedActivity's. Proton's implementations ignore it, but pass the real one
// where we can rather than relying on that.
jclass target_class()
{
    State& s = state();
    if (s.shared_activity != nullptr) {
        return s.shared_activity;
    }

    if (!s.shared_activity_tried && s.env != nullptr) {
        s.shared_activity_tried = true;

        jclass local = s.env->FindClass("com/rtsoft/growtopia/SharedActivity");
        if (local != nullptr) {
            s.shared_activity = static_cast<jclass>(s.env->NewGlobalRef(local));
            s.env->DeleteLocalRef(local);
            LOGI("Cached SharedActivity class ref");
        } else {
            // FindClass sets a pending exception on failure; clear it or the
            // next JNI call from this thread will abort.
            if (s.env->ExceptionCheck()) {
                s.env->ExceptionClear();
            }
            LOGW("FindClass(SharedActivity) failed; falling back to hook's jclass");
        }
    }

    return s.shared_activity != nullptr ? s.shared_activity : s.fallback_class;
}

} // namespace

void init()
{
    State& s = state();
    if (s.initialized) {
        return;
    }
    s.initialized = true;

    s.input_text = resolve<fn_input_text>("Java_com_rtsoft_growtopia_SharedActivity_nativeOnInputText");
    s.on_key = resolve<fn_on_key>("Java_com_rtsoft_growtopia_SharedActivity_nativeOnKey");
    s.screen_width = resolve<fn_get_float>("Java_com_rtsoft_growtopia_SharedActivity_nativeGetScreenWidth");
    s.screen_height = resolve<fn_get_float>("Java_com_rtsoft_growtopia_SharedActivity_nativeGetScreenHeight");
    s.send_gui = resolve<fn_send_gui>("Java_com_rtsoft_growtopia_SharedActivity_nativeSendGUIEx");
    s.cancel_button = resolve<fn_void>("Java_com_rtsoft_growtopia_SharedActivity_nativeCancelBtnPressed");

    s.caps.key = s.on_key != nullptr;
    s.caps.send_text = s.input_text != nullptr && s.on_key != nullptr;
    s.caps.screen_size = s.screen_width != nullptr && s.screen_height != nullptr;
    s.caps.gui_event = s.send_gui != nullptr;
    s.caps.cancel_button = s.cancel_button != nullptr;
    // caps.touch is set by set_touch_passthrough once the hook is installed.
}

const Capabilities& capabilities()
{
    return state().caps;
}

void set_touch_passthrough(fn_on_touch original)
{
    State& s = state();
    s.touch_passthrough = original;
    s.caps.touch = original != nullptr;
}

void set_frame_env(JNIEnv* env, jclass fallback_class)
{
    State& s = state();
    s.env = env;
    s.fallback_class = fallback_class;
}

void clear_frame_env()
{
    state().env = nullptr;
}

bool has_frame_env()
{
    return state().env != nullptr;
}

bool send_text(const std::string& text)
{
    State& s = state();
    if (!s.caps.send_text || s.env == nullptr) {
        return false;
    }

    jstring jtext = s.env->NewStringUTF(text.c_str());
    if (jtext == nullptr) {
        if (s.env->ExceptionCheck()) {
            s.env->ExceptionClear();
        }
        return false;
    }

    jclass clazz = target_class();
    s.input_text(s.env, clazz, jtext);
    s.env->DeleteLocalRef(jtext);

    // Commit it, the same way the game's keyboard handler does.
    s.on_key(s.env, clazz, kKeyDown, kCommitTextKeycode, 0);
    return true;
}

bool send_key(int type, int keycode, int character)
{
    State& s = state();
    if (!s.caps.key || s.env == nullptr) {
        return false;
    }

    s.on_key(s.env, target_class(), type, keycode, character);
    return true;
}

bool inject_touch(int action, float x, float y, int finger)
{
    State& s = state();
    if (s.touch_passthrough == nullptr || s.env == nullptr) {
        return false;
    }

    s.touch_passthrough(s.env, target_class(), action, x, y, finger);
    return true;
}

bool screen_size(float& width, float& height)
{
    State& s = state();
    if (!s.caps.screen_size || s.env == nullptr) {
        return false;
    }

    jclass clazz = target_class();
    width = s.screen_width(s.env, clazz);
    height = s.screen_height(s.env, clazz);
    return true;
}

bool send_gui_event(int message_type, int parm1, int parm2, int finger)
{
    State& s = state();
    if (!s.caps.gui_event || s.env == nullptr) {
        return false;
    }

    s.send_gui(s.env, target_class(), message_type, parm1, parm2, finger);
    return true;
}

bool cancel_button()
{
    State& s = state();
    if (!s.caps.cancel_button || s.env == nullptr) {
        return false;
    }

    s.cancel_button(s.env, target_class());
    return true;
}

} // api
} // game
