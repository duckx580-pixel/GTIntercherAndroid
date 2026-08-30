#pragma once
#include <jni.h>
#include <string>

// Growtopia 5.55 ships a stripped libgrowtopia.so, so none of the engine's
// internal C++ functions can be reached. Its JNI entry points, however, are
// still exported -- and they are not only hookable but *callable*, which is
// where the menu's actual features come from.
//
// Everything here runs on the GL/JNI thread: the UI is drawn from inside the
// nativeRender hook, so the JNIEnv captured there is valid for the whole draw.
namespace game {
namespace api {

// Which of the game's JNI entry points were found on this build.
struct Capabilities {
    bool send_text{ false };   // nativeOnInputText + nativeOnKey
    bool key{ false };         // nativeOnKey
    bool touch{ false };       // nativeOnTouch (injection, via the unhooked original)
    bool screen_size{ false }; // nativeGetScreenWidth / nativeGetScreenHeight
    bool gui_event{ false };   // nativeSendGUIEx
    bool cancel_button{ false };// nativeCancelBtnPressed
};

using fn_on_touch = void (*)(JNIEnv*, jclass, jint, jfloat, jfloat, jint);

// Resolve the exported JNI symbols. Safe to call more than once.
void init();

const Capabilities& capabilities();

// Injecting a touch must not re-enter our own nativeOnTouch hook, or the
// synthetic event would be swallowed by the ImGui capture check that hook
// applies. The hook hands us Dobby's trampoline to the original so injection
// bypasses it.
void set_touch_passthrough(fn_on_touch original);

// Publish the JNIEnv for the current frame. Called at the top of the render
// hook; clears again when the frame ends.
void set_frame_env(JNIEnv* env, jclass fallback_class);
void clear_frame_env();
bool has_frame_env();

// --- Features -------------------------------------------------------------
// Each returns false when the underlying symbol is unavailable or there is no
// live JNIEnv, so callers can surface that instead of failing silently.

// Types `text` into the game and commits it, exactly the way the game's own
// keyboard handler does (Main.OnKeyboardHeightChanged): nativeOnInputText
// followed by nativeOnKey(1, 500000, 0).
bool send_text(const std::string& text);

bool send_key(int type, int keycode, int character);

// action is an Android MotionEvent code (0 down, 1 up, 2 move).
bool inject_touch(int action, float x, float y, int finger);

bool screen_size(float& width, float& height);

bool send_gui_event(int message_type, int parm1, int parm2, int finger);

bool cancel_button();

} // api
} // game
